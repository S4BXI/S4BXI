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

#include "s4bxi/mpi_middleware/BxiMpiComm.hpp"
#include "s4bxi/s4bxi_xbt_log.h"
#include "s4bxi/actors/BxiMainActor.hpp"
#include "s4bxi/s4bxi_util.hpp"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_mpi_comm, "Messages specific to the middleware MPI communicators");

MPI_Comm BxiMpiComm::implem_comm(MPI_Comm original)
{
        BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

// Inside this cpp file, we are in simulator territory (not in the user app), so the symbols like MPI_COMM_WORLD are
// only linked with SimGrid, and therefore they refer to the SMPI variant
#define MPI_COMM_TRANSLATION(comm)                                                                                     \
    if (original == MPI_COMM_##comm || original == main_actor->bull_mpi_ops->COMM_##comm)                              \
        return (main_actor->use_smpi_implem ? MPI_COMM_##comm : main_actor->bull_mpi_ops->COMM_##comm);

    MPI_COMM_TRANSLATION(WORLD);
    MPI_COMM_TRANSLATION(SELF);

    auto s4bxi_comm = (BxiMpiComm*)original;

    return main_actor->use_smpi_implem ? s4bxi_comm->smpi : s4bxi_comm->bull;
}

MPI_Comm BxiMpiComm::bull_comm(MPI_Comm original)
{
#define MPI_COMM_BULL(comm)                                                                                            \
    if (original == MPI_COMM_##comm || original == GET_CURRENT_MAIN_ACTOR->bull_mpi_ops->COMM_##comm)                  \
        return GET_CURRENT_MAIN_ACTOR->bull_mpi_ops->COMM_##comm;

    MPI_COMM_BULL(WORLD);
    MPI_COMM_BULL(SELF);

    auto s4bxi_comm = (BxiMpiComm*)original;

    return s4bxi_comm->bull;
}

MPI_Comm BxiMpiComm::smpi_comm(MPI_Comm original)
{
// Inside this cpp file, we are in simulator territory (not in the user app), so the symbols like MPI_COMM_WORLD are
// only linked with SimGrid, and therefore they refer to the SMPI variant
#define MPI_COMM_SMPI(comm)                                                                                            \
    if (original == MPI_COMM_##comm || original == GET_CURRENT_MAIN_ACTOR->bull_mpi_ops->COMM_##comm)                  \
        return MPI_COMM_##comm;

    MPI_COMM_SMPI(WORLD);
    MPI_COMM_SMPI(SELF);

    auto s4bxi_comm = (BxiMpiComm*)original;

    return s4bxi_comm->smpi;
}