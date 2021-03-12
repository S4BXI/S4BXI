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

#include <simgrid/s4u.hpp>
#include <cmath>
#include <cstdio>
#include "s4bxi/s4bxi_redefine.h"
#include "s4bxi/s4bxi_xbt_log.h"
#include "s4bxi/s4bxi_bench.hpp"
#include "s4bxi/s4bxi_util.hpp"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_redefine, "Messages generated in redefined functions");

using namespace std;
using namespace simgrid;

int s4bxi_gettimeofday(struct timeval* tv)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);

    double simtime = s4u::Engine::get_clock();
    tv->tv_sec     = floor(simtime);
    tv->tv_usec    = round((simtime - tv->tv_sec) * 1e6);

    s4bxi_bench_begin(main_actor);

    return 0;
}

unsigned int s4bxi_sleep(unsigned int seconds)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);

    s4u::this_actor::sleep_for(seconds);

    s4bxi_bench_begin(main_actor);

    return 0;
}

int s4bxi_usleep(useconds_t usec)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);

    s4u::this_actor::sleep_for(1e-6 * usec);

    s4bxi_bench_begin(main_actor);

    return 0;
}

int s4bxi_nanosleep(const struct timespec* req, struct timespec* rem)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);

    s4u::this_actor::sleep_for(req->tv_sec + 1e-9 * req->tv_nsec);

    s4bxi_bench_begin(main_actor);

    return 0;
}

/**
 * args[0] = duration to wait (in seconds, double)
 * args[1] = type of signal to trigger (int)
 * args[2] = SimGrid PID of main actor
 */
void s4bxi_timer_actor(vector<string> args)
{
    s4u::this_actor::sleep_for(stod(args[0]));

    struct sigaction* action;
    auto main_actor = GET_MAIN_ACTOR(stoi(args[2]));
    if (!main_actor)
        return; // It has probably been killed, should be fine

    if ((action = main_actor->registered_signals[stoi(args[1])]))
        action->sa_handler(stoi(args[1]));
}

int s4bxi_sigaction(int signum, const struct sigaction* act, struct sigaction* oldact)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;

    if (signum > 32) // Signal number > 32 is erroneous
        return -1;

    if (oldact && main_actor->registered_signals[signum])
        *oldact = *main_actor->registered_signals[signum];

    if (!main_actor->registered_signals[signum])
        main_actor->registered_signals[signum] = new struct sigaction;

    if (act)                                            // NULL is legal
        *main_actor->registered_signals[signum] = *act; // Not sure if the copy is necessary, but probably

    return 0;
}

int s4bxi_setitimer(int __which, const struct itimerval* __new, struct itimerval* __old)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);

    int which;

    switch (__which) {
    case ITIMER_REAL:
        which = SIGALRM;
        break;
    case ITIMER_VIRTUAL:
        which = SIGVTALRM;
        break;
    case ITIMER_PROF:
        which = SIGPROF;
        break;

    default:
        s4bxi_bench_begin(main_actor);

        return -1;
    }

    vector<string> args;
    args.push_back(to_string(__new->it_value.tv_sec + 1e-6 * __new->it_value.tv_usec));
    args.push_back(to_string(which));
    args.push_back(to_string(s4u::this_actor::get_pid()));

    s4u::Actor::create("_setitimer_actor", s4u::Host::current(), &s4bxi_timer_actor, args);

    s4bxi_bench_begin(main_actor);

    return 0;
}

/**
 * Incomplete, the return value is possibly incorrect
 */
unsigned int s4bxi_alarm(unsigned int __seconds) __THROW
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);

    vector<string> args;
    args.push_back(to_string(__seconds));
    args.push_back(to_string(SIGALRM));
    args.push_back(to_string(s4u::this_actor::get_pid()));

    s4u::Actor::create("_alarm_actor", s4u::Host::current(), &s4bxi_timer_actor, args);

    s4bxi_bench_begin(main_actor);

    return 0;
}

int s4bxi_gethostname(char* __name, size_t __len)
{
    const char* name = GET_CURRENT_MAIN_ACTOR->getSlug().c_str();
    strncpy(__name, name, __len);

    return 0;
}

int s4bxi_clock_gettime(clockid_t __clock_id, struct timespec* __tp) __THROW
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);

    double simtime = s4u::Engine::get_clock();
    __tp->tv_sec   = floor(simtime);
    __tp->tv_nsec  = round((simtime - __tp->tv_sec) * 1e9);

    s4bxi_bench_begin(main_actor);

    return 0;
}

/**
 * I think ? Maybe ?
 */
int s4bxi_clock_getres(clockid_t __clock_id, struct timespec* __tp) __THROW
{
    __tp->tv_sec  = 0;
    __tp->tv_nsec = 1;

    return 0;
}