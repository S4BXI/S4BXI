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

#ifndef S4BXI_S4BXI_SHARED_H
#define S4BXI_S4BXI_SHARED_H

#ifdef BUILD_MPI_MIDDLEWARE

#include <unistd.h>

#ifdef __cplusplus

#ifdef COMPILING_SIMULATOR
// This file is from SMPI
#include "private.hpp"
#endif

extern "C" {

#endif

extern char* s4bxi_data_exe_start; // start of the data+bss segment of the executable
extern int s4bxi_data_exe_size;    // size of the data+bss segment of the executable

void s4bxi_shared_destroy();

unsigned char* s4bxi_get_tmp_sendbuffer(size_t size);
unsigned char* s4bxi_get_tmp_recvbuffer(size_t size);
void s4bxi_free_tmp_buffer(const unsigned char* buf);
void s4bxi_free_replay_tmp_buffers();

int s4bxi_temp_shm_get();
void* s4bxi_temp_shm_mmap(int fd, size_t size);

void* s4bxi_shared_malloc_intercept(size_t size, const char* file, int line);
void* s4bxi_shared_calloc_intercept(size_t num_elm, size_t elem_size, const char* file, int line);
void s4bxi_shared_free(void* data);

void* s4bxi_shared_malloc(size_t size, const char* file, int line);
#define S4BXI_SHARED_MALLOC(size) s4bxi_shared_malloc((size), __FILE__, __LINE__)
void* s4bxi_shared_malloc_partial(size_t size, const size_t* shared_block_offsets, int nb_shared_blocks);
#define S4BXI_PARTIAL_SHARED_MALLOC(size, shared_block_offsets, nb_shared_blocks)                                      \
    s4bxi_shared_malloc_partial((size), (shared_block_offsets), (nb_shared_blocks))

#define S4BXI_SHARED_FREE(data) s4bxi_shared_free(data)

int s4bxi_shared_known_call(const char* func, const char* input);
void* s4bxi_shared_get_call(const char* func, const char* input);
void* s4bxi_shared_set_call(const char* func, const char* input, void* data);
#define S4BXI_SHARED_CALL(func, input, ...)                                                                            \
    (s4bxi_shared_known_call(_XBT_STRINGIFY(func), (input))                                                            \
         ? s4bxi_shared_get_call(_XBT_STRINGIFY(func), (input))                                                        \
         : s4bxi_shared_set_call(_XBT_STRINGIFY(func), (input), ((func)(__VA_ARGS__))))

#ifdef __cplusplus
}
#endif

#else

#define S4BXI_SHARED_MALLOC(size)                                                 malloc(size)
#define S4BXI_PARTIAL_SHARED_MALLOC(size, shared_block_offsets, nb_shared_blocks) malloc(size)
#define S4BXI_SHARED_FREE(data)                                                   free(data)
#define S4BXI_SHARED_CALL(func, input, ...)                                       ((func)(__VA_ARGS__))

#endif

#endif // S4BXI_S4BXI_SHARED_H
