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

#ifndef S4BXI_REDEFINE_H
#define S4BXI_REDEFINE_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

int s4bxi_gettimeofday(struct timeval* tv);

unsigned int s4bxi_sleep(unsigned int seconds);

int s4bxi_usleep(useconds_t usec);

int s4bxi_nanosleep(const struct timespec* req, struct timespec* rem);

int s4bxi_sigaction(int signum, const struct sigaction* act, struct sigaction* oldact);

int s4bxi_setitimer(int __which, const struct itimerval* __new, struct itimerval* __old);

unsigned int s4bxi_alarm(unsigned int __seconds) __THROW;

int s4bxi_gethostname(char* __name, size_t __len);

int s4bxi_clock_gettime(clockid_t __clock_id, struct timespec* __tp) __THROW;

int s4bxi_clock_getres(clockid_t __clock_id, struct timespec* __tp) __THROW;

#define gettimeofday(x, y)  s4bxi_gettimeofday(x)
#define sleep(x)            s4bxi_sleep(x)
#define usleep(x)           s4bxi_usleep(x)
#define nanosleep(x, y)     s4bxi_nanosleep(x, y)
#define sigaction(x, y, z)  s4bxi_sigaction(x, y, z)
#define setitimer(x, y, z)  s4bxi_setitimer(x, y, z)
#define alarm(x)            s4bxi_alarm(x)
#define gethostname(x, y)   s4bxi_gethostname(x, y)
#define clock_gettime(x, y) s4bxi_clock_gettime(x, y)
#define clock_getres(x, y)  s4bxi_clock_getres(x, y)

#ifdef __cplusplus
}
#endif

#endif
