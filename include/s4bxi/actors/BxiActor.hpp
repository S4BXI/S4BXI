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

#ifndef S4BXI_BXIACTOR_HPP
#define S4BXI_BXIACTOR_HPP

#include <string>

#include "../s4ptl.hpp"
#include "../BxiEngine.hpp"
#include "../BxiNode.hpp"
#include "../s4bxi_util.hpp"

constexpr int INLINE_BASE_SIZE = 8;
constexpr int MATCH_BITS_SIZE  = 8;
constexpr int PIO_BASE_SIZE    = 408;

#define INLINE_SIZE(req) ((req)->matching ? INLINE_BASE_SIZE : (INLINE_BASE_SIZE + MATCH_BITS_SIZE))
#define PIO_SIZE(req)    ((req)->matching ? PIO_BASE_SIZE : (PIO_BASE_SIZE + MATCH_BITS_SIZE))

class BxiActor {
  protected:
    std::string slug;
    std::shared_ptr<BxiNode> node;
    simgrid::s4u::Actor* self;

    // Mailbox names
    std::string nic_rx_mailbox_name(const bxi_vn);
    std::string nic_tx_mailbox_name(const bxi_vn);
    static std::string nic_rx_mailbox_name(const int, const bxi_vn);
    static std::string nic_tx_mailbox_name(const int, const bxi_vn);

    // Events
    void issue_event(BxiEQ* eq, ptl_event_t* ev);

  public:
    BxiActor();
    simgrid::s4u::Actor* getSimgridActor();
    ptl_nid_t getNid();
    std::string getSlug();
    std::shared_ptr<BxiNode> getNode();
};

#endif // S4BXI_BXIACTOR_HPP
