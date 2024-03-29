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

#ifndef S4BXI_BXINODE_HPP
#define S4BXI_BXINODE_HPP

#include <map>
#include <string>
#include <vector>
#include <deque>

#include "portals4.h"
#include "s4ptl.hpp"
#include "s4bxi/BxiLog.hpp"
#include "s4bxi/BxiQueue.hpp"

class BxiNicE2E;

struct flowctrl_process_id {
    ptl_pid_t src_pid;
    ptl_pid_t dst_pid;
    ptl_nid_t dst_nid;
    // The struct requires an operator "<" to be usable as a std::map's key
    bool operator<(const flowctrl_process_id& other) const
    {
        return std::tie(src_pid, dst_pid, dst_nid) < std::tie(other.src_pid, other.dst_pid, other.dst_nid);
    }
};

class BxiNode {
  public:
    explicit BxiNode(int nid);

    std::vector<int> used_pids;
    std::vector<BxiNI*> ni_handles;
    ptl_nid_t nid;
    simgrid::s4u::Host* main_host;
    simgrid::s4u::Host* nic_host;
    simgrid::s4u::SemaphorePtr e2e_entries;
    BxiNicE2E* e2e_actor                   = nullptr;
    std::shared_ptr<BxiQueue> tx_queues[4] = {nullptr, nullptr, nullptr, nullptr};

    // Node level flow control semaphores
    std::map<ptl_nid_t, std::shared_ptr<int>> flowctrl_node_counts[4] = {
        std::map<ptl_nid_t, std::shared_ptr<int>>(), std::map<ptl_nid_t, std::shared_ptr<int>>(),
        std::map<ptl_nid_t, std::shared_ptr<int>>(), std::map<ptl_nid_t, std::shared_ptr<int>>()};
    // Process level flow control semaphores
    std::map<flowctrl_process_id, std::shared_ptr<int>> flowctrl_process_counts[4] = {
        std::map<flowctrl_process_id, std::shared_ptr<int>>(), std::map<flowctrl_process_id, std::shared_ptr<int>>(),
        std::map<flowctrl_process_id, std::shared_ptr<int>>(), std::map<flowctrl_process_id, std::shared_ptr<int>>()};
    // Process level miscellaneous things (for cycle detection and processing only)
    std::deque<BxiMsg*> flowctrl_waiting_messages[4] = {std::deque<BxiMsg*>(), std::deque<BxiMsg*>(),
                                                        std::deque<BxiMsg*>(), std::deque<BxiMsg*>()};

    // Params
    bool use_real_memory    = true;
    bool model_pci          = true;
    bool model_pci_commands = true;
    bool e2e_off            = true;

    // Counters
    unsigned long e2e_retried = 0;
    unsigned long e2e_gave_up = 0;

    void pci_transfer(ptl_size_t size, bool direction, bxi_log_type type);
    simgrid::s4u::CommPtr pci_transfer_async(ptl_size_t size, bool direction, bxi_log_type type, bool detach = false);
    simgrid::s4u::CommPtr pci_transfer_init(ptl_size_t size, bool direction, bxi_log_type type);
    void issue_event(BxiEQ* eq, ptl_event_t* ev);
    bool check_flowctrl(const BxiMsg* msg);
    void acquire_e2e_entry(const BxiMsg* msg);
    void release_e2e_entry(ptl_nid_t target_nid, bxi_vn vn, ptl_pid_t src_pid, ptl_pid_t dst_pid);
    void resume_waiting_tx_actors(bxi_vn vn);
};

#endif // S4BXI_BXINODE_HPP