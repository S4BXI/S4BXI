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
#include "s4bxi/s4bxi_util.hpp"
#include "s4bxi/s4bxi_xbt_log.h"

using namespace std;
using namespace simgrid;

S4BXI_LOG_NEW_DEFAULT_CATEGORY(bxi_s4ptl_pt, "Messages specific to s4ptl PT implementation");

BxiPT::BxiPT(ptl_handle_ni_t ni_handle, ptl_handle_eq_t eq_handle, ptl_index_t index, unsigned int options)
    : ni((BxiNI*)ni_handle), eq((BxiEQ*)eq_handle), index(index), options(options)
{
    priority_list = new BxiList;
    overflow_list = new BxiList;
}

BxiPT::~BxiPT()
{
    delete priority_list;
    delete overflow_list;
}

/**
 * There should definitely be more verification before doing `*actual = desired;` here,
 * especially if desired = PTL_PT_ANY ...
 */
int BxiPT::alloc(ptl_handle_ni_t ni_handle, unsigned int options, ptl_handle_eq_t eq_handle, ptl_index_t desired,
                 ptl_index_t* actual)
{
    *actual = desired;
    auto ni = (BxiNI*)ni_handle;
    ni->pt_indexes.insert(pair<ptl_pt_index_t, BxiPT*>(*actual, new BxiPT(ni_handle, eq_handle, *actual, options)));

    return PTL_OK;
}

BxiPT* BxiPT::getFromNI(ptl_handle_ni_t ni_handle, ptl_pt_index_t pt_index)
{
    auto n = (BxiNI*)ni_handle;

    return n->pt_indexes.at(pt_index);
}

int BxiPT::enable()
{
    enabled = true;

    return PTL_OK;
}

int BxiPT::disable()
{
    enabled = false;

    return PTL_OK;
}

int BxiPT::free(ptl_handle_ni_t ni_handle, ptl_index_t pt_index)
{
    auto n    = (BxiNI*)ni_handle;
    BxiPT* pt = getFromNI(ni_handle, pt_index);

    // Is this dangerous ? I can't tell, I don't think so
    for (auto const& me : *pt->priority_list)
        delete me;
    for (auto const& me : *pt->overflow_list)
        delete me;
    for (auto const& header : pt->unexpected_headers)
        delete header;

    pt->priority_list->clear();
    pt->overflow_list->clear();
    pt->unexpected_headers.clear();
    n->pt_indexes.erase(pt_index);

    delete pt;

    return PTL_OK;
}

BxiME* BxiPT::walk_through_lists(BxiMsg* msg)
{
    if (!enabled) // This has probably already been checked, but who knows
        return nullptr;

    BxiRequest* req = msg->parent_request;
    for (auto me : *priority_list) {
        if (me->matches_request(req)) {
            me->used = true;

            return me;
        }
    }
    for (auto me : *overflow_list) {
        if (me->matches_request(req)) {
            me->used = true;
            ++msg->ref_count;
            unexpected_headers.push_back(msg);

            return me;
        }
    }

    return nullptr;
}

bool BxiPT::walk_through_UHs(BxiME* me)
{
    bool matched = false;

    for (int i = 0; i < unexpected_headers.size(); ++i) {
        auto header = unexpected_headers[i];
        auto req    = header->parent_request;

        if (me->matches_request(req)) {
            matched  = true;
            me->used = true;

            // DON'T update manage_local offset of the ME here even if you really want to (see sec 3.12 in spec)
            // also don't AUTO_UNLINK me : we simply don't insert it, so nothing to unlink

            auto ev = new ptl_event_t;
            switch (req->type) {
            case S4BXI_FETCH_ATOMIC_REQUEST:
                ev->type = PTL_EVENT_FETCH_ATOMIC_OVERFLOW;
                break;
            case S4BXI_ATOMIC_REQUEST:
                ev->type = PTL_EVENT_ATOMIC_OVERFLOW;
                break;
            case S4BXI_PUT_REQUEST:
                ev->type = PTL_EVENT_PUT_OVERFLOW;
                break;
            case S4BXI_GET_REQUEST:
                ev->type = PTL_EVENT_GET_OVERFLOW;
                break;
            default:
                ptl_panic("Incorrect request type found when walking through UH\n");
            }
            ev->initiator    = ptl_process_t{.phys{.nid = req->md->ni->node->nid, .pid = req->md->ni->pid}};
            ev->ni_fail_type = PTL_OK;
            ev->pt_index     = req->matched_me->pt->index;
            ev->user_ptr     = req->matched_me->user_ptr;
            ev->rlength      = req->payload_size;
            ev->mlength      = req->mlength;
            ev->match_bits   = req->match_bits;
            ev->start        = req->start;
            if (req->type != S4BXI_GET_REQUEST) // All requests other than GET look like a PUT
                ev->hdr_data = ((BxiPutRequest*)req)->hdr;

            unexpected_headers.erase(unexpected_headers.begin() + i--);
            BxiMsg::unref(header);

            ni->node->issue_event(eq, ev);

            if (HAS_PTL_OPTION(me->me, PTL_ME_USE_ONCE)) // Only match once if use_once ME
                return matched;
        }
    }

    return matched;
}