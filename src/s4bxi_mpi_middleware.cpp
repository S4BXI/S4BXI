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
#include <private.hpp>
#include <smpi_actor.hpp>

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_mpi_middlware, "Messages generated in MPI middleware");

using namespace std;

#define USE_SMPI 0

typedef int (*finalize_func)(void);
typedef int (*init_func)(int* argc, char*** argv);
typedef int (*gather_func)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                           MPI_Datatype recvtype, int root, MPI_Comm comm);
typedef int (*allgather_func)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                              MPI_Datatype recvtype, MPI_Comm comm);

#define SETUP_SYMBOLS_IN_IMPLEMS(symbol)                                                                               \
    main_actor->bull_mpi_ops.symbol = dlsym(bull_libhandle, "MPI_" #symbol);                                           \
    main_actor->smpi_mpi_ops.symbol = dlsym(smpi_libhandle, "MPI_" #symbol);                                           \
    XBT_INFO("Setting up " #symbol " (address %p, %p)", main_actor->bull_mpi_ops.symbol,                               \
             main_actor->smpi_mpi_ops.symbol);

#define SETUP_DATATYPES_IN_IMPLEMS(lowercase, uppercase)                                                               \
    main_actor->bull_mpi_ops.TYPE_##uppercase = (MPI_Datatype)dlsym(bull_libhandle, "ompi_mpi_" #lowercase);           \
    main_actor->smpi_mpi_ops.TYPE_##uppercase = (MPI_Datatype)dlsym(smpi_libhandle, "smpi_MPI_" #uppercase);           \
    XBT_INFO("Setting up TYPE_" #uppercase " (address %p, %p)", main_actor->bull_mpi_ops.TYPE_##uppercase,             \
             main_actor->smpi_mpi_ops.TYPE_##uppercase);

MPI_Datatype implem_datatype(MPI_Datatype original)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

#define MPI_TYPE_TRANSLATION(type)                                                                                     \
    if (original == MPI_##type || original == main_actor->bull_mpi_ops.TYPE_##type ||                                  \
        original == main_actor->smpi_mpi_ops.TYPE_##type)                                                              \
        return (USE_SMPI ? main_actor->smpi_mpi_ops : main_actor->bull_mpi_ops).TYPE_##type;

    MPI_TYPE_TRANSLATION(CHAR)
    MPI_TYPE_TRANSLATION(DATATYPE_NULL)
    MPI_TYPE_TRANSLATION(CHAR)
    MPI_TYPE_TRANSLATION(SHORT)
    MPI_TYPE_TRANSLATION(INT)
    MPI_TYPE_TRANSLATION(LONG)
    MPI_TYPE_TRANSLATION(LONG_LONG)
    MPI_TYPE_TRANSLATION(SIGNED_CHAR)
    MPI_TYPE_TRANSLATION(UNSIGNED_CHAR)
    MPI_TYPE_TRANSLATION(UNSIGNED_SHORT)
    MPI_TYPE_TRANSLATION(UNSIGNED)
    MPI_TYPE_TRANSLATION(UNSIGNED_LONG)
    MPI_TYPE_TRANSLATION(UNSIGNED_LONG_LONG)
    MPI_TYPE_TRANSLATION(FLOAT)
    MPI_TYPE_TRANSLATION(DOUBLE)
    MPI_TYPE_TRANSLATION(LONG_DOUBLE)
    MPI_TYPE_TRANSLATION(WCHAR)
    MPI_TYPE_TRANSLATION(C_BOOL)
    MPI_TYPE_TRANSLATION(INT8_T)
    MPI_TYPE_TRANSLATION(INT16_T)
    MPI_TYPE_TRANSLATION(INT32_T)
    MPI_TYPE_TRANSLATION(INT64_T)
    MPI_TYPE_TRANSLATION(UINT8_T)
    MPI_TYPE_TRANSLATION(BYTE)
    MPI_TYPE_TRANSLATION(UINT16_T)
    MPI_TYPE_TRANSLATION(UINT32_T)
    MPI_TYPE_TRANSLATION(UINT64_T)
    MPI_TYPE_TRANSLATION(C_FLOAT_COMPLEX)
    MPI_TYPE_TRANSLATION(C_DOUBLE_COMPLEX)
    MPI_TYPE_TRANSLATION(C_LONG_DOUBLE_COMPLEX)
    MPI_TYPE_TRANSLATION(AINT)
    MPI_TYPE_TRANSLATION(OFFSET)
    MPI_TYPE_TRANSLATION(LB)
    MPI_TYPE_TRANSLATION(UB)
    MPI_TYPE_TRANSLATION(FLOAT_INT)
    MPI_TYPE_TRANSLATION(LONG_INT)
    MPI_TYPE_TRANSLATION(DOUBLE_INT)
    MPI_TYPE_TRANSLATION(SHORT_INT)
    MPI_TYPE_TRANSLATION(2INT)
    MPI_TYPE_TRANSLATION(LONG_DOUBLE_INT)
    MPI_TYPE_TRANSLATION(2FLOAT)
    MPI_TYPE_TRANSLATION(2DOUBLE)
    MPI_TYPE_TRANSLATION(2LONG)
    MPI_TYPE_TRANSLATION(REAL)
    MPI_TYPE_TRANSLATION(REAL4)
    MPI_TYPE_TRANSLATION(REAL8)
    MPI_TYPE_TRANSLATION(REAL16)
    MPI_TYPE_TRANSLATION(COMPLEX8)
    MPI_TYPE_TRANSLATION(COMPLEX16)
    MPI_TYPE_TRANSLATION(COMPLEX32)
    MPI_TYPE_TRANSLATION(INTEGER1)
    MPI_TYPE_TRANSLATION(INTEGER2)
    MPI_TYPE_TRANSLATION(INTEGER4)
    MPI_TYPE_TRANSLATION(INTEGER8)
    MPI_TYPE_TRANSLATION(INTEGER16)
    MPI_TYPE_TRANSLATION(COUNT)

#undef MPI_TYPE_TRANSLATION

    return original;
}

MPI_Comm implem_comm(MPI_Comm original)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    if (original == MPI_COMM_WORLD || original == main_actor->bull_mpi_ops.COMM_WORLD ||
        original == main_actor->smpi_mpi_ops.COMM_WORLD)
        return (USE_SMPI ? smpi_process()->comm_world() : GET_CURRENT_MAIN_ACTOR->bull_mpi_ops.COMM_WORLD);
}

void set_mpi_middleware_ops(void* bull_libhandle, void* smpi_libhandle)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    SETUP_SYMBOLS_IN_IMPLEMS(Init)
    SETUP_SYMBOLS_IN_IMPLEMS(Finalize)
    SETUP_SYMBOLS_IN_IMPLEMS(Gather)
    SETUP_SYMBOLS_IN_IMPLEMS(Allgather)

    main_actor->bull_mpi_ops.COMM_WORLD = (MPI_Comm)dlsym(bull_libhandle, "ompi_mpi_comm_world");
    main_actor->smpi_mpi_ops.COMM_WORLD = nullptr;
    XBT_INFO("Setting up COMM_WORLD (address %p, %p)", main_actor->bull_mpi_ops.COMM_WORLD,
             main_actor->smpi_mpi_ops.COMM_WORLD);

    // Datatypes
    SETUP_DATATYPES_IN_IMPLEMS(char, CHAR)
    SETUP_DATATYPES_IN_IMPLEMS(datatype_null, DATATYPE_NULL)
    SETUP_DATATYPES_IN_IMPLEMS(char, CHAR)
    SETUP_DATATYPES_IN_IMPLEMS(short, SHORT)
    SETUP_DATATYPES_IN_IMPLEMS(int, INT)
    SETUP_DATATYPES_IN_IMPLEMS(long, LONG)
    SETUP_DATATYPES_IN_IMPLEMS(long_long, LONG_LONG)
    SETUP_DATATYPES_IN_IMPLEMS(signed_char, SIGNED_CHAR)
    SETUP_DATATYPES_IN_IMPLEMS(unsigned_char, UNSIGNED_CHAR)
    SETUP_DATATYPES_IN_IMPLEMS(unsigned_short, UNSIGNED_SHORT)
    SETUP_DATATYPES_IN_IMPLEMS(unsigned, UNSIGNED)
    SETUP_DATATYPES_IN_IMPLEMS(unsigned_long, UNSIGNED_LONG)
    SETUP_DATATYPES_IN_IMPLEMS(unsigned_long_long, UNSIGNED_LONG_LONG)
    SETUP_DATATYPES_IN_IMPLEMS(float, FLOAT)
    SETUP_DATATYPES_IN_IMPLEMS(double, DOUBLE)
    SETUP_DATATYPES_IN_IMPLEMS(long_double, LONG_DOUBLE)
    SETUP_DATATYPES_IN_IMPLEMS(wchar, WCHAR)
    SETUP_DATATYPES_IN_IMPLEMS(c_bool, C_BOOL)
    SETUP_DATATYPES_IN_IMPLEMS(int8_t, INT8_T)
    SETUP_DATATYPES_IN_IMPLEMS(int16_t, INT16_T)
    SETUP_DATATYPES_IN_IMPLEMS(int32_t, INT32_T)
    SETUP_DATATYPES_IN_IMPLEMS(int64_t, INT64_T)
    SETUP_DATATYPES_IN_IMPLEMS(uint8_t, UINT8_T)
    SETUP_DATATYPES_IN_IMPLEMS(byte, BYTE)
    SETUP_DATATYPES_IN_IMPLEMS(uint16_t, UINT16_T)
    SETUP_DATATYPES_IN_IMPLEMS(uint32_t, UINT32_T)
    SETUP_DATATYPES_IN_IMPLEMS(uint64_t, UINT64_T)
    SETUP_DATATYPES_IN_IMPLEMS(c_float_complex, C_FLOAT_COMPLEX)
    SETUP_DATATYPES_IN_IMPLEMS(c_double_complex, C_DOUBLE_COMPLEX)
    SETUP_DATATYPES_IN_IMPLEMS(c_long_double_complex, C_LONG_DOUBLE_COMPLEX)
    SETUP_DATATYPES_IN_IMPLEMS(aint, AINT)
    SETUP_DATATYPES_IN_IMPLEMS(offset, OFFSET)
    SETUP_DATATYPES_IN_IMPLEMS(lb, LB)
    SETUP_DATATYPES_IN_IMPLEMS(ub, UB)
    SETUP_DATATYPES_IN_IMPLEMS(float_int, FLOAT_INT)
    SETUP_DATATYPES_IN_IMPLEMS(long_int, LONG_INT)
    SETUP_DATATYPES_IN_IMPLEMS(double_int, DOUBLE_INT)
    SETUP_DATATYPES_IN_IMPLEMS(short_int, SHORT_INT)
    SETUP_DATATYPES_IN_IMPLEMS(2int, 2INT)
    SETUP_DATATYPES_IN_IMPLEMS(long_double_int, LONG_DOUBLE_INT)
    SETUP_DATATYPES_IN_IMPLEMS(2float, 2FLOAT)
    SETUP_DATATYPES_IN_IMPLEMS(2double, 2DOUBLE)
    SETUP_DATATYPES_IN_IMPLEMS(2long, 2LONG)
    SETUP_DATATYPES_IN_IMPLEMS(real, REAL)
    SETUP_DATATYPES_IN_IMPLEMS(real4, REAL4)
    SETUP_DATATYPES_IN_IMPLEMS(real8, REAL8)
    SETUP_DATATYPES_IN_IMPLEMS(real16, REAL16)
    SETUP_DATATYPES_IN_IMPLEMS(complex8, COMPLEX8)
    SETUP_DATATYPES_IN_IMPLEMS(complex16, COMPLEX16)
    SETUP_DATATYPES_IN_IMPLEMS(complex32, COMPLEX32)
    SETUP_DATATYPES_IN_IMPLEMS(integer1, INTEGER1)
    SETUP_DATATYPES_IN_IMPLEMS(integer2, INTEGER2)
    SETUP_DATATYPES_IN_IMPLEMS(integer4, INTEGER4)
    SETUP_DATATYPES_IN_IMPLEMS(integer8, INTEGER8)
    SETUP_DATATYPES_IN_IMPLEMS(integer16, INTEGER16)
    SETUP_DATATYPES_IN_IMPLEMS(count, COUNT)
}

int S4BXI_MPI_Init(int* argc, char*** argv)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    XBT_INFO("Middleware init");
    int bull = ((init_func)main_actor->bull_mpi_ops.Init)(argc, argv);
    int smpi = ((init_func)main_actor->smpi_mpi_ops.Init)(argc, argv);

    return bull | smpi;
}

int S4BXI_MPI_Finalize()
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    XBT_INFO("Middleware finalize");
    int bull = ((finalize_func)main_actor->bull_mpi_ops.Finalize)();
    int smpi = ((finalize_func)main_actor->smpi_mpi_ops.Finalize)();

    return bull | smpi;
}

int S4BXI_MPI_Gather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                     MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    return ((gather_func)(USE_SMPI ? main_actor->smpi_mpi_ops : main_actor->bull_mpi_ops).Gather)(
        sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcount, implem_datatype(recvtype), root,
        implem_comm(comm));
}

int S4BXI_MPI_Allgather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                        MPI_Datatype recvtype, MPI_Comm comm)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    return ((allgather_func)(USE_SMPI ? main_actor->smpi_mpi_ops : main_actor->bull_mpi_ops).Allgather)(
        sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcount, implem_datatype(recvtype),
        implem_comm(comm));
}
