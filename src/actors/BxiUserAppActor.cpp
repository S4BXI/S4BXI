/*
 * Author: Julien EMMANUEL
 * Copyright (C) 2019-2021 Bull S.A.S
 * All rights reserved
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation,
 * which comes with this package.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include "s4bxi/actors/BxiUserAppActor.hpp"
#include "s4bxi/s4bxi_util.hpp"

#include <dlfcn.h>
#include <fcntl.h>
#include <fstream>
#include <link.h>
#include <s4bxi/s4bxi.hpp>
#include <simgrid/s4u.hpp>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <boost/algorithm/string.hpp>
#include <csignal>
#include <map>
#include "s4bxi/s4bxi_xbt_log.h"
#include "s4bxi/s4bxi_bench.hpp"
#include "s4bxi/plugins/BxiActorExt.hpp"
#include "pugixml.hpp"

#ifdef BUILD_MPI_MIDDLEWARE
#include <smpi/smpi.h>
#include "smpi_coll.hpp"
#include "s4bxi/mpi_middleware/s4bxi_mpi_middleware.h"
#endif

static const std::string smpi_default_instance_name("smpirun");

using namespace std;
using namespace simgrid;

S4BXI_LOG_NEW_DEFAULT_CATEGORY(user_app, "Messages relative to user app actor");

#if !RTLD_DEEPBIND || __SANITIZE_ADDRESS__ || HAVE_SANITIZER_ADDRESS || HAVE_SANITIZER_THREAD
#define WANT_RTLD_DEEPBIND 0
#else
#define WANT_RTLD_DEEPBIND RTLD_DEEPBIND
#endif

typedef int (*s4bxi_entry_point_type)(int argc, char** argv);
typedef void (*cpp_platform_callback)();
typedef void (*cpp_config_callback)(const simgrid::s4u::Engine&);

#define S4BXI_ABORT(...)                                                                                               \
    do {                                                                                                               \
        XBT_CRITICAL(__VA_ARGS__);                                                                                     \
        xbt_abort();                                                                                                   \
    } while (0)

vector<string> privatize_libs_paths;
off_t fdin_size;

string user_app_name;
string executable;
string simulation_rand_id = "0000000000";

void* smpi_lib;

map<string, uint32_t, less<>> local_ranks;

static void s4bxi_copy_file(const string& src, const string& target, off_t fdin_size)
{
    int fdin = open(src.c_str(), O_RDONLY);
    xbt_assert(fdin >= 0,
               "Cannot read from %s. Please make sure that the file exists and "
               "is executable.",
               src.c_str());
    int fdout = open(target.c_str(), O_CREAT | O_RDWR, S_IRWXU);
    xbt_assert(fdout >= 0, "Cannot write into %s", target.c_str());

    ssize_t sent_size = sendfile(fdout, fdin, NULL, fdin_size);
    if (sent_size == fdin_size) {
        close(fdin);
        close(fdout);
        return;
    } else if (sent_size != -1 || errno != ENOSYS) {
        XBT_ERROR("Error while copying %s: only %zd bytes copied instead of %ld "
                  "(errno: %d -- %s)",
                  target.c_str(), sent_size, static_cast<intmax_t>(fdin_size), errno, strerror(errno));
    }
}

static int visit_libs(struct dl_phdr_info* info, size_t, void* data)
{
    auto libname     = (char*)(data);
    const char* path = info->dlpi_name;
    if (strstr(path, libname)) {
        strncpy(libname, path, 512);
        return 1;
    }

    return 0;
}

static s4bxi_entry_point_type s4bxi_resolve_function(void* handle)
{
    auto entry_point = (s4bxi_entry_point_type)dlsym(handle, "main");
    if (entry_point != nullptr) {
        return entry_point;
    }

    return s4bxi_entry_point_type();
}

static void s4bxi_init_privatization_dlopen(const string& e)
{
    // Prepare the copy of the binary (get its size)
    struct stat fdin_stat;
    stat(e.c_str(), &fdin_stat);
    fdin_size = fdin_stat.st_size;

    string libnames = S4BXI_GLOBAL_CONFIG(privatize_libs);
    if (not libnames.empty()) {
        // split option
        vector<string> privatize_libs;
        boost::split(privatize_libs, libnames, boost::is_any_of(";"));

        for (auto const& libname : privatize_libs) {
            // load the library once to add it to the local libs, to get the absolute path
            void* libhandle;
            if (!(libhandle = dlopen(libname.c_str(), RTLD_LAZY)))
                S4BXI_ABORT("dlopen %s error: %s", libname.c_str(), dlerror());
            // get library name from path
            char fullpath[512] = {'\0'};
            strncpy(fullpath, libname.c_str(), 511);

            // This is separate from the xbt_assert call because the xbt_assert will not be compiled in release
            int rc = dl_iterate_phdr(visit_libs, fullpath);
            xbt_assert(0 != rc,
                       "Can't find a linked %s - check your settings in "
                       "s4bxi/privatize-libs",
                       fullpath);
            XBT_DEBUG("Extra lib to privatize '%s' found", fullpath);
            privatize_libs_paths.push_back(fullpath);

            if (!S4BXI_GLOBAL_CONFIG(no_dlclose))
                dlclose(libhandle);
        }
    }
}

BxiUserAppActor::BxiUserAppActor(const vector<string>& args) : BxiMainActor(args), string_args(args) {}

/**
 * Mostly stolen from SMPI
 */
