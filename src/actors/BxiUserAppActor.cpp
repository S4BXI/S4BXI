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

#include <dlfcn.h>
#include <fcntl.h>
#include <fstream>
#include <link.h>
#include <s4bxi/s4bxi.hpp>
#include <simgrid/s4u.hpp>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <boost/algorithm/string.hpp>
#include "s4bxi/s4bxi_xbt_log.h"
#include "s4bxi/s4bxi_bench.hpp"
#include "s4bxi/plugins/BxiActorExt.hpp"
#include <smpi/smpi.h>

#include "smpi_coll.hpp"

static const std::string smpi_default_instance_name("smpirun");

using namespace std;

S4BXI_LOG_NEW_DEFAULT_CATEGORY(user_app, "Messages relative to user app actor");

#if !RTLD_DEEPBIND || __SANITIZE_ADDRESS__ || HAVE_SANITIZER_ADDRESS || HAVE_SANITIZER_THREAD
#define WANT_RTLD_DEEPBIND 0
#else
#define WANT_RTLD_DEEPBIND RTLD_DEEPBIND
#endif

typedef int (*s4bxi_entry_point_type)(int argc, char** argv);
typedef void (*cpp_platform_callback)();

vector<string> privatize_libs_paths;
off_t fdin_size;

string user_app_name;
string executable;
string simulation_rand_id = "0000000000";

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
    char* libname    = (char*)(data);
    const char* path = info->dlpi_name;
    if (strstr(path, libname)) {
        strncpy(libname, path, 512);
        return 1;
    }

    return 0;
}

static s4bxi_entry_point_type s4bxi_resolve_function(void* handle)
{
    s4bxi_entry_point_type entry_point = (s4bxi_entry_point_type)dlsym(handle, "main");
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
            void* libhandle = dlopen(libname.c_str(), RTLD_LAZY);
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
    static size_t rank = 0;
    // Copy the dynamic library:
    string target_executable = executable + "_" + simulation_rand_id + "_" + to_string(rank) + ".so";

    s4bxi_copy_file(executable, target_executable, fdin_size);
    // if s4bxi/privatize-libs is set, duplicate pointed lib and link each
    // executable copy to a different one.
    vector<string> target_libs;
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
            string target_lib =
                simulation_rand_id.substr(0, pad - to_string(rank).length()) + to_string(rank) + libname.substr(pad);
            target_libs.push_back(target_lib);
            XBT_DEBUG("copy lib %s to %s, with size %lld", libpath.c_str(), target_lib.c_str(), (long long)fdin_size2);
            s4bxi_copy_file(libpath, target_lib, fdin_size2);

            string sedcommand = "sed -i -e 's/" + libname + "/" + target_lib + "/g' " + target_executable;
            XBT_DEBUG("Relinking user code:\n → %s\n", sedcommand.c_str());
            int rc = system(sedcommand.c_str());
            xbt_assert(rc == 0, "error while applying sed command %s \n", sedcommand.c_str());
        }
    }

    // <custom-things>
    // S4BXI addition : also privatize privatized libs (not just the user code)
    // Actually that doesn't work for MPI because some libs are never linked but dlopened at runtime,
    // ~~I don't really have a solution to this problem~~
    // → Actually it works if OMPI is configured with --disable-dlopen (so that everything is in the
    // three libs OMPI, ORTE and OPAL)

    for (auto const& relink_me : target_libs) {
        // This inner loop is stupid: we should cache the names instead of re-computing them, but for now it will do
        for (auto const& libpath : privatize_libs_paths) {
            size_t index = libpath.find_last_of("/\\");
            string libname;
            if (index != string::npos)
                libname = libpath.substr(index + 1);

            if (not libname.empty()) {
                unsigned int pad = 7;
                if (libname.length() < pad)
                    pad = libname.length();
                string new_lib_name = simulation_rand_id.substr(0, pad - to_string(rank).length()) + to_string(rank) +
                                      libname.substr(pad);

                string sedcommand = "sed -i -e 's/" + libname + "/" + new_lib_name + "/g' " + relink_me;
                XBT_DEBUG("Relinking privatized lib:\n → %s\n", sedcommand.c_str());
                int rc = system(sedcommand.c_str());
                xbt_assert(rc == 0, "error while applying sed command %s \n", sedcommand.c_str());
            }
        }
    }

    // </custom-things>

    my_rank = rank++;

    // Load the copy and resolve the entry point:
    void* handle    = dlopen(target_executable.c_str(), RTLD_LAZY | RTLD_LOCAL | WANT_RTLD_DEEPBIND);
    int saved_errno = errno;
    if (S4BXI_GLOBAL_CONFIG(keep_temps) == false) {
        unlink(target_executable.c_str());
        for (const string& target_lib : target_libs)
            unlink(target_lib.c_str());
    }
    xbt_assert(handle != nullptr, "dlopen failed: %s (errno: %d -- %s)", dlerror(), saved_errno, strerror(saved_errno));

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

    const char* prop = self->get_property("delay_start");
    if (prop)
        s4u::this_actor::sleep_for(atof(prop));

    s4bxi_bench_begin(this);

    entry_point(argc, argv);

    s4bxi_bench_end(this);

    for (char* s : args2str)
        xbt_free(s);
    delete args4argv;

    if (!S4BXI_GLOBAL_CONFIG(no_dlclose))
        dlclose(handle);
}

int s4bxi_default_main(int argc, char* argv[])
{
    s4bxi_actor_ext_plugin_init();
    simgrid::s4u::Engine e(&argc, argv);
    xbt_assert(argc > 4, "Usage: %s platform_file deployment_file user_app_path user_app_name\n", argv[0]);

    e.set_config("surf/precision:1e-9");
    e.set_config("network/loopback-lat:0");
    e.set_config("network/loopback-bw:999e9");
    e.set_config("network/model:CM02");

    char random_characters[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
                                'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
                                'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
    srand(getpid());
    for (int i = 0; i < simulation_rand_id.length(); i++)
        simulation_rand_id[i] = random_characters[rand() % 36];

    executable    = string(argv[3]);
    user_app_name = string(argv[4]);
    s4bxi_init_privatization_dlopen(executable);

    smpi_init_options();

    simgrid::smpi::colls::set_collectives();
    simgrid::smpi::colls::smpi_coll_cleanup_callback = nullptr;
    SMPI_init();

    /* Register the classes representing the actors */
    e.register_actor<BxiNicInitiator>("nic_initiator");
    e.register_actor<BxiNicTarget>("nic_target");
    e.register_actor<BxiNicE2E>("nic_e2e");
    e.register_actor<BxiUserAppActor>("user_app");

    /* Load the platform description and then deploy the application */
    string platf = argv[1];
    string xml   = ".xml";
    // Check if file ends with ".xml"
    if ((0 == platf.compare(platf.length() - xml.length(), xml.length(), xml))) {
        e.load_platform(argv[1]);
    } else { // If it doesn't then it's a C++ platform
        auto lib = dlopen(platf.c_str(), RTLD_LAZY);
        auto sym = (cpp_platform_callback)dlsym(lib, "make_platform");
        sym();
        if (!S4BXI_GLOBAL_CONFIG(no_dlclose))
            dlclose(lib);
    }
    e.load_deployment(argv[2]);

    int rank_counts = 0;
    for (auto actor : e.get_all_actors())
        if (actor->get_name() == "user_app")
            rank_counts++;

    SMPI_app_instance_register(smpi_default_instance_name.c_str(), nullptr, rank_counts);

    /* Run the simulation */
    e.run();

    SMPI_finalize();

    BxiEngine::get_instance()->end_simulation();

    return 0;
}