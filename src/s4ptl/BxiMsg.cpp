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

#include "s4bxi/s4ptl.hpp"
#include "s4bxi/BxiEngine.hpp"
#include "s4bxi/s4bxi_xbt_log.h"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(bxi_s4ptl_msg, "Messages specific to s4ptl msg implementation");

BxiMsg::BxiMsg(ptl_nid_t initiator, ptl_nid_t target, bxi_msg_type type, ptl_size_t simulated_size,
               BxiRequest* parent_request)
    : initiator(initiator)
    , target(target)
    , type(type)
    , simulated_size(simulated_size)
    , retry_count(0)
    , parent_request(parent_request)
    , ref_count(1)
{
    ++parent_request->msg_ref_count;
}

BxiMsg::BxiMsg(const BxiMsg& msg)
    : initiator(msg.initiator)
    , target(msg.target)
    , type(msg.type)
    , send_init_time(msg.send_init_time)
    , simulated_size(msg.simulated_size)
    , retry_count(msg.retry_count)
    , parent_request(msg.parent_request)
    , ref_count(1)
    , bxi_log(nullptr)
{
    ++parent_request->msg_ref_count;
}

/**
 * Thanks to this switch we don't need to have virtual Request destructors, which is faster at runtime
 */
BxiMsg::~BxiMsg()
{
    if (answers_msg)
        unref(answers_msg);

    if (!--parent_request->msg_ref_count) {
        switch (parent_request->type) {
        case S4BXI_FETCH_ATOMIC_REQUEST:
            delete (BxiFetchAtomicRequest*)parent_request;
            break;
        case S4BXI_ATOMIC_REQUEST:
            delete (BxiAtomicRequest*)parent_request;
            break;
        case S4BXI_PUT_REQUEST:
            delete (BxiPutRequest*)parent_request;
            break;
        case S4BXI_GET_REQUEST:
            delete (BxiGetRequest*)parent_request;
            break;
        }
    }
}

void BxiMsg::unref(BxiMsg* msg)
{
    if (!--msg->ref_count)
        delete msg;
}

bxi_vn BxiMsg::get_vn() const
{
    int num =
        (type == S4BXI_PTL_PUT || type == S4BXI_PTL_GET || type == S4BXI_PTL_ATOMIC || type == S4BXI_PTL_FETCH_ATOMIC)
            ? S4BXI_VN_SERVICE_REQUEST
            : S4BXI_VN_SERVICE_RESPONSE;

    if (!parent_request->service_vn)
        num += 1; // Switch to compute version

    assert(num >= 0 && num < 4);

    return (bxi_vn)num;
}