void BxiUserAppActor::operator()()
{
    map<string, string, less<>>* privatize_libs_renames = new map<string, string, less<>>;

    my_rank   = stoul(string(self->get_property("rank")));
    auto pair = local_ranks.find(getSlug());
    if (pair == local_ranks.end()) {
        my_local_rank = 0;
        local_ranks.emplace(getSlug(), 0);
    } else {
        my_local_rank = ++pair->second;
    }

    XBT_INFO("Init rank %d ; local_rank %d", my_rank, my_local_rank);

    setup_barrier();

    // Copy the dynamic library:
    string target_executable = executable + "_" + simulation_rand_id + "_" + to_string(my_rank) + ".so";

    s4bxi_copy_file(executable, target_executable, fdin_size);
    // if s4bxi/privatize-libs is set, duplicate pointed lib and link each
    // executable copy to a different one.
    for (auto const& libpath : privatize_libs_paths) {
        // if we were given a full path, strip it
        size_t index = libpath.find_last_of("/\\");
        string libname;
        if (index != string::npos)
            libname = libpath.substr(index + 1);

        if (not libname.empty()) {
            // load the library to add it to the local libs, to get the absolute
            // path
            struct stat fdin_stat2;
            stat(libpath.c_str(), &fdin_stat2);
            off_t fdin_size2 = fdin_stat2.st_size;

            // Copy the dynamic library, the new name must be the same length as the
            // old one just replace the name with 7 digits for the rank and the rest
            // of the name.
            unsigned int pad = 7;
            if (libname.length() < pad)
                pad = libname.length();
            string target_lib = simulation_rand_id.substr(0, pad - to_string(my_rank).length()) + to_string(my_rank) +
                                libname.substr(pad);
            privatize_libs_renames->emplace(libname, target_lib);
            XBT_DEBUG("copy lib %s to %s, with size %lld", libpath.c_str(), target_lib.c_str(), (long long)fdin_size2);
            s4bxi_copy_file(libpath, target_lib, fdin_size2);

            string sedcommand = "sed -i -e 's/" + libname + "/" + target_lib + "/g' " + target_executable;
            XBT_DEBUG("Relinking user code:\n → %s\n", sedcommand.c_str());
            int rc = system(sedcommand.c_str());
            xbt_assert(rc == 0, "error while applying sed command %s \n", sedcommand.c_str());

            if (libname == "libmpi.so.40") { // Fetch MPI ops if we are handling the MPI lib
                bull_mpi_lib = target_lib;
            }
        }
    }

    // <custom-things>
    // S4BXI addition : also privatize privatized libs (not just the user code)
    // Actually that doesn't work for MPI because some libs are never linked but dlopened at runtime,
    // ~~I don't really have a solution to this problem~~
    // → Actually it works if OMPI is configured with --disable-dlopen (so that everything is in the
    // three libs OMPI, ORTE and OPAL)

    for (auto const& relink_me : *privatize_libs_renames) {
        for (auto const& target_lib : *privatize_libs_renames) {
            if (target_lib.second == relink_me.second)
                continue;
            string sedcommand =
                "sed -i -e 's/" + target_lib.first + "/" + target_lib.second + "/g' " + relink_me.second;
            XBT_DEBUG("Relinking privatized lib:\n → %s\n", sedcommand.c_str());
            int rc = system(sedcommand.c_str());
            xbt_assert(rc == 0, "error while applying sed command %s \n", sedcommand.c_str());
        }
    }

    // </custom-things>

    // Load the copy and resolve the entry point:
    void* handle;
    if (!(handle = dlopen(target_executable.c_str(), RTLD_LAZY | RTLD_LOCAL | WANT_RTLD_DEEPBIND)))
        S4BXI_ABORT("dlopen %s error: %s", target_executable.c_str(), dlerror());
    int saved_errno = errno;

#ifdef BUILD_MPI_MIDDLEWARE
    if (!bull_mpi_lib.empty()) {
        void* bull_lib;
        if (!(bull_lib = dlopen(bull_mpi_lib.c_str(), RTLD_LAZY | RTLD_LOCAL | WANT_RTLD_DEEPBIND)))
            S4BXI_ABORT("dlopen %s error: %s", bull_mpi_lib.c_str(), dlerror());
        XBT_INFO("Extracting symbols from Bull lib %p (%s) and SMPI lib %p", bull_lib, bull_mpi_lib.c_str(), smpi_lib);
        set_mpi_middleware_ops(bull_lib, smpi_lib);
        dlclose(bull_lib);
    }
#endif

    if (S4BXI_GLOBAL_CONFIG(keep_temps) == false) {
        unlink(target_executable.c_str());
        for (const auto& target_lib : *privatize_libs_renames)
            unlink(target_lib.second.c_str());
    }
    xbt_assert(handle != nullptr, "dlopen failed: %s (errno: %d -- %s)", dlerror(), saved_errno, strerror(saved_errno));

    delete privatize_libs_renames; // No need to keep this in memory when running user app

    s4bxi_entry_point_type entry_point = s4bxi_resolve_function(handle);
    xbt_assert(entry_point, "Could not resolve entry point");

    // copy C strings, we need them writable
    auto* args4argv = new std::vector<char*>(string_args.size());
    std::transform(std::begin(string_args), std::end(string_args), begin(*args4argv),
                   [](const std::string& s) { return xbt_strdup(s.c_str()); });

    // set argv[0] to executable_path
    xbt_free((*args4argv)[0]);
    (*args4argv)[0] = xbt_strdup(user_app_name.c_str());

    // take a copy of args4argv to keep reference of the allocated strings
    const std::vector<char*> args2str(*args4argv);
    int argc = args4argv->size();
    args4argv->push_back(nullptr);
    char** argv = args4argv->data();

    s4bxi_barrier();

    const char* prop = self->get_property("delay_start");
    if (prop)
        s4u::this_actor::sleep_for(atof(prop));

    s4bxi_bench_begin();

    entry_point(argc, argv);

    s4bxi_bench_end();

    for (char* s : args2str)
        xbt_free(s);
    delete args4argv;

    if (!S4BXI_GLOBAL_CONFIG(no_dlclose))
        dlclose(handle);

    s4bxi_barrier();
}

