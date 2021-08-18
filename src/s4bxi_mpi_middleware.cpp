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

#include <dlfcn.h>
#include <fcntl.h>

#include "s4bxi/s4bxi_mpi_middleware.h"
#include "s4bxi/s4bxi_xbt_log.h"
#include "s4bxi/actors/BxiMainActor.hpp"
#include "s4bxi/s4bxi_util.hpp"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_mpi_middlware, "Messages generated in MPI middleware");

using namespace std;

typedef int (*gather_func)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                           MPI_Datatype recvtype, int root, MPI_Comm comm);

void set_mpi_middleware_ops(void* bull_libhandle, void* smpi_libhandle)
{
    BxiMainActor* main_actor        = GET_CURRENT_MAIN_ACTOR;
    main_actor->bull_mpi_ops.Gather = dlsym(bull_libhandle, "MPI_Gather");
    XBT_INFO("Setting up gather (address %p)", main_actor->bull_mpi_ops.Gather);
}

int S4BXI_MPI_Gather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                     MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    XBT_INFO("Middleware gather (address %p)", main_actor->bull_mpi_ops.Gather);
    return ((gather_func)main_actor->bull_mpi_ops.Gather)(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                                          root, comm);
}
