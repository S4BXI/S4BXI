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

#ifndef S4BXI_BXINICE2E_HPP
#define S4BXI_BXINICE2E_HPP

#include "BxiActor.hpp"
#include "../s4ptl.hpp"
#include "../BxiQueue.hpp"

class BxiNicE2E : public BxiActor {
    simgrid::s4u::Mailbox* nic_cmd_mailboxes[4] = {nullptr};
    BxiMsg* current_msg                         = nullptr;
    BxiQueue queue;

  public:
    explicit BxiNicE2E(const std::vector<std::string>& args);

    void operator()();
    simgrid::s4u::Mailbox* get_retransmit_mailbox(const BxiMsg* msg);
    void process_message(BxiMsg* msg);
};

#endif // S4BXI_BXINICE2E_HPP
