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
#include "simgrid/s4u/Exec.hpp"
#include "s4bxi/s4bxi_xbt_log.h"
#include "s4bxi/s4bxi_util.hpp"
#include <xbt.h>

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_bench, "Logging specific to benchmarking");

using namespace simgrid;

void s4bxi_execute(BxiMainActor* main_actor, double duration)
{
    static double cpu_factor    = S4BXI_GLOBAL_CONFIG(cpu_factor);
    static double cpu_threshold = S4BXI_GLOBAL_CONFIG(cpu_threshold);

    double simulated_duration = main_actor->cpu_accumulator + duration * cpu_factor;

    if (simulated_duration >= cpu_threshold) {
        auto nid = main_actor->getNid();
        S4BXI_STARTLOG(S4BXILOG_COMPUTE, nid, nid)
        XBT_DEBUG("Sleep for %g to handle real computation time", duration);
        s4u::this_actor::exec_init(simulated_duration * s4u::Actor::self()->get_host()->get_speed())
            ->set_name("computation")
            ->start()
            ->wait();
        S4BXI_WRITELOG()
        main_actor->cpu_accumulator = 0;
    } else if (S4BXI_GLOBAL_CONFIG(cpu_accumulate)) {
        main_actor->cpu_accumulator = simulated_duration;
        XBT_DEBUG("Storing %g in CPU accumulator for next execution", simulated_duration);
    } else {
        XBT_DEBUG("Real computation took %g while ignore threshold is set to %g => ignore it", simulated_duration,
                  cpu_threshold);
    }
}

void s4bxi_bench_begin(BxiMainActor* main_actor)
{
    xbt_os_threadtimer_start(main_actor->timer);
}

void s4bxi_bench_end(BxiMainActor* main_actor)
{
    if (main_actor->sampling()) {
        XBT_CRITICAL("Cannot do recursive benchmarks.");
        XBT_CRITICAL("Are you trying to make a call to MPI within an S4BXI_SAMPLE_ block?");
        xbt_backtrace_display_current();
        xbt_die("Aborting.");
    }

    xbt_os_timer_t timer = main_actor->timer;
    xbt_os_threadtimer_stop(timer);

    // Maybe we need to artificially speed up or slow down our computation based on our statistical analysis.
    // Simulate the benchmarked computation unless disabled via command-line argument
    s4bxi_execute(main_actor, xbt_os_timer_elapsed(timer));
}
