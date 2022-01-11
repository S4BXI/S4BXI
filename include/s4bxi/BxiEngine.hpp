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

#ifndef S4BXI_BxiEngine_HPP
#define S4BXI_BxiEngine_HPP

#include <map>
#include <set>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>

#include "BxiNode.hpp"
#include "BxiLog.hpp"
#include "s4bxi_config.hpp"
#include "s4bxi_util.hpp"

class BxiMainActor;

class BxiEngine {
    static BxiEngine* instance;
    std::map<int, std::shared_ptr<BxiNode>> nodes;
    std::map<aid_t, BxiMainActor*> actors;
    std::shared_ptr<s4bxi_config> config;
    unsigned long logCount = 0;
    std::ofstream logFile;
    std::string simulation_rand_id = "0000000000";

    BxiEngine();

    // Config
    inline char* get_s4bxi_param(const std::string& name);
    double get_double_s4bxi_param(const std::string& name, double default_val);
    int get_int_s4bxi_param(const std::string& name, int default_val);
    long get_long_s4bxi_param(const std::string& name, int default_val);
    bool get_bool_s4bxi_param(const std::string& name, bool default_val);
    std::string get_string_s4bxi_param(const std::string& name, std::string default_val);

  public:
    static BxiEngine* get_instance()
    {
        if (!instance)
            instance = new BxiEngine;

        return instance;
    }

    static std::shared_ptr<s4bxi_config> get_config();

    std::string get_simulation_rand_id();
    void set_simulation_rand_id(std::string id);
    std::shared_ptr<BxiNode> get_node(int);
    void log(const BxiLog& log);
    void end_simulation();
    void register_main_actor(BxiMainActor*);
    std::map<aid_t, BxiMainActor*> main_actors_on_host(const std::string& hostname);
    std::set<std::string, std::less<>> used_nodes();
    BxiMainActor* get_current_main_actor();
    BxiMainActor* get_main_actor(aid_t pid);
    int get_main_actor_count();
    BxiMainActor* get_actor_from_rank(int rank);
    BxiMainActor* get_actor_from_slug_and_localrank(const std::string& slug, int localrank);
};

#endif // S4BXI_BxiEngine_HPP