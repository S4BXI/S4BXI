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

#ifndef S4BXI_BXINICINITIATOR_HPP
#define S4BXI_BXINICINITIATOR_HPP

#include "BxiNicActor.hpp"
#include "../s4ptl.hpp"

/**
 * @brief TX side of the NIC
 *
 * This actor listens for commands from the PCI bus and processes
 * them to send the correct message on the BXI network
 */
class BxiNicInitiator : public BxiNicActor {
    BxiQueue* tx_queue;

  public:
    explicit BxiNicInitiator(const vector<string>& args);

    void operator()();
    void handle_put(BxiMsg* msg);
    void handle_get(BxiMsg* msg);
    void handle_response(BxiMsg* msg);
    void handle_fetch_atomic_response(BxiMsg* msg);
};

#endif // S4BXI_BXINICINITIATOR_HPP
