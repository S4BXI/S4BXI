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

#ifndef S4BXI_S4BXI_SAMPLE_H
#define S4BXI_S4BXI_SAMPLE_H

#include <xbt/base.h>

#ifdef __cplusplus
extern "C" {
#endif

void s4bxi_sample_1(int global, const char* file, int line, int iters, double threshold);
int s4bxi_sample_2(int global, const char* file, int line, int iter_count);
void s4bxi_sample_3(int global, const char* file, int line);
int s4bxi_sample_exit(int global, const char* file, int line, int iter_count);

#define S4BXI_ITER_NAME1(line) _XBT_CONCAT(iter_count, line)
#define S4BXI_ITER_NAME(line)  S4BXI_ITER_NAME1(line)
#define S4BXI_SAMPLE_LOOP(loop_init, loop_end, loop_iter, global, iters, thres)                                        \
    int S4BXI_ITER_NAME(__LINE__) = 0;                                                                                 \
    {                                                                                                                  \
        loop_init;                                                                                                     \
        while (loop_end) {                                                                                             \
            S4BXI_ITER_NAME(__LINE__)++;                                                                               \
            (loop_iter);                                                                                               \
        }                                                                                                              \
    }                                                                                                                  \
    for (loop_init; (loop_end) ? (s4bxi_sample_1((global), __FILE__, __LINE__, (iters), (thres)),                      \
                                  (s4bxi_sample_2((global), __FILE__, __LINE__, S4BXI_ITER_NAME(__LINE__))))           \
                               : s4bxi_sample_exit((global), __FILE__, __LINE__, S4BXI_ITER_NAME(__LINE__));           \
         s4bxi_sample_3((global), __FILE__, __LINE__), (loop_iter))
#define S4BXI_SAMPLE_LOCAL(loop_init, loop_end, loop_iter, iters, thres)                                               \
    S4BXI_SAMPLE_LOOP(loop_init, (loop_end), (loop_iter), 0, (iters), (thres))
#define S4BXI_SAMPLE_GLOBAL(loop_init, loop_end, loop_iter, iters, thres)                                              \
    S4BXI_SAMPLE_LOOP(loop_init, (loop_end), (loop_iter), 1, (iters), (thres))
#define S4BXI_SAMPLE_DELAY(duration) for (s4bxi_execute(duration); 0;)
#define S4BXI_SAMPLE_FLOPS(flops)    for (s4bxi_execute_flops(flops); 0;)

#ifdef __cplusplus
}
#endif

#endif // S4BXI_S4BXI_SAMPLE_H
