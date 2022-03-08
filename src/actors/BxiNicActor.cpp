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

#include "s4bxi/actors/BxiNicActor.hpp"
#include "s4bxi/actors/BxiNicE2E.hpp"

using namespace std;
using namespace simgrid;

BxiNicActor::BxiNicActor(const vector<string>& args) : BxiActor()
{
    xbt_assert(args.size() == 1, "NIC actors expect no arguments");
    vn = (bxi_vn)atoi(self->get_property("VN"));
    self->daemonize();
}

// TO-DO : factorize get & fetch atomic identical things

void BxiNicActor::maybe_issue_get(BxiGetRequest* req)
{
    // Make sure the GET event hasn't already been issued for this request.
    if (req->get_event_issued)
        return;

    req->get_event_issued = true;

    if (req->matched_me && req->matched_me->me && !HAS_PTL_OPTION(req->matched_me->me, PTL_ME_EVENT_COMM_DISABLE) &&
        !HAS_PTL_OPTION(req->matched_me->me, PTL_ME_EVENT_SUCCESS_DISABLE) &&
        req->matched_me->list == PTL_PRIORITY_LIST) { // OVERFLOW ME will have a GET_OVERFLOW later
        auto event           = new ptl_event_t;
        event->initiator     = ptl_process_t{.phys{.nid = req->md->ni->node->nid, .pid = req->md->ni->pid}};
        event->type          = PTL_EVENT_GET;
        event->ni_fail_type  = PTL_OK;
        event->pt_index      = req->matched_me->pt->index;
        event->user_ptr      = req->matched_me->user_ptr;
        event->rlength       = req->payload_size;
        event->mlength       = req->mlength;
        event->remote_offset = req->remote_offset;
        event->match_bits    = req->match_bits;
        event->start         = req->start;
        node->issue_event(req->matched_me->pt->eq, event);
    }
}

void BxiNicActor::maybe_issue_fetch_atomic(BxiFetchAtomicRequest* req)
{
    // Make sure the FETCH  event hasn't already been issued for this request.
    if (req->fetch_atomic_event_issued)
        return;

    req->fetch_atomic_event_issued = true;

    if (req->matched_me && req->matched_me->me && !HAS_PTL_OPTION(req->matched_me->me, PTL_ME_EVENT_COMM_DISABLE) &&
        !HAS_PTL_OPTION(req->matched_me->me, PTL_ME_EVENT_SUCCESS_DISABLE) &&
        req->matched_me->list == PTL_PRIORITY_LIST) { // OVERFLOW ME will have a FETCH_ATOMIC_OVERFLOW later
        auto event           = new ptl_event_t;
        event->initiator     = ptl_process_t{.phys{.nid = req->md->ni->node->nid, .pid = req->md->ni->pid}};
        event->type          = PTL_EVENT_FETCH_ATOMIC;
        event->ni_fail_type  = PTL_OK;
        event->pt_index      = req->matched_me->pt->index;
        event->user_ptr      = req->matched_me->user_ptr;
        event->hdr_data      = req->hdr;
        event->rlength       = req->payload_size;
        event->mlength       = req->mlength;
        event->remote_offset = req->remote_offset;
        event->match_bits    = req->match_bits;
        event->start         = req->start;
        node->issue_event(req->matched_me->pt->eq, event);
    }
}

void BxiNicActor::reliable_comm(BxiMsg* msg)
{
    S4BXI_STARTLOG((bxi_log_type)msg->type, msg->initiator, msg->target);
    //             ^^^^^^^^^^^^^^
    // This highly unsafe cast is why the beginning of the bxi_log_type
    // enum must be the exact copy of the bxi_msg_type enum
    reliable_comm_init(msg, false)->wait();
    S4BXI_WRITELOG();
}

void BxiNicActor::shallow_reliable_comm(BxiMsg* msg)
{
    reliable_comm_init(msg, true)->wait();
}

s4u::CommPtr BxiNicActor::reliable_comm_init(BxiMsg* msg, bool shallow)
{
    s4u::this_actor::sleep_for(2e-9);
    // BXI_ACKs don't have any higher level of ACK, so no E2E logic
    //                    ⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄
    if (!node->e2e_off && msg->type != S4BXI_E2E_ACK) {
        node->e2e_actor->process_message(msg);
    }

    return s4u::Mailbox::by_name(nic_rx_mailbox_name(msg->target, vn))
        ->put_init(msg, shallow ? 0 : msg->simulated_size)
        ->set_copy_data_callback(&s4u::Comm::copy_pointer_callback);
}
