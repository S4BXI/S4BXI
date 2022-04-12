/*
 * Author: Julien EMMANUEL
 * Copyright (C) 2019-2022 Bull S.A.S
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

#ifndef S4BXI_BXIUSERAPPACTOR_HPP
#define S4BXI_BXIUSERAPPACTOR_HPP

#include <string>
#include <vector>

#include "BxiMainActor.hpp"

class BxiUserAppActor : public BxiMainActor {
    std::vector<std::string> string_args;
    std::string bull_mpi_lib = "";

  public:
    uint32_t my_rank;
    uint32_t my_local_rank;
    double last_model_change = 0;

    explicit BxiUserAppActor(const std::vector<std::string>& args);

    void operator()();
};

int s4bxi_default_main(int argc, char* argv[]);

#endif