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

#ifndef S4BXI_MPI_COMM_HPP
#define S4BXI_MPI_COMM_HPP

#include "s4bxi_mpi_ops.h"
#include <smpi/smpi.h>

class BxiMpiComm {
  public:
    MPI_Comm bull;
    MPI_Comm smpi;

    static MPI_Comm implem_comm(MPI_Comm original);
    static MPI_Comm bull_comm(MPI_Comm original);
    static MPI_Comm smpi_comm(MPI_Comm original);
};

#endif
