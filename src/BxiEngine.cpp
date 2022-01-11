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

#include "s4bxi/BxiEngine.hpp"
#include "s4bxi/actors/BxiUserAppActor.hpp"
#include "s4bxi/s4bxi_mailbox_pool.hpp"

#include <iomanip> // setprecision
#include <cmath>   // floor
#include <simgrid/s4u.hpp>

// This define thing is ugly, but I can't find an elegant way to deal with these log categories
#define MAKE_NEW_LOG_CATEGORY

#include "s4bxi/s4bxi_xbt_log.h"

using namespace std;
using namespace simgrid;

S4BXI_LOG_NEW_DEFAULT_CATEGORY(bxi_engine, "Messages specific to BXI engine");

#define LOGS_IN_FILE 10000

using namespace simgrid;

BxiEngine* BxiEngine::instance = nullptr;

#define LOG_STRING_CONFIG(x) XBT_DEBUG("%s: %s", #x, config->x.c_str())
#define LOG_CONFIG(x)        XBT_DEBUG("%s: %s", #x, to_string(config->x).c_str())

BxiEngine::BxiEngine()
{
    config = make_shared<s4bxi_config>();

    config->max_retries               = get_int_s4bxi_param("MAX_RETRIES", 5);
    config->retry_timeout             = get_double_s4bxi_param("RETRY_TIMEOUT", 10.0F);
    config->use_real_memory           = get_bool_s4bxi_param("USE_REAL_MEMORY", true);
    config->model_pci                 = get_bool_s4bxi_param("MODEL_PCI", true);
    config->model_pci_commands        = config->model_pci && get_bool_s4bxi_param("MODEL_PCI_COMMANDS", true);
    config->e2e_off                   = get_bool_s4bxi_param("E2E_OFF", false);
    config->log_folder                = get_string_s4bxi_param("LOG_FOLDER", "/dev/null");
    config->log_level                 = config->log_folder == "/dev/null" ? 0 : 1;
    config->privatize_libs            = get_string_s4bxi_param("PRIVATIZE_LIBS", "");
    config->keep_temps                = get_bool_s4bxi_param("KEEP_TEMPS", false);
    config->max_memcpy                = get_long_s4bxi_param("MAX_MEMCPY", -1);
    config->cpu_factor                = get_double_s4bxi_param("CPU_FACTOR", 1.0F);
    config->cpu_threshold             = get_double_s4bxi_param("CPU_THRESHOLD", 1e-7);
    config->cpu_accumulate            = get_bool_s4bxi_param("CPU_ACCUMULATE", false);
    config->quick_acks                = get_bool_s4bxi_param("QUICK_ACKS", false);
    config->auto_shared_malloc_thresh = get_double_s4bxi_param("SHARED_MALLOC_THRESH", 1.0);
    config->shared_malloc_hugepage    = get_string_s4bxi_param("SHARED_MALLOC_HUGEPAGE", "");
    config->shared_malloc_blocksize   = get_long_s4bxi_param("SHARED_MALLOC_BLOCKSIZE", 1048576);
    config->max_inflight_to_target    = get_int_s4bxi_param("MAX_INFLIGHT_TO_TARGET", 0);
    config->max_inflight_to_process   = get_int_s4bxi_param("MAX_INFLIGHT_TO_PROCESS", 0);
    config->no_dlclose                = get_bool_s4bxi_param("NO_DLCLOSE", false);
    config->use_pugixml               = get_bool_s4bxi_param("USE_PUGIXML", false);
    const string s                    = get_string_s4bxi_param("SHARED_MALLOC", "none");
    if (s == "local")
        config->shared_malloc = 1;
    else if (s == "global")
        config->shared_malloc = 2;
    else
        config->shared_malloc = 0;

    XBT_DEBUG("Engine was configured with:");
    LOG_CONFIG(max_retries);
    LOG_CONFIG(retry_timeout);
    LOG_CONFIG(use_real_memory);
    LOG_CONFIG(model_pci);
    LOG_CONFIG(model_pci_commands);
    LOG_CONFIG(e2e_off);
    LOG_STRING_CONFIG(log_folder);
    LOG_CONFIG(log_level);
    LOG_STRING_CONFIG(privatize_libs);
    LOG_CONFIG(keep_temps);
    LOG_CONFIG(max_memcpy);
    LOG_CONFIG(cpu_factor);
    LOG_CONFIG(quick_acks);
    LOG_CONFIG(auto_shared_malloc_thresh);
    LOG_STRING_CONFIG(shared_malloc_hugepage);
    LOG_CONFIG(shared_malloc);
    LOG_CONFIG(shared_malloc_blocksize);
    LOG_CONFIG(max_inflight_to_target);
    LOG_CONFIG(max_inflight_to_process);
    LOG_CONFIG(no_dlclose);
}

