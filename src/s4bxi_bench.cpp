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

#include "s4bxi/actors/BxiMainActor.hpp"
#include "s4bxi/s4bxi_bench.h"
#include "s4bxi/s4bxi_xbt_log.h"
#include "s4bxi/s4bxi_util.hpp"
#include "simgrid/s4u/Exec.hpp"

using namespace simgrid;

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_bench, "Logging specific to benchmarking");

void s4bxi_force_execute(double duration)
{
    static double cpu_factor = S4BXI_GLOBAL_CONFIG(cpu_factor);

    auto nid = GET_CURRENT_MAIN_ACTOR->getNid();
    S4BXI_STARTLOG(S4BXILOG_COMPUTE, nid, nid)
    s4u::this_actor::exec_init(duration * cpu_factor * s4u::Actor::self()->get_host()->get_speed())
        ->set_name("computation")
        ->start()
        ->wait();
    S4BXI_WRITELOG()
}

// If we have SMPI functions available, use them
#ifdef BUILD_MPI_MIDDLEWARE
#include <smpi/smpi.h>

BxiLog __bxi_log;

void s4bxi_execute(double duration)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    auto nid                 = main_actor->getNid();
    S4BXI_STARTLOG(S4BXILOG_COMPUTE, nid, nid)
    smpi_execute(duration);
    S4BXI_WRITELOG()
}

void s4bxi_bench_begin()
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    main_actor->bench_number++;

    if (main_actor->bench_number != 1)
        return;

    ptl_nid_t nid       = GET_CURRENT_MAIN_ACTOR->getNid();
    int __bxi_log_level = S4BXI_GLOBAL_CONFIG(log_level);
    bool __bxi_must_log = __bxi_log_level && S4BXI_GLOBAL_CONFIG(log_computation);
    if (__bxi_must_log) {
        __bxi_log.start     = simgrid::s4u::Engine::get_clock();
        __bxi_log.type      = S4BXILOG_COMPUTE;
        __bxi_log.initiator = nid;
        __bxi_log.target    = nid;
    }
    smpi_bench_begin();
}

void s4bxi_bench_end()
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    main_actor->bench_number--;

    if (main_actor->bench_number != 0)
        return;

    smpi_bench_end();
    int __bxi_log_level = S4BXI_GLOBAL_CONFIG(log_level);
    bool __bxi_must_log = __bxi_log_level && S4BXI_GLOBAL_CONFIG(log_computation);
    if (__bxi_must_log) {
        __bxi_log.end = simgrid::s4u::Engine::get_clock();
        BxiEngine::get_instance()->log(__bxi_log);
    }
}

// Otherwise fall back to copy/pasting something very similar
#else

#include <xbt.h>

void s4bxi_execute(double duration)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    static double cpu_factor    = S4BXI_GLOBAL_CONFIG(cpu_factor);
    static double cpu_threshold = S4BXI_GLOBAL_CONFIG(cpu_threshold);

    double simulated_duration = main_actor->cpu_accumulator + duration * cpu_factor;

    if (simulated_duration >= cpu_threshold) {
        auto nid = main_actor->getNid();
        S4BXI_STARTLOG(S4BXILOG_COMPUTE, nid, nid)
        s4u::this_actor::exec_init(simulated_duration * s4u::Actor::self()->get_host()->get_speed())
            ->set_name("computation")
            ->start()
            ->wait();
        S4BXI_WRITELOG()
        main_actor->cpu_accumulator = 0;
    } else if (S4BXI_GLOBAL_CONFIG(cpu_accumulate)) {
        main_actor->cpu_accumulator = simulated_duration;
    }
}

void s4bxi_bench_begin()
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    main_actor->bench_number++;

    if (main_actor->bench_number != 1)
        return;

    xbt_os_threadtimer_start(main_actor->timer);
}

void s4bxi_bench_end()
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    main_actor->bench_number--;

    if (main_actor->bench_number != 0)
        return;

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
    s4bxi_execute(xbt_os_timer_elapsed(timer));
}

#endif