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

#ifndef S4BXI_BXINODE_HPP
#define S4BXI_BXINODE_HPP

#include <map>
#include <string>
#include <vector>

#include "portals4.h"
#include "s4ptl.hpp"
#include "s4bxi/BxiLog.hpp"
#include "s4bxi/BxiQueue.hpp"

using namespace std;

class BxiNicE2E;

class BxiNode {
  public:
    explicit BxiNode(int nid);
    ~BxiNode();

    vector<int> used_pids;
    vector<BxiNI*> ni_handles;
    ptl_nid_t nid;
    s4u::Host* main_host;
    s4u::Host* nic_host;
    s4u::SemaphorePtr e2e_entries;
    BxiNicE2E* e2e_actor   = nullptr;
    BxiQueue* tx_queues[4] = {nullptr, nullptr, nullptr, nullptr};

    // Params
    bool use_real_memory = false;
    bool model_pci       = true;
    bool e2e_off         = true;

    // Counters
    unsigned long e2e_retried = 0;
    unsigned long e2e_gave_up = 0;

    void pci_transfer(ptl_size_t size, bool direction, bxi_log_type type);
    void pci_transfer_async(ptl_size_t size, bool direction, bxi_log_type type);
    void issue_event(BxiEQ* eq, ptl_event_t* ev);
};

#endif // S4BXI_BXINODE_HPP