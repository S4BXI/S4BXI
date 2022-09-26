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

#ifndef S4BXI_BXINICACTOR_HPP
#define S4BXI_BXINICACTOR_HPP

#include <vector>
#include <string>

#include "BxiActor.hpp"
#include "../s4ptl.hpp"

constexpr double ONE_PCI_PACKET_TRANSFER = 400e-9 /* PCI latency */ + 32.508e-9 /* Bandwidth for 512B */;

class BxiNicActor : public BxiActor {
  protected:
    bxi_vn vn;
    void maybe_issue_get(BxiGetRequest* req);
    void maybe_issue_fetch_atomic(BxiFetchAtomicRequest* req);
    void reliable_comm(BxiMsg* msg);
    void shallow_reliable_comm(BxiMsg* msg);
    simgrid::s4u::CommPtr reliable_comm_init(BxiMsg* msg, bool shallow);

  public:
    BxiNicActor(const std::vector<std::string>& args);
};
#endif // S4BXI_BXINICACTOR_HPP