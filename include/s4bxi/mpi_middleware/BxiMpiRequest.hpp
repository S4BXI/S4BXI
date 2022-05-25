/*
 * Author: Julien EMMANUEL
 * Copyright (C) 2019-2022 Bull S.A.S
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

#ifndef S4BXI_MPI_REQUEST_HPP
#define S4BXI_MPI_REQUEST_HPP

#include "s4bxi_mpi_ops.h"
#include <smpi/smpi.h>

class BxiMpiRequest {
  public:
    MPI_Request req;
    bool is_smpi;

    BxiMpiRequest(MPI_Request req, bool is_smpi) : req(req), is_smpi(is_smpi) {}
};

#endif
