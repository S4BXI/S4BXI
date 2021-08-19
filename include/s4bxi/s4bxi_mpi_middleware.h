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
    void* Init;
    void* Finalize;
    void* Gather;
    void* Allgather;

    // Special communicators
    MPI_Comm COMM_WORLD;

    // Datatypes
    MPI_Datatype TYPE_DATATYPE_NULL;
    MPI_Datatype TYPE_CHAR;
    MPI_Datatype TYPE_SHORT;
    MPI_Datatype TYPE_INT;
    MPI_Datatype TYPE_LONG;
    MPI_Datatype TYPE_LONG_LONG;
    MPI_Datatype TYPE_SIGNED_CHAR;
    MPI_Datatype TYPE_UNSIGNED_CHAR;
    MPI_Datatype TYPE_UNSIGNED_SHORT;
    MPI_Datatype TYPE_UNSIGNED;
    MPI_Datatype TYPE_UNSIGNED_LONG;
    MPI_Datatype TYPE_UNSIGNED_LONG_LONG;
    MPI_Datatype TYPE_FLOAT;
    MPI_Datatype TYPE_DOUBLE;
    MPI_Datatype TYPE_LONG_DOUBLE;
    MPI_Datatype TYPE_WCHAR;
    MPI_Datatype TYPE_C_BOOL;
    MPI_Datatype TYPE_INT8_T;
    MPI_Datatype TYPE_INT16_T;
    MPI_Datatype TYPE_INT32_T;
    MPI_Datatype TYPE_INT64_T;
    MPI_Datatype TYPE_UINT8_T;
    MPI_Datatype TYPE_BYTE;
    MPI_Datatype TYPE_UINT16_T;
    MPI_Datatype TYPE_UINT32_T;
    MPI_Datatype TYPE_UINT64_T;
    MPI_Datatype TYPE_C_FLOAT_COMPLEX;
    MPI_Datatype TYPE_C_DOUBLE_COMPLEX;
    MPI_Datatype TYPE_C_LONG_DOUBLE_COMPLEX;
    MPI_Datatype TYPE_AINT;
    MPI_Datatype TYPE_OFFSET;
    MPI_Datatype TYPE_LB;
    MPI_Datatype TYPE_UB;
    // The following are datatypes for the MPI functions MPI_MAXLOC  and MPI_MINLOC.
    MPI_Datatype TYPE_FLOAT_INT;
    MPI_Datatype TYPE_LONG_INT;
    MPI_Datatype TYPE_DOUBLE_INT;
    MPI_Datatype TYPE_SHORT_INT;
    MPI_Datatype TYPE_2INT;
    MPI_Datatype TYPE_LONG_DOUBLE_INT;
    MPI_Datatype TYPE_2FLOAT;
    MPI_Datatype TYPE_2DOUBLE;
    MPI_Datatype TYPE_2LONG;
    // only for compatibility with Fortran
    MPI_Datatype TYPE_REAL;
    MPI_Datatype TYPE_REAL4;
    MPI_Datatype TYPE_REAL8;
    MPI_Datatype TYPE_REAL16;
    MPI_Datatype TYPE_COMPLEX8;
    MPI_Datatype TYPE_COMPLEX16;
    MPI_Datatype TYPE_COMPLEX32;
    MPI_Datatype TYPE_INTEGER1;
    MPI_Datatype TYPE_INTEGER2;
    MPI_Datatype TYPE_INTEGER4;
    MPI_Datatype TYPE_INTEGER8;
    MPI_Datatype TYPE_INTEGER16;
    MPI_Datatype TYPE_COUNT;
};

void set_mpi_middleware_ops(void* bull_libhandle, void* smpi_libhandle);

int S4BXI_MPI_Init(int* argc, char*** argv);

int S4BXI_MPI_Finalize(void);

int S4BXI_MPI_Gather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                     MPI_Datatype recvtype, int root, MPI_Comm comm);

int S4BXI_MPI_Allgather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                        MPI_Datatype recvtype, MPI_Comm comm);

#ifdef __cplusplus
}
#endif

#endif // S4BXI_MPI_MIDDLEWARE_H

#ifndef COMPILING_SIMULATOR

#define MPI_Init      S4BXI_MPI_Init
#define MPI_Finalize  S4BXI_MPI_Finalize
#define MPI_Gather    S4BXI_MPI_Gather
#define MPI_Allgather S4BXI_MPI_Allgather

#endif
