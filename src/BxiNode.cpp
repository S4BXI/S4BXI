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

using namespace simgrid;

BxiNode::BxiNode(int nid) : nid(nid), e2e_entries(s4u::Semaphore::create(MAX_E2E_ENTRIES)) {}

void BxiNode::pci_transfer(ptl_size_t size, bool direction, bxi_log_type type)
{
    s4u::Host* source = direction == PCI_CPU_TO_NIC ? main_host : nic_host;
    s4u::Host* dest   = direction == PCI_NIC_TO_CPU ? main_host : nic_host;

    S4BXI_STARTLOG(type, nid, nid)
    s4u::Comm::sendto(source, dest, size);
    S4BXI_WRITELOG()
}

s4u::CommPtr BxiNode::pci_transfer_async(ptl_size_t size, bool direction, bxi_log_type type)
{
    s4u::Host* source = direction == PCI_CPU_TO_NIC ? main_host : nic_host;
    s4u::Host* dest   = direction == PCI_NIC_TO_CPU ? main_host : nic_host;

    S4BXI_STARTLOG(type, nid, nid)

    // It's important to do that, instead of sendto_async,
    // see https://framagit.org/simgrid/simgrid/-/issues/60
    // (Thanks Martin for your help on this)
    s4u::CommPtr comm = s4u::Comm::sendto_init(source, dest);
    comm->set_remaining(size);

    if (__bxi_log_level)
        comm->on_completion.connect([__bxi_log_level, __bxi_log](s4u::Comm const&) mutable { S4BXI_WRITELOG(); });

    // Technically `detach` works too but if I do that Augustin wants to physically harm me so I guess I won't
    comm->start();

    return comm;
}

void BxiNode::issue_event(BxiEQ* eq, ptl_event_t* ev)
{
    if (eq == PTL_EQ_NONE)
        return;

    if (S4BXI_CONFIG_AND(this, model_pci_commands)) {
        pci_transfer(EVENT_SIZE, PCI_NIC_TO_CPU, S4BXILOG_PCI_EVENT);
    }

    eq->mailbox->put_init(ev, 0)->detach();
}

void BxiNode::acquire_e2e_entry(const BxiMsg* msg)
{
    if (e2e_off || msg->target == nid)
        return;

    e2e_entries->acquire();

    int max_inflight = S4BXI_GLOBAL_CONFIG(max_inflight_to_target);

    if (!max_inflight)
        return;

    int vn_num = (msg->type == S4BXI_PTL_PUT || msg->type == S4BXI_PTL_GET || msg->type == S4BXI_PTL_ATOMIC ||
                  msg->type == S4BXI_PTL_FETCH_ATOMIC)
                     ? S4BXI_VN_SERVICE_REQUEST
                     : S4BXI_VN_SERVICE_RESPONSE;
    if (!msg->parent_request->service_vn)
        vn_num += 1; // Switch to compute version

    s4u::SemaphorePtr flow_control_sem;
    auto it = flow_control_semaphores[vn_num].find(msg->target);
    if (it == flow_control_semaphores[vn_num].end()) {
        XBT_DEBUG("Making semaphore with max capacity of %d", max_inflight);
        flow_control_sem = s4u::Semaphore::create(max_inflight);
        flow_control_semaphores[vn_num].emplace(msg->target, flow_control_sem);
    } else {
        flow_control_sem = it->second;
    }
    flow_control_sem->acquire();
}

void BxiNode::release_e2e_entry(ptl_nid_t target_nid, bxi_vn vn)
{
    if (e2e_off || target_nid == nid)
        return;

    int max_inflight = S4BXI_GLOBAL_CONFIG(max_inflight_to_target);
    if (S4BXI_GLOBAL_CONFIG(max_inflight_to_target)) {
        auto it = flow_control_semaphores[vn].find(target_nid);

        if (it == flow_control_semaphores[vn].end())
            ptl_panic("Trying to release a flow control entry in a non-existing semaphore.");

        auto sem = it->second;
        sem->release();

        assert(sem->get_capacity() <= max_inflight);
    }

    e2e_entries->release();
}

BxiNode::~BxiNode()
{
    for (auto& tx_queue : tx_queues)
        delete tx_queue;
}
