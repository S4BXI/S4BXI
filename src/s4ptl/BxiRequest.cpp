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