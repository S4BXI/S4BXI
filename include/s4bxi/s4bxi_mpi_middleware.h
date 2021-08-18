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

#ifndef S4BXI_MPI_MIDDLEWARE_H
#define S4BXI_MPI_MIDDLEWARE_H

// Inside Bull's OMPI there are already all the type definition we need, steal them in other cases only
#ifndef OMPI_MPI_H
#include <smpi/smpi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct s4bxi_mpi_ops {
    void* Gather;
};

void set_mpi_middleware_ops(void* bull_libhandle, void* smpi_libhandle);

int S4BXI_MPI_Gather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                     MPI_Datatype recvtype, int root, MPI_Comm comm);

#ifdef __cplusplus
}
#endif

#endif // S4BXI_MPI_MIDDLEWARE_H

#ifndef COMPILING_SIMULATOR

#define MPI_Gather(a, b, c, d, e, f, g, h) S4BXI_MPI_Gather(a, b, c, d, e, f, g, h)

#endif
