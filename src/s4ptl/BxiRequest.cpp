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

S4BXI_LOG_NEW_DEFAULT_CATEGORY(bxi_s4ptl_request, "Messages specific to s4ptl request implementation");

BxiRequest::BxiRequest(bxi_req_type type, BxiMD* md, ptl_size_t payload_size, bool matching,
                       ptl_match_bits_t match_bits, ptl_pid_t target_pid, ptl_pt_index_t pt_index, void* user_ptr,
                       bool service_vn, ptl_size_t local_offset, ptl_size_t remote_offset)
    : type(type)
    , payload_size(payload_size)
    , md(new BxiMD(*md))
    , matching(matching)
    , match_bits(match_bits)
    , target_pid(target_pid)
    , pt_index(pt_index)
    , process_state(S4BXI_REQ_CREATED)
    , user_ptr(user_ptr)
    , service_vn(service_vn)
    , local_offset(local_offset)
    , remote_offset(remote_offset)
{
}

BxiRequest::~BxiRequest()
{
    delete md;
}

BxiPutRequest::BxiPutRequest(BxiMD* md, ptl_size_t payload_size, bool matching, ptl_match_bits_t match_bits,
                             ptl_pid_t target_pid, ptl_pt_index_t pt_index, void* user_ptr, bool service_vn,
                             ptl_size_t local_offset, ptl_size_t remote_offset, ptl_ack_req_t ack_req,
                             ptl_hdr_data_t hdr)
    : BxiRequest(S4BXI_PUT_REQUEST, md, payload_size, matching, match_bits, target_pid, pt_index, user_ptr, service_vn,
                 local_offset, remote_offset)
    , ack_req(ack_req)
    , hdr(hdr)
{
}

void BxiPutRequest::issue_ack()
{
    if (HAS_PTL_OPTION(md->md, PTL_MD_EVENT_CT_ACK))
        md->increment_ct(payload_size);
    if (ack_req == PTL_ACK_REQ) {
        auto ack           = new ptl_event_t;
        ack->type          = PTL_EVENT_ACK;
        ack->ni_fail_type  = PTL_OK;
        ack->user_ptr      = user_ptr;
        ack->mlength       = mlength;
        ack->remote_offset = target_remote_offset;
        (md->ni->node)->issue_event((BxiEQ*)md->md->eq_handle, ack);
    }
}

void BxiPutRequest::maybe_issue_send()
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
    if (send_event_issued)
        return;

    // if (!md || !md->md || !md->ni) return;
    // This is extremely ugly: if the md has been deleted it probably comes from another error
    // somewhere, but maybe it can come from an error in the user code ? In this case I guess
    // we should fail silently, and hope we don't mess anything up (I guess that's how a real-
    // world NIC works ? It doesn't segfault ? Who knows about NIC's life ...)

    send_event_issued = true;

    if (HAS_PTL_OPTION(md->md, PTL_MD_EVENT_CT_SEND))
        md->increment_ct(payload_size);

    if (!HAS_PTL_OPTION(md->md, PTL_MD_EVENT_SEND_DISABLE) && !HAS_PTL_OPTION(md->md, PTL_MD_EVENT_SUCCESS_DISABLE)) {
        auto event          = new ptl_event_t;
        event->type         = PTL_EVENT_SEND;
        event->ni_fail_type = PTL_OK;
        event->user_ptr     = user_ptr;
        event->mlength      = payload_size; // SEND doesn't care about truncated payloads
        (md->ni->node)->issue_event((BxiEQ*)md->md->eq_handle, event);
    }
}

BxiAtomicRequest::BxiAtomicRequest(BxiMD* md, ptl_size_t payload_size, bool matching, ptl_match_bits_t match_bits,
                                   ptl_pid_t target_pid, ptl_pt_index_t pt_index, void* user_ptr, bool service_vn,
                                   ptl_size_t local_offset, ptl_size_t remote_offset, ptl_ack_req_t ack_req,
                                   ptl_hdr_data_t hdr, ptl_op_t op, ptl_datatype_t datatype)
    : BxiPutRequest(md, payload_size, matching, match_bits, target_pid, pt_index, user_ptr, service_vn, local_offset,
                    remote_offset, ack_req, hdr)
    , op(op)
    , datatype(datatype)
{
    type = S4BXI_ATOMIC_REQUEST; // Overwrite what PUT constructor did
}

BxiFetchAtomicRequest::BxiFetchAtomicRequest(BxiMD* md, ptl_size_t payload_size, bool matching,
                                             ptl_match_bits_t match_bits, ptl_pid_t target_pid, ptl_pt_index_t pt_index,
                                             void* user_ptr, bool service_vn, ptl_size_t local_offset,
                                             ptl_size_t remote_offset, ptl_hdr_data_t hdr, ptl_op_t op,
                                             ptl_datatype_t datatype, BxiMD* get_md, ptl_size_t get_local_offset)
    : BxiAtomicRequest(md, payload_size, matching, match_bits, target_pid, pt_index, user_ptr, service_vn, local_offset,
                       remote_offset, PTL_NO_ACK_REQ /* unused, there will be a reply anyway */, hdr, op, datatype)
    , get_md(get_md)
    , get_local_offset(get_local_offset)
{
    type = S4BXI_FETCH_ATOMIC_REQUEST; // Overwrite what ATOMIC constructor did
}

BxiFetchAtomicRequest::~BxiFetchAtomicRequest()
{
    delete matched_me;
}

bool BxiFetchAtomicRequest::is_swap_request()
{
    switch (op) {
    case PTL_SWAP:
    case PTL_CSWAP:
    case PTL_CSWAP_NE:
    case PTL_CSWAP_LE:
    case PTL_CSWAP_LT:
    case PTL_CSWAP_GE:
    case PTL_CSWAP_GT:
    case PTL_MSWAP:
        return true;
    default:
        // Whatever, won't be used
        return false;
    }
}

BxiSwapRequest::BxiSwapRequest(BxiMD* md, ptl_size_t payload_size, bool matching, ptl_match_bits_t match_bits,
                               ptl_pid_t target_pid, ptl_pt_index_t pt_index, void* user_ptr, bool service_vn,
                               ptl_size_t local_offset, ptl_size_t remote_offset, ptl_hdr_data_t hdr, ptl_op_t op,
                               ptl_datatype_t datatype, BxiMD* get_md, ptl_size_t get_local_offset, const void* cst)
    : BxiFetchAtomicRequest(md, payload_size, matching, match_bits, target_pid, pt_index, user_ptr, service_vn,
                            local_offset, remote_offset, hdr, op, datatype, get_md, get_local_offset)
    , cst(cst)
{
} // No need to change the request type, BxiNicTarget knows how to deal with this in the FetchAtomic processing

/**
 * Copy / paste FetchAtomic destructor instead of using virtual destructors:
 * it's an ugly design but virtual calls are slow at runtime
 */
BxiSwapRequest::~BxiSwapRequest()
{
    delete matched_me;
}

BxiGetRequest::BxiGetRequest(BxiMD* md, ptl_size_t payload_size, bool matching, ptl_match_bits_t match_bits,
                             ptl_pid_t target_pid, ptl_pt_index_t pt_index, void* user_ptr, bool service_vn,
                             ptl_size_t local_offset, ptl_size_t remote_offset)
    : BxiRequest(S4BXI_GET_REQUEST, md, payload_size, matching, match_bits, target_pid, pt_index, user_ptr, service_vn,
                 local_offset, remote_offset)
{
}

BxiGetRequest::~BxiGetRequest()
{
    delete matched_me;
}