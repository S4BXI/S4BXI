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

#include "s4bxi/BxiNode.hpp"
#include "s4bxi/s4bxi_util.hpp"
#include "s4bxi/s4bxi_xbt_log.h"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_bxi_node, "Messages specific to BxiNode");

using namespace simgrid;
using namespace std;

BxiNode::BxiNode(int nid) : nid(nid), e2e_entries(s4u::Semaphore::create(MAX_E2E_ENTRIES)) {}

void BxiNode::pci_transfer(ptl_size_t size, bool direction, bxi_log_type type)
{
    s4u::Host* source = direction == PCI_CPU_TO_NIC ? main_host : nic_host;
    s4u::Host* dest   = direction == PCI_NIC_TO_CPU ? main_host : nic_host;

    S4BXI_STARTLOG(type, nid, nid)
    s4u::Comm::sendto(source, dest, size);
    S4BXI_WRITELOG()
}

void BxiNode::pci_transfer_async(ptl_size_t size, bool direction, bxi_log_type type)
{
    pci_transfer_init(size, direction, type)->detach();
}

s4u::CommPtr BxiNode::pci_transfer_init(ptl_size_t size, bool direction, bxi_log_type type)
{
    s4u::Host* source = direction == PCI_CPU_TO_NIC ? main_host : nic_host;
    s4u::Host* dest   = direction == PCI_NIC_TO_CPU ? main_host : nic_host;

    // See comment about broken things below
    // S4BXI_STARTLOG(type, nid, nid)

    // It's important to do that, instead of sendto_async,
    // see https://framagit.org/simgrid/simgrid/-/issues/60
    // (Thanks Martin for your help on this)
    s4u::CommPtr comm = s4u::Comm::sendto_init(source, dest);
    comm->set_payload_size(size);

    // This is broken because SimGrid's signals don't do what I was expecting. Disable it completely until I make a
    // proper plugin for logging this type of things
    // if (__bxi_log_level) {
    //     comm->on_completion_cb([__bxi_log_level, __bxi_log, comm](s4u::Comm const&) mutable { fprintf(stderr,
    //     "Writing pci_transfer log for comm %p\n", comm); S4BXI_WRITELOG(); });
    // }

    // Technically `detach` works too but if I do that Augustin wants to physically harm me so I guess I won't
    // Edit: do not do anything to this comm for now
    // comm->start();

    return comm;
}

void BxiNode::issue_event(BxiEQ* eq, ptl_event_t* ev)
{
    if (eq == PTL_EQ_NONE)
        return;

    // There is literally no other way to make this transfer asynchronous while still being able to log it
    // s4u::Actor::create("_pci_event_actor", s4u::Host::current(), [&]() {
        if (S4BXI_CONFIG_AND(this, model_pci_commands))
            pci_transfer(EVENT_SIZE, PCI_NIC_TO_CPU, S4BXILOG_PCI_EVENT);
        eq->mailbox->put_init(ev, 0)->set_copy_data_callback(&s4u::Comm::copy_pointer_callback)->detach();
    // });
}

