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

    // See comment about broken things below
    // S4BXI_STARTLOG(type, nid, nid)

    // It's important to do that, instead of sendto_async,
    // see https://framagit.org/simgrid/simgrid/-/issues/60
    // (Thanks Martin for your help on this)
    s4u::CommPtr comm = s4u::Comm::sendto_init(source, dest);
    comm->set_remaining(size);

    // This is broken because SimGrid's signals don't do what I was expecting. Disable it completely until I make a 
    // proper plugin for logging this type of things
    // if (__bxi_log_level) {
    //     comm->on_completion.connect([__bxi_log_level, __bxi_log, comm](s4u::Comm const&) mutable { fprintf(stderr,
    //     "Writing pci_transfer log for comm %p\n", comm); S4BXI_WRITELOG(); });
    // }

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
    if (e2e_off)
        return;

    e2e_entries->acquire();

    int max_inflight_to_target = S4BXI_GLOBAL_CONFIG(max_inflight_to_target);

    bxi_vn vn = msg->get_vn();

    if (max_inflight_to_target) {
        s4u::SemaphorePtr flow_control_sem;

        auto it = flowctrl_sems_node[vn].find(msg->target);

        if (it == flowctrl_sems_node[vn].end()) {
            XBT_DEBUG("Making semaphore %u -> %u with max capacity of %d (node-level)", nid, msg->target,
                      max_inflight_to_target);
            flow_control_sem = s4u::Semaphore::create(max_inflight_to_target);
            flowctrl_sems_node[vn].emplace(msg->target, flow_control_sem);
        } else {
            flow_control_sem = it->second;
        }

        flow_control_sem->acquire();
    }

    int max_inflight_to_process = S4BXI_GLOBAL_CONFIG(max_inflight_to_process);
    if (!max_inflight_to_process)
        return;

    s4u::SemaphorePtr flow_control_sem;
    ptl_pid_t req_src = msg->parent_request->md->ni->pid;
    ptl_pid_t req_target = msg->parent_request->target_pid;
    bool is_request_vn = vn == S4BXI_VN_COMPUTE_REQUEST || vn == S4BXI_VN_SERVICE_REQUEST;
    flowctrl_process_id f_id = {.src_pid = is_request_vn ? req_src : req_target,
                                .dst_pid = is_request_vn ? req_target : req_src,
                                .dst_nid = msg->target};

    auto it = flowctrl_sems_process[vn].find(f_id);

    if (it == flowctrl_sems_process[vn].end()) {
        XBT_DEBUG("Making semaphore %u:%u -> %u:%u with max capacity of %d (process-level)", nid,
                  msg->parent_request->md->ni->pid, msg->target, msg->parent_request->target_pid,
                  max_inflight_to_process);
        flow_control_sem = s4u::Semaphore::create(max_inflight_to_process);
        flowctrl_sems_process[vn].emplace(f_id, flow_control_sem);
    } else {
        flow_control_sem = it->second;
    }

    flow_control_sem->acquire();
}

void BxiNode::release_e2e_entry(ptl_nid_t target_nid, bxi_vn vn, ptl_pid_t src_pid, ptl_pid_t dst_pid)
{
    if (e2e_off)
        return;

    e2e_entries->release();

    int max_inflight = S4BXI_GLOBAL_CONFIG(max_inflight_to_target);
    if (S4BXI_GLOBAL_CONFIG(max_inflight_to_target)) {
        auto it = flowctrl_sems_node[vn].find(target_nid);

        if (it == flowctrl_sems_node[vn].end())
            ptl_panic_fmt("Trying to release a flow control entry in a non-existing semaphore (node-level): %u -> %u",
                          nid, target_nid);

        auto sem = it->second;
        sem->release();

        xbt_assert(sem->get_capacity() <= max_inflight,
                   "Semaphore %u -> %u has more capacity than max inflight (%u > %u)", nid, target_nid,
                   sem->get_capacity(), max_inflight);
    }

    if (!S4BXI_GLOBAL_CONFIG(max_inflight_to_process))
        return;

    auto it = flowctrl_sems_process[vn].find({.src_pid = src_pid, .dst_pid = dst_pid, .dst_nid = target_nid});

    if (it == flowctrl_sems_process[vn].end())
        ptl_panic_fmt(
            "Trying to release a flow control entry in a non-existing semaphore (process-level): %u:%u -> %u:%u", nid,
            src_pid, target_nid, dst_pid);

    auto sem = it->second;
    sem->release();

    assert(sem->get_capacity() <= max_inflight);
}

BxiNode::~BxiNode()
{
    for (auto& tx_queue : tx_queues)
        delete tx_queue;
}
