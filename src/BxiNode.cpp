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

#include "s4bxi/BxiNode.hpp"
#include "s4bxi/s4bxi_util.hpp"
#include "s4bxi/s4bxi_xbt_log.h"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_bxi_node, "Messages specific to BxiNode");

/**
 * Actor to perform a DMA operation and log it
 *
 * We can't use an async sendto because we need to know when it ends (to write the log), so the only option I see is to
 * have this whole actor just to perform the operation
 */
void dma_log_actor(vector<string> args)
{
    ptl_nid_t nid = atoi(args[2].c_str());
    S4BXI_STARTLOG((bxi_log_type)atoi(args[3].c_str()), nid, nid)
    s4u::Actor::self()->get_host()->sendto(s4u::Host::by_name(args[1]), atoi(args[0].c_str()));
    S4BXI_WRITELOG()
}

BxiNode::BxiNode(int nid) : nid(nid), e2e_entries(s4u::Semaphore::create(MAX_E2E_ENTRIES)) {}

void BxiNode::pci_transfer(ptl_size_t size, bool direction, bxi_log_type type)
{
    simgrid::s4u::Host* source = direction == PCI_CPU_TO_NIC ? main_host : nic_host;
    simgrid::s4u::Host* dest   = direction == PCI_NIC_TO_CPU ? main_host : nic_host;

    S4BXI_STARTLOG(type, nid, nid)
    source->sendto(dest, size);
    S4BXI_WRITELOG()
}

void BxiNode::pci_transfer_async(ptl_size_t size, bool direction, bxi_log_type type)
{
    simgrid::s4u::Host* source = direction == PCI_CPU_TO_NIC ? main_host : nic_host;
    simgrid::s4u::Host* dest   = direction == PCI_NIC_TO_CPU ? main_host : nic_host;

    if (!S4BXI_CONFIG(log_level)) {
        // It's important to do that, instead of sendto_async,
        // see https://framagit.org/simgrid/simgrid/-/issues/60
        // (Thanks Martin for your help on this)
        auto comm = s4u::Comm::sendto_init(source, dest);
        comm->set_remaining(size);
        comm->detach();
        return;
    }

    // If we want to log things, it is going to be painful

    vector<string> args(4);
    args[0] = to_string(size);
    args[1] = dest->get_name();
    args[2] = to_string(nid);
    args[3] = to_string(type);
    s4u::Actor::create("dma_logger", source, dma_log_actor, args);
}

void BxiNode::issue_event(BxiEQ* eq, ptl_event_t* ev)
{
    if (eq == PTL_EQ_NONE)
        return;

    if (model_pci) {
        pci_transfer(EVENT_SIZE, PCI_NIC_TO_CPU, S4BXILOG_PCI_EVENT);
    }

    eq->mailbox->put_init(ev, 0)->detach();
}

BxiNode::~BxiNode()
{
    for (auto& tx_queue : tx_queues)
        delete tx_queue;
}