bool BxiNode::check_flowctrl(const BxiMsg* msg)
{
    if (e2e_off || msg->type == S4BXI_E2E_ACK || msg->retry_count)
        return true;

    bxi_vn vn = msg->get_vn();
    shared_ptr<int> flow_control_count_node;
    shared_ptr<int> flow_control_count_process;

    // Check node-level flow-control
    int max_inflight_to_target = S4BXI_GLOBAL_CONFIG(max_inflight_to_target);
    if (max_inflight_to_target) {
        auto it = flowctrl_node_counts[vn].find(msg->target);

        if (it == flowctrl_node_counts[vn].end()) {
            XBT_INFO("Making semaphore %u -> %u with max capacity of %d (node-level)", nid, msg->target,
                     max_inflight_to_target);
            flow_control_count_node = make_shared<int>(max_inflight_to_target);
            flowctrl_node_counts[vn].emplace(msg->target, flow_control_count_node);
        } else {
            flow_control_count_node = it->second;
        }

        if (*flow_control_count_node < 0)
            ptl_panic("Node flow control has less than 0 credits");
        if (*flow_control_count_node == 0)
            return false;
    }

    // Check process-level flow-control
    int max_inflight_to_process = S4BXI_GLOBAL_CONFIG(max_inflight_to_process);
    if (max_inflight_to_process) {
        ptl_pid_t req_src        = msg->parent_request->md->ni->pid;
        ptl_pid_t req_target     = msg->parent_request->target_pid;
        bool is_request_vn       = vn == S4BXI_VN_COMPUTE_REQUEST || vn == S4BXI_VN_SERVICE_REQUEST;
        flowctrl_process_id f_id = {.src_pid = is_request_vn ? req_src : req_target,
                                    .dst_pid = is_request_vn ? req_target : req_src,
                                    .dst_nid = msg->target};

        auto it = flowctrl_process_counts[vn].find(f_id);

        if (it == flowctrl_process_counts[vn].end()) {
            XBT_DEBUG("Making semaphore %u:%u -> %u:%u with max capacity of %d (process-level)", nid,
                      msg->parent_request->md->ni->pid, msg->target, msg->parent_request->target_pid,
                      max_inflight_to_process);
            flow_control_count_process = make_shared<int>(max_inflight_to_process);
            flowctrl_process_counts[vn].emplace(f_id, flow_control_count_process);
        } else {
            flow_control_count_process = it->second;
        }

        if (*flow_control_count_process < 0)
            ptl_panic("Process flow control has less than 0 credits");
        if (*flow_control_count_process == 0)
            return false;
    }

    // If we have enough flow control, consume the entries
    if (max_inflight_to_target) {
        *flow_control_count_node -= 1;
    }
    if (max_inflight_to_process) {
        *flow_control_count_process -= 1;
    }

    return true;
}

void BxiNode::acquire_e2e_entry(const BxiMsg* msg)
{
    if (e2e_off)
        return;

    e2e_entries->acquire();
}

void BxiNode::release_e2e_entry(ptl_nid_t target_nid, bxi_vn vn, ptl_pid_t src_pid, ptl_pid_t dst_pid)
{
    if (e2e_off)
        return;

    e2e_entries->release();

    int max_inflight_to_target = S4BXI_GLOBAL_CONFIG(max_inflight_to_target);
    if (max_inflight_to_target) {
        auto it = flowctrl_node_counts[vn].find(target_nid);

        if (it == flowctrl_node_counts[vn].end())
            ptl_panic_fmt("Trying to release a flow control entry in a non-existing counter (node-level): %u -> %u",
                          nid, target_nid);

        auto count = it->second;
        *count += 1;

        xbt_assert(*count <= max_inflight_to_target, "Counter %u -> %u has more capacity than max inflight (%u > %u)",
                   nid, target_nid, *count, max_inflight_to_target);
    }

    int max_inflight_to_process = S4BXI_GLOBAL_CONFIG(max_inflight_to_process);
    if (max_inflight_to_process) {
        auto it = flowctrl_process_counts[vn].find({.src_pid = src_pid, .dst_pid = dst_pid, .dst_nid = target_nid});

        if (it == flowctrl_process_counts[vn].end())
            ptl_panic_fmt(
                "Trying to release a flow control entry in a non-existing counter (process-level): %u:%u -> %u:%u", nid,
                src_pid, target_nid, dst_pid);

        auto count = it->second;
        *count += 1;

        xbt_assert(*count <= max_inflight_to_process,
                   "Counter %u:%u -> %u:%u has more capacity than max inflight (%u > %u)", nid, src_pid, target_nid,
                   dst_pid, *count, max_inflight_to_process);
    }

    if (max_inflight_to_process || max_inflight_to_target)
        resume_waiting_tx_actors(vn);
}

void BxiNode::resume_waiting_tx_actors(bxi_vn vn)
{
    while (!flowctrl_waiting_messages[vn].empty()) {
        tx_queues[vn]->put(flowctrl_waiting_messages[vn].front(), 0, true);
        flowctrl_waiting_messages[vn].pop_front();
    }
}
