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

S4BXI_LOG_NEW_DEFAULT_CATEGORY(bxi_engine, "Messages specific to BXI engine");

#define LOG_IN_FILE 10000

using namespace simgrid;

BxiEngine* BxiEngine::instance = nullptr;

#define LOG_STRING_CONFIG(x) XBT_DEBUG("%s: %s", #x, config->x.c_str())
#define LOG_CONFIG(x)        XBT_DEBUG("%s: %s", #x, to_string(config->x).c_str())

BxiEngine::BxiEngine()
{
    config = new s4bxi_config;

    config->max_retries        = get_int_s4bxi_param("MAX_RETRIES", 5);
    config->retry_timeout      = get_double_s4bxi_param("RETRY_TIMEOUT", 10.0F);
    config->model_pci_commands = get_bool_s4bxi_param("MODEL_PCI_COMMANDS", true);
    config->e2e_off            = get_bool_s4bxi_param("E2E_OFF", false);
    config->log_folder         = get_string_s4bxi_param("LOG_FOLDER", "/dev/null");
    config->log_level          = config->log_folder == "/dev/null" ? 0 : 1;
    config->privatize_libs     = get_string_s4bxi_param("PRIVATIZE_LIBS", "");
    config->keep_temps         = get_bool_s4bxi_param("KEEP_TEMPS", false);
    config->max_memcpy         = get_long_s4bxi_param("MAX_MEMCPY", -1);
    config->cpu_factor         = get_double_s4bxi_param("CPU_FACTOR", 1.0F);
    config->cpu_threshold      = get_double_s4bxi_param("CPU_FACTOR", 1e-7);
    config->quick_acks         = get_bool_s4bxi_param("QUICK_ACKS", false);

    XBT_DEBUG("Engine was configured with:");
    LOG_CONFIG(max_retries);
    LOG_CONFIG(retry_timeout);
    LOG_CONFIG(model_pci_commands);
    LOG_CONFIG(e2e_off);
    LOG_STRING_CONFIG(log_folder);
    LOG_CONFIG(log_level);
    LOG_STRING_CONFIG(privatize_libs);
    LOG_CONFIG(keep_temps);
    LOG_CONFIG(max_memcpy);
    LOG_CONFIG(cpu_factor);
    LOG_CONFIG(quick_acks);
}

BxiEngine::~BxiEngine()
{
    end_simulation();
}

void BxiEngine::end_simulation()
{
    if (config->log_level)
        logFile.close();

    delete config;
    for (auto pair : nodes)
        delete pair.second;
    nodes.clear();

    free_mailbox_pool();
}

BxiNode* BxiEngine::get_node(int nid)
{
    auto nodeIt = nodes.find(nid);
    BxiNode* node;

    if (nodeIt == nodes.end()) {
        node = new BxiNode(nid);
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

void BxiEngine::log(BxiLog& log)
{
    bool newFile = false;
    if (logCount == 0) {
        newFile = true;
        logFile.open(config->log_folder + "/log_0.csv");
    } else if (logCount % LOG_IN_FILE == 0) {
        newFile = true;
        logFile.close();
        logFile.open(config->log_folder + "/log_" + to_string((int)floor(logCount / LOG_IN_FILE)) + ".csv");
    }

    if (newFile)
        logFile << "operation_type,initiator_nid,target_nid,start_time,end_time" << endl;

    logFile << log << endl;
    ++logCount;
}