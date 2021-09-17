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

#ifndef S4BXI_S4BXI_BENCH_HPP
#define S4BXI_S4BXI_BENCH_HPP

#include "s4bxi/actors/BxiMainActor.hpp"

void s4bxi_execute(double duration);
void s4bxi_bench_begin();
void s4bxi_bench_end();

#endif // S4BXI_S4BXI_BENCH_HPP