template <typename T> class BxiActorFactory {
  public:
    vector<string> args;
    BxiActorFactory(vector<string> a) { args = a; }
    void operator()()
    {
        T actor(args);
        actor();
    }
};

/**
 * See segvhandler in SimGrid's EngineImpl.cpp, we're modifying it to display the current backtrace
 */
void app_signal_handler(int nSignum, siginfo_t* siginfo, void* vcontext)
{
    if ((siginfo->si_signo == SIGSEGV && siginfo->si_code == SEGV_ACCERR) || siginfo->si_signo == SIGBUS) {
        XBT_CRITICAL("Access violation or Bus error detected.\n"
                     "This probably comes from a programming error in your code, or from a stack\n"
                     "overflow. If you are certain of your code, try increasing the stack size\n"
                     "   --cfg=contexts/stack-size:XXX.\n"
                     "\n"
                     "If it does not help, this may have one of the following causes:\n"
                     "a bug in SimGrid, a bug in the OS or a bug in a third-party libraries.\n"
                     "Failing hardware can sometimes generate such errors too.\n"
                     "\n"
                     "If you think you've found a bug in SimGrid, please report it along with a\n"
                     "Minimal Working Example (MWE) reproducing your problem and a full backtrace\n"
                     "of the fault captured with gdb or valgrind.");
    } else if (siginfo->si_signo == SIGSEGV) {
        XBT_CRITICAL("Segmentation fault.");
        xbt_backtrace_display_current();
    }

    // See https://stackoverflow.com/a/39269908
    _exit(128 + nSignum);
}

