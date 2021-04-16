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

#include "s4bxi/s4ptl.hpp"
#include "s4bxi/s4bxi_xbt_log.h"
#include "s4bxi/s4bxi_util.hpp"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(bxi_s4ptl_ni, "Messages specific to s4ptl NI implementation");

BxiNI::BxiNI(BxiNode* node, ptl_interface_t iface, unsigned int options, ptl_pid_t pid, ptl_ni_limits_t* limits)
    : node(node)
    , iface(iface)
    , options(options)
    , pid(pid)
    , limits(new ptl_ni_limits_t(*limits))
    , cq(s4u::Semaphore::create(CQ_INITIAL_CAPACITY))
{
}

BxiNI* BxiNI::init(BxiNode* node, ptl_interface_t iface, unsigned int options, ptl_pid_t pid,
                   const ptl_ni_limits_t* desired, ptl_ni_limits_t* actual)
{
    ptl_ni_limits_t tmp_limits;

    if (!actual)
        actual = &tmp_limits;

    actual->max_entries            = desired ? min(desired->max_entries, 16319) : 4080;
    actual->max_unexpected_headers = desired ? min(desired->max_unexpected_headers, 16319) : 16319;
    actual->max_mds                = desired ? min(desired->max_mds, 8191) : 1024;
    actual->max_cts                = desired ? min(desired->max_cts, 2047) : 1024;
    actual->max_eqs                = desired ? min(desired->max_eqs, 2046) : 960;
    actual->max_pt_index           = desired ? min(desired->max_pt_index, 255) : 255;
    actual->max_iovecs             = desired ? min(desired->max_iovecs, 0) : 0;
    actual->max_list_size          = desired ? min(desired->max_list_size, 65535) : 16582;
    actual->max_triggered_ops      = desired ? min(desired->max_triggered_ops, 16378) : 16378;
    actual->max_msg_size           = desired ? min(desired->max_msg_size, ptl_size_t(67108864)) : 67108864;
    actual->max_atomic_size        = desired ? min(desired->max_atomic_size, ptl_size_t(1024)) : 1024;
    actual->max_fetch_atomic_size  = desired ? min(desired->max_fetch_atomic_size, ptl_size_t(64)) : 64;
    actual->max_waw_ordered_size   = desired ? min(desired->max_waw_ordered_size, ptl_size_t(0)) : 0;
    actual->max_war_ordered_size   = desired ? min(desired->max_war_ordered_size, ptl_size_t(0)) : 0;
    actual->max_volatile_size      = desired ? min(desired->max_volatile_size, ptl_size_t(64)) : 64;
    actual->features               = desired ? desired->features : 7;

    return new BxiNI(node, iface, options, pid, actual);
}

void BxiNI::fini(ptl_handle_ni_t handle)
{
    auto n = (BxiNI*)handle;

    for (int i = 0; i < CQ_INITIAL_CAPACITY; ++i)
        n->cq->acquire(); // This is a (maybe weird) way of making sure all commands have been accepted by the NIC
                          // before destroying the CQ
    delete n->limits;
    delete n;
}

bool BxiNI::can_match_request(BxiRequest* req)
{
    return (pid == req->target_pid || req->target_pid == PTL_PID_ANY) &&
           HAS_PTL_OPTION(this, PTL_NI_MATCHING) == req->matching;
}

ptl_rank_t BxiNI::get_l2p_rank()
{
    for (int i = 0; i < l2p_map.size(); ++i) {
        if (l2p_map[i].phys.nid == node->nid && l2p_map[i].phys.pid == pid)
            return i;
    }

    ptl_panic("Couldn't find my rank in L2P table");
}

const ptl_process_t BxiNI::get_physical_proc(const ptl_process_t& proc)
{
    return HAS_PTL_OPTION(this, PTL_NI_PHYSICAL) ? proc : l2p_map[proc.rank];
}