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

#include "s4bxi/actors/BxiNicActor.hpp"
#include "s4bxi/actors/BxiNicE2E.hpp"

BxiNicActor::BxiNicActor(const vector<string>& args) : BxiActor()
{
    xbt_assert(args.size() == 1, "NIC actors expect no arguments");
    vn = (bxi_vn)atoi(self->get_property("VN"));
    self->daemonize();
}

void BxiNicActor::maybe_issue_send(BxiPutRequest* req)
{
    // Make sure the SEND event hasn't already been issued for this MD.
    // It could have been if :
    //
    //    * We're retransmitting (E2E) a buffered PUT (seems unlikely
    //      but it is physically possible)
    //
    //    * We're processing the portals ACK after a buffered PUT
    //      (wether there was E2E retries in there or not)
    //
    //    * We're processing a retransmitted ACK after a PUT that was
    //      not buffered
    if (req->send_event_issued)
        return;

    // if (!req->md || !req->md->md || !req->md->ni) return;
    // This is extremely ugly: if the md has been deleted it probably comes from another error
    // somewhere, but maybe it can come from an error in the user code ? In this case I guess
    // we should fail silently, and hope we don't mess anything up (I guess that's how a real-
    // world NIC works ? It doesn't segfault ? Who knows about NIC's life ...)

    req->send_event_issued = true;

    if (HAS_PTL_OPTION(req->md->md, PTL_MD_EVENT_CT_SEND))
        req->md->increment_ct(req->payload_size);

    if (!HAS_PTL_OPTION(req->md->md, PTL_MD_EVENT_SEND_DISABLE) &&
        !HAS_PTL_OPTION(req->md->md, PTL_MD_EVENT_SUCCESS_DISABLE)) {
        auto event          = new ptl_event_t;
        event->type         = PTL_EVENT_SEND;
        event->ni_fail_type = PTL_OK;
        event->user_ptr     = req->user_ptr;
        event->mlength      = req->payload_size; // TO-DO : support truncated payloads
        issue_event((BxiEQ*)req->md->md->eq_handle, event);
    }
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
        auto event          = new ptl_event_t;
        event->type         = PTL_EVENT_GET;
        event->ni_fail_type = PTL_OK;
        event->pt_index     = req->matched_me->pt->index;
        event->user_ptr     = req->matched_me->user_ptr;
        event->mlength      = req->payload_size; // TO-DO : support truncated payloads
        event->match_bits   = req->match_bits;   // Maybe check if we have a matching NI ?
        event->start        = req->start;
        issue_event(req->matched_me->pt->eq, event);
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
        auto event          = new ptl_event_t;
        event->type         = PTL_EVENT_FETCH_ATOMIC;
        event->ni_fail_type = PTL_OK;
        event->pt_index     = req->matched_me->pt->index;
        event->user_ptr     = req->matched_me->user_ptr;
        event->hdr_data     = req->hdr;
        event->mlength      = req->payload_size; // TO-DO : support truncated payloads
        event->match_bits   = req->match_bits;   // Maybe check if we have a matching NI ?
        event->start        = req->start;
        issue_event(req->matched_me->pt->eq, event);
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
    //             BXI_ACKs don't have any higher level of ACK, so no E2E logic
    //                                              ⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄
    if (!S4BXI_CONFIG(e2e_off) && !node->e2e_off && msg->type != S4BXI_E2E_ACK) {
        node->e2e_actor->process_message(msg);
    }

    return s4u::Mailbox::by_name(nic_rx_mailbox_name(msg->target, vn))
        ->put_init(msg, shallow ? 0 : msg->simulated_size);
}