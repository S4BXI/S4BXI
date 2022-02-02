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

#ifndef S4BXI_MPI_DATATYPE_HPP
#define S4BXI_MPI_DATATYPE_HPP

#include "s4bxi_mpi_ops.h"
#include <smpi/smpi.h>

class BxiMpiDatatype {
  public:
    MPI_Datatype bull;
    MPI_Datatype smpi;

    BxiMpiDatatype(MPI_Datatype bull, MPI_Datatype smpi) : bull(bull), smpi(smpi) {}
    static MPI_Datatype implem_datatype(MPI_Datatype original);
    static MPI_Datatype bull_datatype(MPI_Datatype original);
    static MPI_Datatype smpi_datatype(MPI_Datatype original);
};

#endif
