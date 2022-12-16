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
#include "s4bxi/s4bxi_util.hpp"
#include "s4bxi/s4bxi_xbt_log.h"

using namespace std;

S4BXI_LOG_NEW_DEFAULT_CATEGORY(bxi_s4ptl_me, "Messages specific to s4ptl ME implementation");

BxiME::BxiME(BxiPT* pt, const ptl_me_t* me_t, ptl_list_t list, void* user_ptr)
    : pt(pt), me(make_unique<ptl_me_t>(*me_t)), list(list), user_ptr(user_ptr)
{
}

BxiME::BxiME(const BxiME& me)
    : pt(me.pt)
    , me(make_unique<ptl_me_t>(*me.me))
    , list(me.list)
    , user_ptr(me.user_ptr)
    , manage_local_offset(me.manage_local_offset)
    , in_use(false)
    , needs_unlink(false)
{
}

/**
 * We need to pass the byte_count from the message manually,
 * because it could be smaller than the actual ME length
 *
 * @param byte_count Byte count from incoming message
 */
void BxiME::increment_ct(ptl_size_t byte_count)
{
    auto ct = (BxiCT*)me->ct_handle;
    if (ct == PTL_CT_NONE)
        return;

    auto amount = HAS_PTL_OPTION(me, PTL_ME_EVENT_CT_BYTES) ? byte_count : 1;
    ct->increment_success(amount);
}

/**
 * We need to traverse the unexpected list if appending to the priority list here, but this may prove far from trivial
 */
void BxiME::append(BxiPT* pt, const ptl_me_t* me_t, ptl_list_t list, void* user_ptr, ptl_handle_me_t* me_handle)
{
    auto me    = new BxiME(pt, me_t, list, user_ptr);
    *me_handle = me;

    bool matched_UH = false;

    // Check if we have corresponding unexpected headers already
    if (list == PTL_PRIORITY_LIST)
        matched_UH = pt->walk_through_UHs(me);

    if (matched_UH && HAS_PTL_OPTION(me->me, PTL_ME_USE_ONCE)) {
        delete me; // If USE_ONCE ME is consumed don't insert it (sec 3.12.2 in spec)
        return;    // therefore we will only have an OVERFLOW event (no LINK, no AUTO_UNLINK)
    }

    // Insert in appropriate list
    me->get_list(pt)->push_back(me);
    if (!HAS_PTL_OPTION(me_t, PTL_ME_EVENT_LINK_DISABLE)) {
        auto ev          = new ptl_event_t;
        ev->type         = PTL_EVENT_LINK;
        ev->ni_fail_type = PTL_OK;
        ev->user_ptr     = user_ptr;
        ev->pt_index     = me->pt->index;
        pt->ni->node->issue_event(pt->eq, ev);
    }
}

void BxiME::unlink(ptl_handle_me_t me_handle)
{
    auto me = (BxiME*)me_handle;

    if (me->in_use) { // Someone is using the ME, mark it as "needs to be unlinked" only
        me->needs_unlink = true;
        return;
    }

    // No one is using the ME, destroy it
    auto list = me->get_list();
    list->erase(remove(list->begin(), list->end(), me));
    delete me;
}

bool BxiME::matches_request(BxiRequest* req)
{
    if (HAS_PTL_OPTION(me, PTL_ME_USE_ONCE) && used)
        return false;

    // LE case : take the first one
    if (!req->matching)
        return true;

    // ME case : matching algorithm directly copy-pasted from Portals 4.1 sec 3.12
    if (((req->match_bits ^ me->match_bits) & ~me->ignore_bits) == 0)
        return true;

    return false;
}

ptl_addr_t BxiME::get_offsetted_addr(BxiMsg* msg, bool update_manage_local_offset)
{
    BxiRequest* req = msg->parent_request;
    // Simple case, use requested remote offset
    if (!HAS_PTL_OPTION(me, PTL_ME_MANAGE_LOCAL)) {
        req->target_remote_offset = req->remote_offset;
        return (char*)me->start + req->remote_offset;
    }

    // MANAGE_LOCAL case, see spec sec 3.12.1 about this option
    ptl_addr_t addr           = (char*)me->start + manage_local_offset;
    req->target_remote_offset = manage_local_offset; // Overwrite the requested offset
    if (update_manage_local_offset) {
        manage_local_offset += req->payload_size;
        manage_local_offset = manage_local_offset <= me->length ? manage_local_offset : me->length;
    }

    return addr;
}

shared_ptr<BxiList> BxiME::get_list()
{
    return get_list(pt);
}

/**
 * Used in case the PT has not been set in the ME yet
 */
shared_ptr<BxiList> BxiME::get_list(BxiPT* considered_pt)
{
    return list == PTL_PRIORITY_LIST ? considered_pt->priority_list : considered_pt->overflow_list;
}

/**
 * Get the quantity of data actually copied at target
 * This is used for potentially truncated payloads
 */
ptl_size_t BxiME::get_mlength(const BxiRequest* req)
{
    ptl_size_t remaining_size = me->length - manage_local_offset;
    return remaining_size < req->payload_size ? remaining_size : req->payload_size;
}

bool BxiME::maybe_auto_unlink(BxiME* me)
{
    bool should_unlink = HAS_PTL_OPTION(me->me, PTL_ME_USE_ONCE) // Simple case: flag is set
                         || (HAS_PTL_OPTION(me->me, PTL_ME_MANAGE_LOCAL) &&
                             (me->manage_local_offset + me->me->min_free > me->me->length));
    //                       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    //                       MANAGE_LOCAL: unlink if we reached the end of the ME's memory

    if (!should_unlink)
        return false;

    auto eq       = me->pt->eq;
    auto user_ptr = me->user_ptr;
    auto pt       = me->pt;
    bool should_log_event =
        !HAS_PTL_OPTION(me->me, PTL_ME_EVENT_UNLINK_DISABLE) && !HAS_PTL_OPTION(me->me, PTL_ME_EVENT_SUCCESS_DISABLE);

    BxiME::unlink(me);

    // Log event if flag is not set
    if (should_log_event) {
        auto event          = new ptl_event_t;
        event->type         = PTL_EVENT_AUTO_UNLINK;
        event->ni_fail_type = PTL_OK;
        event->pt_index     = pt->index;
        event->user_ptr     = user_ptr;
        pt->ni->node->issue_event(eq, event);
    }

    return true;
}