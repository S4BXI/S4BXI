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

#include <simgrid/s4u.hpp>
#include <cmath>
#include <cstdio>
#include "s4bxi/actors/BxiMainActor.hpp"
#include "s4bxi/s4bxi_redefine.h"
#include "s4bxi/s4bxi_xbt_log.h"
#include "s4bxi/s4bxi_bench.h"
#include "s4bxi/s4bxi_util.hpp"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_redefine, "Messages generated in redefined functions");

using namespace std;
using namespace simgrid;

int s4bxi_gettimeofday(struct timeval* tv)
{
    s4bxi_bench_end();

    double simtime = s4u::Engine::get_clock();
    tv->tv_sec     = floor(simtime);
    tv->tv_usec    = round((simtime - tv->tv_sec) * 1e6);

    s4bxi_bench_begin();

    return 0;
}

unsigned int s4bxi_sleep(unsigned int seconds)
{
    s4bxi_bench_end();
    s4u::this_actor::sleep_for(seconds);
    s4bxi_bench_begin();

    return 0;
}

int s4bxi_usleep(useconds_t usec)
{
    s4bxi_bench_end();
    s4u::this_actor::sleep_for(1e-6 * usec);
    s4bxi_bench_begin();

    return 0;
}

int s4bxi_nanosleep(const struct timespec* req, struct timespec* rem)
{
    s4bxi_bench_end();
    s4u::this_actor::sleep_for(req->tv_sec + 1e-9 * req->tv_nsec);
    s4bxi_bench_begin();

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
    auto main_actor = BxiEngine::get_instance()->get_main_actor(stoi(args[2]));
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
    s4bxi_bench_end();

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
        s4bxi_bench_begin();

        return -1;
    }

    vector<string> args;
    args.push_back(to_string(__new->it_value.tv_sec + 1e-6 * __new->it_value.tv_usec));
    args.push_back(to_string(which));
    args.push_back(to_string(s4u::this_actor::get_pid()));

    s4u::Actor::create("_setitimer_actor", s4u::Host::current(), &s4bxi_timer_actor, args);

    s4bxi_bench_begin();

    return 0;
}

/**
 * Incomplete, the return value is possibly incorrect
 */
unsigned int s4bxi_alarm(unsigned int __seconds) __THROW
{
    s4bxi_bench_end();

    vector<string> args;
    args.push_back(to_string(__seconds));
    args.push_back(to_string(SIGALRM));
    args.push_back(to_string(s4u::this_actor::get_pid()));

    s4u::Actor::create("_alarm_actor", s4u::Host::current(), &s4bxi_timer_actor, args);

    s4bxi_bench_begin();

    return 0;
}

int s4bxi_gethostname(char* __name, size_t __len)
{
    strncpy(__name, GET_CURRENT_MAIN_ACTOR->getSlug().c_str(), __len);

    return 0;
}

int s4bxi_clock_gettime(clockid_t __clock_id, struct timespec* __tp) __THROW
{
    s4bxi_bench_end();

    double simtime = s4u::Engine::get_clock();
    __tp->tv_sec   = floor(simtime);
    __tp->tv_nsec  = round((simtime - __tp->tv_sec) * 1e9);

    s4bxi_bench_begin();

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

int s4bxi_getopt_long_only(int argc, char* const* argv, const char* options, const struct option* long_options,
                           int* opt_index)
{
    auto main_actor    = GET_CURRENT_MAIN_ACTOR;
    optind             = main_actor->optind;
    int ret            = getopt_long_only(argc, argv, options, long_options, opt_index);
    main_actor->optind = optind;

    return ret;
}

int s4bxi_getopt_long(int argc, char* const* argv, const char* options, const struct option* long_options,
                      int* opt_index)
{
    auto main_actor    = GET_CURRENT_MAIN_ACTOR;
    optind             = main_actor->optind;
    int ret            = getopt_long(argc, argv, options, long_options, opt_index);
    main_actor->optind = optind;

    return ret;
}

int s4bxi_getopt(int argc, char* const* argv, const char* options)
{
    auto main_actor    = GET_CURRENT_MAIN_ACTOR;
    optind             = main_actor->optind;
    int ret            = getopt(argc, argv, options);
    main_actor->optind = optind;

    return ret;
}

void s4bxi_exit(int code) __THROW
{
    s4bxi_bench_end();
    if (code)
        XBT_WARN("User code exited with code %d", code);
    else
        XBT_INFO("User code exited gracefully");

    s4u::this_actor::exit();
}

pid_t s4bxi_getpid(void) {
    return s4u::this_actor::get_pid();
}