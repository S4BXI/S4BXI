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

#include "s4bxi/s4bxi_bench.hpp"
#include <smpi/smpi.h>
#include <private.hpp>

void s4bxi_execute(BxiMainActor* main_actor, double duration)
{
    smpi_execute(duration);
}

void s4bxi_bench_begin(BxiMainActor* main_actor)
{
    smpi_bench_begin();
}

void s4bxi_bench_end(BxiMainActor* main_actor)
{
    smpi_bench_end();
}