void BxiEngine::end_simulation()
{
    if (config->log_level)
        logFile.close();

    nodes.clear();

    free_mailbox_pool();

    delete instance;
}

string BxiEngine::get_simulation_rand_id()
{
    return simulation_rand_id;
}

void BxiEngine::set_simulation_rand_id(string id)
{
    simulation_rand_id = id;
}

shared_ptr<BxiNode> BxiEngine::get_node(int nid)
{
    auto nodeIt = nodes.find(nid);
    shared_ptr<BxiNode> node;

    if (nodeIt == nodes.end()) {
        node = make_shared<BxiNode>(nid);
        nodes.emplace(nid, node);

        // For some mysterious reason everything blocks if we initialize that in
        // BxiNode's constructor, but it's fine if we do it afterwards
        node->e2e_entries = s4u::Semaphore::create(MAX_E2E_ENTRIES);
    } else {
        node = nodeIt->second;
    }

    return node;
}

void BxiEngine::register_main_actor(BxiMainActor* actor)
{
    actors.emplace(s4u::Actor::self()->get_pid(), actor);
}

map<aid_t, BxiMainActor*> BxiEngine::main_actors_on_host(const string& hostname)
{
    map<aid_t, BxiMainActor*> on_host;
    auto pred = [hostname](pair<aid_t, BxiMainActor*> const& the_pair) {
        return the_pair.second->getSlug() == hostname;
    };
    copy_if(actors.begin(), actors.end(), inserter(on_host, on_host.end()), pred);

    return on_host;
}

set<string, less<>> BxiEngine::used_nodes()
{
    set<string, less<>> out;
    for (auto p : actors)
        out.emplace(p.second->getSlug());

    return out;
}

BxiMainActor* BxiEngine::get_current_main_actor()
{
    auto actor = get_main_actor(s4u::Actor::self()->get_pid());

    if (!actor)
        ptl_panic("Couldn't find current actor, has it been registered correctly ?");

    return actor;
}

BxiMainActor* BxiEngine::get_main_actor(aid_t pid)
{
    auto actorIt = actors.find(pid);

    return actorIt == actors.end() ? nullptr : actorIt->second;
}

BxiMainActor* BxiEngine::get_actor_from_rank(int rank)
{
    for (auto actorIt : actors)
        if (((BxiUserAppActor*)actorIt.second)->my_rank == rank)
            return actorIt.second;

    return nullptr;
}

BxiMainActor* BxiEngine::get_actor_from_slug_and_localrank(const string& slug, int localrank)
{
    for (auto actorIt : actors)
        if (actorIt.second->getSlug() == slug && ((BxiUserAppActor*)actorIt.second)->my_local_rank == localrank)
            return actorIt.second;

    return nullptr;
}

int BxiEngine::get_main_actor_count()
{
    return actors.size();
}

inline char* BxiEngine::get_s4bxi_param(const string& name)
{
    return getenv(("S4BXI_" + name).c_str());
}

double BxiEngine::get_double_s4bxi_param(const string& name, double default_val)
{
    char* env = get_s4bxi_param(name);

    return env ? atof(env) : default_val;
}

int BxiEngine::get_int_s4bxi_param(const string& name, int default_val)
{
    char* env = get_s4bxi_param(name);

    return env ? atoi(env) : default_val;
}

long BxiEngine::get_long_s4bxi_param(const string& name, int default_val)
{
    char* env = get_s4bxi_param(name);

    return env ? atol(env) : default_val;
}

bool BxiEngine::get_bool_s4bxi_param(const string& name, bool default_val)
{
    char* env = get_s4bxi_param(name);

    if (!env)
        return default_val;

    return TRUTHY_CHAR(env);
}

string BxiEngine::get_string_s4bxi_param(const string& name, string default_val)
{
    char* env = get_s4bxi_param(name);

    return env ? string(env) : default_val;
}

void BxiEngine::log(const BxiLog& log)
{
    bool newFile = false;
    if (logCount == 0) {
        newFile = true;
        logFile.open(config->log_folder + "/log_0.csv");
    } else if (logCount % LOGS_IN_FILE == 0) {
        newFile = true;
        logFile.close();
        logFile.open(config->log_folder + "/log_" + to_string((int)floor(logCount / LOGS_IN_FILE)) + ".csv");
    }

    if (newFile)
        logFile << "operation_type,initiator_nid,target_nid,start_time,end_time" << endl;

    logFile << log << endl;
    ++logCount;
}

shared_ptr<s4bxi_config> BxiEngine::get_config()
{
    return get_instance()->config;
}