int s4bxi_default_main(int argc, char* argv[])
{
#ifdef BUILD_MPI_MIDDLEWARE
    if (!(smpi_lib = dlopen("libsimgrid.so", RTLD_LAZY | RTLD_LOCAL | WANT_RTLD_DEEPBIND)))
        S4BXI_ABORT("dlopen %s error: %s", "libsimgrid.so", dlerror());
#endif

    s4bxi_actor_ext_plugin_init();
    auto simgrid_engine = new s4u::Engine(&argc, argv);
    xbt_assert(argc > 4, "Usage: %s platform_file deployment_file user_app_path user_app_name\n", argv[0]);

    string platf  = argv[1];
    string deploy = argv[2];
    executable    = argv[3];
    user_app_name = argv[4];

    /* Load the platform description and then deploy the application */
    string xml      = ".xml";
    void* platf_lib = nullptr;
    // Check if file ends with ".xml", if it doesn't it's a dll that we dlopen
    if (platf.compare(platf.length() - xml.length(), xml.length(), xml)) {
        if (!(platf_lib = dlopen(platf.c_str(), RTLD_LAZY)))
            S4BXI_ABORT("dlopen %s error: %s", platf.c_str(), dlerror());
    }

    simgrid_engine->set_config("surf/precision:1e-9");
    simgrid_engine->set_config("network/loopback-lat:0");
    simgrid_engine->set_config("network/loopback-bw:999e9");
    simgrid_engine->set_config("network/model:CM02");

    char random_characters[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                                'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                                'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                                'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
    for (int i = 0; i < simulation_rand_id.length(); i++)
        simulation_rand_id[i] = random_characters[random_int(0, 51)];

    BxiEngine::get_instance()->set_simulation_rand_id(simulation_rand_id);

    s4bxi_init_privatization_dlopen(executable);

#ifdef BUILD_MPI_MIDDLEWARE
    smpi_init_options();
#endif

    if (platf_lib) {
        cpp_config_callback sym;
        if (!(sym = (cpp_config_callback)dlsym(platf_lib, "configure_engine")))
            XBT_WARN("No engine configuration found in platform (%s)", dlerror());
        else
            sym(*simgrid_engine);
    }

#ifdef BUILD_MPI_MIDDLEWARE
    simgrid_engine->set_config("smpi/coll-selector:ompi");

    simgrid::smpi::colls::set_collectives();
    simgrid::smpi::colls::smpi_coll_cleanup_callback = nullptr;
    SMPI_init();
#endif

    /* Load the platform description and then deploy the application */
    if (platf_lib) { // If we have a dll fetch the symbol, otherwise load the XML
        cpp_platform_callback sym;
        if (!(sym = (cpp_platform_callback)dlsym(platf_lib, "load_platform")))
            S4BXI_ABORT("dlsym %s error: %s", "load_platform", dlerror());
        sym();
        if (!S4BXI_GLOBAL_CONFIG(no_dlclose))
            dlclose(platf_lib);
    } else {
        simgrid_engine->load_platform(platf);
    }

#ifdef BUILD_MPI_MIDDLEWARE
    simgrid_engine->set_default_comm_data_copy_callback(smpi_comm_copy_buffer_callback);
#endif

    /* Load deployment */
    if (!S4BXI_GLOBAL_CONFIG(use_pugixml)) {
        /* Register the classes representing the actors */
        simgrid_engine->register_actor<BxiNicInitiator>("nic_initiator");
        simgrid_engine->register_actor<BxiNicTarget>("nic_target");
        simgrid_engine->register_actor<BxiNicE2E>("nic_e2e");
        simgrid_engine->register_actor<BxiUserAppActor>("user_app");

        simgrid_engine->load_deployment(deploy);
    } else {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(deploy.c_str());
        if (!result) {
            XBT_ERROR("Error during parsing of XML deploy");
            return 1;
        }

        for (pugi::xml_node node : doc.child("platform").children()) {
            if (strcmp(node.name(), "actor")) {
                XBT_WARN("Unexpected tag in XML deployment: %s (expected %s)", node.name(), "actor");
                continue;
            }
            vector<string> actor_args = {string(node.name())};

            for (pugi::xml_node arg : node.children("argument"))
                actor_args.push_back(string(arg.attribute("value").value()));

            const char* func = node.attribute("function").value();
            s4u::Host* host  = simgrid_engine->host_by_name(string(node.attribute("host").value()));
            assert(host);
            s4u::ActorPtr actorPtr = s4u::Actor::init(func, host);

            for (pugi::xml_node arg : node.children("prop"))
                actorPtr->set_property(string(arg.attribute("id").value()), string(arg.attribute("value").value()));

            if (!strcmp(func, "nic_initiator"))
                actorPtr->start(BxiActorFactory<BxiNicInitiator>(actor_args));
            else if (!strcmp(func, "nic_target"))
                actorPtr->start(BxiActorFactory<BxiNicTarget>(actor_args));
            else if (!strcmp(func, "nic_e2e"))
                actorPtr->start(BxiActorFactory<BxiNicE2E>(actor_args));
            else if (!strcmp(func, "user_app")) {
                actorPtr->start(BxiActorFactory<BxiUserAppActor>(actor_args));
            } else {
                XBT_WARN("Unexpected actor function in deployment: %s", func);
                continue;
            }
        }
    }
    int rank_counts = 0;
    for (auto actor : simgrid_engine->get_all_actors()) {
        if (actor->get_name() == "user_app") {
            actor->set_property("instance_id", smpi_default_instance_name.c_str());
            if (!actor->get_property("rank"))
                actor->set_property("rank", to_string(rank_counts));
            rank_counts++;
        }
    }

#ifdef BUILD_MPI_MIDDLEWARE
    SMPI_app_instance_register(smpi_default_instance_name.c_str(), nullptr, rank_counts);
#endif

    // By default the simulation fails "silently" (shows an error message but returns with code 0) in case of deadlock.
    // Throwing an error allows us to see what was going at the time of deadlock in GDB
    s4u::Engine::on_deadlock_cb([]() { abort(); });

    struct sigaction app_action;
    memset(&app_action, 0, sizeof(struct sigaction));
    app_action.sa_flags     = SA_SIGINFO;
    app_action.sa_sigaction = app_signal_handler;
    sigaction(SIGSEGV, &app_action, NULL);

    /* Run the simulation */
    simgrid_engine->run();

    // Remove our signal (reset the default one)
    signal(SIGSEGV, SIG_DFL);

#ifdef BUILD_MPI_MIDDLEWARE
    SMPI_finalize();
    dlclose(smpi_lib);
#endif

    BxiEngine::get_instance()->end_simulation();

    delete simgrid_engine;

    return 0;
}