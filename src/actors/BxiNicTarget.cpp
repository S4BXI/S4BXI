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

#include "s4bxi/actors/BxiNicTarget.hpp"

#include <xbt.h>
#include <complex.h>
#include "s4bxi/s4bxi_xbt_log.h"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_nic_target, "Messages specific to the NIC target");

BxiNicTarget::BxiNicTarget(const vector<string>& args) : BxiNicActor(args)
{
    nic_rx_mailbox = s4u::Mailbox::by_name(nic_rx_mailbox_name(vn));
    nic_rx_mailbox->set_receiver(self);
}

/**
 * Processing receive logic of NIC
 *
 * It is OK to have an infinite loop since this actor is daemonized
 */
void BxiNicTarget::operator()()
{
    // TX VN is always RESPONSE, we just need to determine SERVICE / COMPUTE
    bxi_vn tx_vn = (vn == S4BXI_VN_SERVICE_REQUEST || vn == S4BXI_VN_SERVICE_RESPONSE) ? S4BXI_VN_SERVICE_RESPONSE
                                                                                       : S4BXI_VN_COMPUTE_RESPONSE;
    do {
        tx_queue = node->tx_queues[tx_vn];
        s4u::this_actor::yield();
    } while (!tx_queue);

    for (;;) {
        auto msg = nic_rx_mailbox->get<BxiMsg>();
        switch (msg->type) {
        case S4BXI_PTL_PUT:
            handle_put_request(msg);
            break;
        case S4BXI_PTL_GET:
            handle_get_request(msg);
            break;
        case S4BXI_PTL_ATOMIC:
            handle_atomic_request(msg);
            break;
        case S4BXI_PTL_FETCH_ATOMIC:
            handle_fetch_atomic_request(msg);
            break;
        case S4BXI_PTL_RESPONSE:
        case S4BXI_PTL_FETCH_ATOMIC_RESPONSE:
            handle_response(msg, msg->parent_request->md);
            break;
        case S4BXI_PTL_ACK:
            handle_ptl_ack(msg);
            break;
        case S4BXI_E2E_ACK:
            handle_bxi_ack(msg);
            break;
        }

        BxiMsg::unref(msg);
    }
}

/**
 * Handles a Put request
 *
 * Write data to memory, issue event, send ACK, etc.
 *
 * When checking options, we don't need to differentiate ME / LE case :
 * each PTL_LE_XXXXX is defined as an alias for PTL_ME_XXXXX (see portals4.h)
 */
void BxiNicTarget::handle_put_request(BxiMsg* msg)
{
    auto req = (BxiPutRequest*)msg->parent_request;

    if (req->process_state > S4BXI_REQ_CREATED)
        return; // Don't process the same message twice

    BxiME* me = match_entry(msg);

    if (me) {
        if (me->list == PTL_OVERFLOW_LIST) // We won't need it if it matched on PRIORITY_LIST
            req->matched_me = new BxiME(*me);

        req->process_state = S4BXI_REQ_RECEIVED;

        req->mlength = me->get_mlength(req);

        BxiMD* md  = req->md;
        req->start = me->get_offsetted_addr(msg, true);
        if (S4BXI_CONFIG_AND(node, use_real_memory) && md->md->length)
            // Here we could copy only the pointer if this piece of memory is read but not written
            capped_memcpy(req->start, (unsigned char*)md->md->start + req->local_offset, req->mlength);

        if (HAS_PTL_OPTION(me->me, PTL_ME_EVENT_CT_COMM))
            me->increment_ct(req->payload_size);

        bool need_portals_ack = !HAS_PTL_OPTION(me->me, PTL_ME_ACK_DISABLE) && req->ack_req != PTL_NO_ACK_REQ;
        bool need_ack         = need_portals_ack || !S4BXI_CONFIG_OR(md->ni->node, e2e_off);

        BxiMsg* ack = nullptr;
        if (need_ack && !S4BXI_GLOBAL_CONFIG(quick_acks)) {
            ack       = new BxiMsg(*msg);
            ack->type = need_portals_ack ? S4BXI_PTL_ACK : S4BXI_E2E_ACK;
            // If no Portals ACK needs to be sent, we still need to send an E2E ACK, which will fast-forward
            // the state of the request to BXI_MSG_ACKED when it will be received at the initiator
            ack->initiator      = msg->target;
            ack->target         = msg->initiator;
            ack->simulated_size = ACK_SIZE;
            ack->retry_count    = 0;

            // DON'T SEND THE ACK YET !
            // Sending the ack will yield to SimGrid, and another process could be scheduled (very rare since
            // sending the ack doesn't take any time, but it definitely does happen sometimes), for example
            // a process in the middle of a ME unlink, which means that the `me` we're currently using could
            // be anihilated before we get to the next line
        }

        if (!HAS_PTL_OPTION(me->me, PTL_ME_EVENT_COMM_DISABLE) &&
            !HAS_PTL_OPTION(me->me, PTL_ME_EVENT_SUCCESS_DISABLE) &&
            me->list == PTL_PRIORITY_LIST) { // OVERFLOW ME will have a PUT_OVERFLOW later
            auto eq              = me->pt->eq;
            auto event           = new ptl_event_t;
            event->initiator     = ptl_process_t{.phys{.nid = req->md->ni->node->nid, .pid = req->md->ni->pid}};
            event->type          = PTL_EVENT_PUT;
            event->ni_fail_type  = PTL_OK;
            event->pt_index      = me->pt->index;
            event->user_ptr      = me->user_ptr;
            event->hdr_data      = req->hdr;
            event->rlength       = req->payload_size;
            event->mlength       = req->mlength;
            event->remote_offset = req->remote_offset;
            event->match_bits    = req->match_bits;
            event->start         = req->start;

            // We need to auto_unlink at this precise moment, otherwise on rare
            // occasions the ME could be already gone by the time we check if
            // it has the "auto unlink" option (if the user got the event and
            // manually unlinked the ME before this actor was scheduled again
            // and executed this). So if we were to put the auto_unlink after
            // the issue_event, we could have an invalid read when doing
            // `HAS_PTL_OPTION(...)` at the beginning of maybe_auto_unlink_me
            // (thanks valgrind for pointing this out)
            BxiME::maybe_auto_unlink(me);
            // ... But by doing this the "auto unlink" event is issued before
            // the "put" event, I don't know if it matters or not (I'm not
            // event sure of how the real world NIC does this)

            issue_event(eq, event);

            // Simulate the PCI transfer to write data to memory (thanks frs69wq for the idea)
            if (S4BXI_CONFIG_AND(node, model_pci) && msg->simulated_size) {
                pci_transfer_async(msg->simulated_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_PAYLOAD_WRITE);
            }
        } else {
            BxiME::maybe_auto_unlink(me);
        }

        if (need_ack) {
            if (S4BXI_GLOBAL_CONFIG(quick_acks)) {
                // Fast-forward this request, we're not sending real ACKs (neither PTL nor BXI)
                req->process_state = S4BXI_REQ_FINISHED;
                // Thanks to simulated world's magic, we can trigger the ACK and / or SEND at the initiator side
                // although we're currently processing the message at the target side.
                req->md->ni->node->release_e2e_entry();
                req->maybe_issue_send();
                req->issue_ack();
            } else {
                tx_queue->put(ack, 0, true);
            }
        }
    } // else {} ? We should probably do something if no entry was found
}

/**
 * Handles a Get request
 *
 * Read data from memory, issue event, send response, etc.
 *
 * When checking options, we don't need to differentiate ME / LE case :
 * each PTL_LE_XXXXX is defined as an alias for PTL_ME_XXXXX (see portals4.h)
 */
void BxiNicTarget::handle_get_request(BxiMsg* msg)
{
    auto req = (BxiGetRequest*)msg->parent_request;

    if (req->process_state > S4BXI_REQ_CREATED)
        return; // Don't process the same message twice

    BxiME* me = match_entry(msg);

    if (me) {
        req->process_state = S4BXI_REQ_RECEIVED;
        req->matched_me    = new BxiME(*me);
        req->mlength       = me->get_mlength(req);
        req->start         = me->get_offsetted_addr(msg, true);

        if (HAS_PTL_OPTION(me->me, PTL_ME_EVENT_CT_COMM))
            me->increment_ct(req->payload_size);

        auto response            = new BxiMsg(*msg);
        response->type           = S4BXI_PTL_RESPONSE;
        response->initiator      = msg->target;
        response->target         = msg->initiator;
        response->retry_count    = 0;
        response->simulated_size = req->mlength;

        tx_queue->put(response, 0, true);

        if (S4BXI_CONFIG_AND(req->md->ni->node, use_real_memory) && me->me->length)
            capped_memcpy((unsigned char*)req->md->md->start + req->local_offset, req->start, req->mlength);

        // GET event isn't here, it will be issued by the initiator actor when the response is sent on the BXI cable
        BxiME::maybe_auto_unlink(me);
    } // else {} ? We should probably do something if no entry was found

    // Same remark as in PtlPut
}

/**
 * Handles an Atomic request
 *
 * Very similar to Put processing, should probably be factorized
 */
void BxiNicTarget::handle_atomic_request(BxiMsg* msg)
{
    auto req = (BxiAtomicRequest*)msg->parent_request;

    if (req->process_state > S4BXI_REQ_CREATED)
        return; // Don't process the same message twice

    BxiME* me = match_entry(msg);

    if (me) {
        if (me->list == PTL_OVERFLOW_LIST) // We won't need it if it matched on PRIORITY_LIST
            req->matched_me = new BxiME(*me);

        req->process_state = S4BXI_REQ_RECEIVED;
        req->mlength       = me->get_mlength(req);

        BxiMD* md  = req->md;
        req->start = me->get_offsetted_addr(msg, true);
        if (S4BXI_CONFIG_AND(node, use_real_memory) && md->md->length)
            apply_atomic_op(req->op, req->datatype, (unsigned char*)req->start,
                            (unsigned char*)md->md->start + req->local_offset,
                            (unsigned char*)md->md->start + req->local_offset,
                            (unsigned char*)req->start /* to have a noop memcpy */, req->mlength);

        if (HAS_PTL_OPTION(me->me, PTL_ME_EVENT_CT_COMM))
            me->increment_ct(req->payload_size);

        bool need_portals_ack = !HAS_PTL_OPTION(me->me, PTL_ME_ACK_DISABLE) && req->ack_req != PTL_NO_ACK_REQ;

        if (need_portals_ack || !S4BXI_CONFIG_OR(md->ni->node, e2e_off)) {
            if (need_portals_ack && S4BXI_GLOBAL_CONFIG(quick_acks)) {
                // Fast-forward this request, we're not sending real ACKs (neither PTL nor BXI)
                req->process_state = S4BXI_REQ_FINISHED;
                // Thanks to simulated world's magic, we can trigger the ACK and / or SEND at the initiator side
                // although we're currently processing the message at the target side.
                req->md->ni->node->release_e2e_entry();
                req->maybe_issue_send();
                req->issue_ack();
            } else {
                auto ack  = new BxiMsg(*msg);
                ack->type = need_portals_ack ? S4BXI_PTL_ACK : S4BXI_E2E_ACK;
                // If no Portals ACK needs to be sent, we still need to send an E2E ACK, which will fast-forward
                // the state of the request to BXI_MSG_ACKED when it will be received at the initiator
                ack->initiator      = msg->target;
                ack->target         = msg->initiator;
                ack->simulated_size = ACK_SIZE;
                ack->retry_count    = 0;

                tx_queue->put(ack, 0, true);
            }
        }

        if (!HAS_PTL_OPTION(me->me, PTL_ME_EVENT_COMM_DISABLE) &&
            !HAS_PTL_OPTION(me->me, PTL_ME_EVENT_SUCCESS_DISABLE) &&
            me->list == PTL_PRIORITY_LIST) { // OVERFLOW ME will have an ATOMIC_OVERFLOW later
            auto eq              = me->pt->eq;
            auto event           = new ptl_event_t;
            event->initiator     = ptl_process_t{.phys{.nid = req->md->ni->node->nid, .pid = req->md->ni->pid}};
            event->type          = PTL_EVENT_ATOMIC;
            event->ni_fail_type  = PTL_OK;
            event->pt_index      = me->pt->index;
            event->user_ptr      = me->user_ptr;
            event->hdr_data      = req->hdr;
            event->rlength       = req->payload_size;
            event->mlength       = req->mlength;
            event->remote_offset = req->remote_offset;
            event->match_bits    = req->match_bits;
            event->start         = req->start;

            // See comment for put
            BxiME::maybe_auto_unlink(me);

            issue_event(eq, event);

            // Simulate the PCI transfer to write data to memory (thanks frs69wq for the idea)
            if (S4BXI_CONFIG_AND(node, model_pci) && msg->simulated_size)
                pci_transfer_async(msg->simulated_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_PAYLOAD_WRITE);
        } else {
            BxiME::maybe_auto_unlink(me);
        }
    } // See comment for put and get
}

/**
 * Handles a Fetch Atomic request
 *
 * This is actually a mix of Atomic and Get, therefore the processing
 * for Atomic is more similar to Put than to FetchAtomic, which in a
 * sense is weird
 *
 * It's also here that we handle SWAP operations, since they are just
 * a special case of fetch atomic (according to Portals spec)
 */
void BxiNicTarget::handle_fetch_atomic_request(BxiMsg* msg)
{
    auto req = (BxiFetchAtomicRequest*)msg->parent_request;

    if (req->process_state > S4BXI_REQ_CREATED)
        return; // Don't process the same message twice

    BxiME* me = match_entry(msg);

    if (me) {
        req->process_state = S4BXI_REQ_RECEIVED;

        BxiMD* md       = req->md;
        req->matched_me = new BxiME(*me);
        req->mlength    = me->get_mlength(req);
        req->start      = me->get_offsetted_addr(msg, true);
        if (S4BXI_CONFIG_AND(node, use_real_memory) && md->md->length) {
            if (me->me->length)
                capped_memcpy((unsigned char*)req->get_md->md->start + req->get_local_offset, req->start, req->mlength);

            unsigned char* cst = req->is_swap_request()
                                     ? (unsigned char*)&((BxiSwapRequest*)req)->cst
                                     : (unsigned char*)md->md->start + req->local_offset; // Whatever, won't be used

            apply_atomic_op(
                req->op, req->datatype, (unsigned char*)req->start, cst,
                (unsigned char*)md->md->start + req->local_offset,
                (unsigned char*)req->matched_me->get_offsetted_addr(msg, false), // <<< I'm so unsure about this
                req->mlength);
        }

        if (HAS_PTL_OPTION(me->me, PTL_ME_EVENT_CT_COMM))
            me->increment_ct(req->payload_size);

        auto response            = new BxiMsg(*msg);
        response->type           = S4BXI_PTL_FETCH_ATOMIC_RESPONSE;
        response->initiator      = msg->target;
        response->target         = msg->initiator;
        response->simulated_size = req->mlength;
        response->retry_count    = 0;

        tx_queue->put(response, 0, true);

        // FETCH_ATOMIC event isn't here, it will be issued by the initiator
        // actor when the response is sent on the BXI cable
        BxiME::maybe_auto_unlink(me);
    } // See comment for put and get
}

void BxiNicTarget::handle_response(BxiMsg* msg, BxiMD* md)
{
    node->release_e2e_entry();
    BxiRequest* req = msg->parent_request;

    if (req->process_state > S4BXI_REQ_RECEIVED)
        return;

    req->process_state = S4BXI_REQ_ANSWERED;

    // Simulate the PCI transfer to write data to memory
    if (S4BXI_CONFIG_AND(node, model_pci) && msg->simulated_size)
        pci_transfer_async(msg->simulated_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_PAYLOAD_WRITE);

    if (!S4BXI_CONFIG_OR(md->ni->node, e2e_off)) {
        auto bxi_ack            = new BxiMsg(*msg);
        bxi_ack->type           = S4BXI_E2E_ACK;
        bxi_ack->initiator      = msg->target;
        bxi_ack->target         = msg->initiator;
        bxi_ack->simulated_size = ACK_SIZE;

        tx_queue->put(bxi_ack, 0, true);
    }

    if (HAS_PTL_OPTION(md->md, PTL_MD_EVENT_CT_REPLY))
        md->increment_ct(req->payload_size);

    auto reply_evt           = new ptl_event_t;
    reply_evt->type          = PTL_EVENT_REPLY;
    reply_evt->ni_fail_type  = PTL_OK;
    reply_evt->user_ptr      = req->user_ptr;
    reply_evt->mlength       = req->mlength;
    reply_evt->remote_offset = req->target_remote_offset;
    issue_event((BxiEQ*)md->md->eq_handle, reply_evt);
}

void BxiNicTarget::handle_ptl_ack(BxiMsg* msg)
{
    node->release_e2e_entry();
    auto req = (BxiPutRequest*)msg->parent_request;

    if (!S4BXI_CONFIG_OR(req->md->ni->node, e2e_off)) {
        auto bxi_ack            = new BxiMsg(*msg);
        bxi_ack->type           = S4BXI_E2E_ACK;
        bxi_ack->initiator      = msg->target;
        bxi_ack->target         = msg->initiator;
        bxi_ack->simulated_size = ACK_SIZE;

        tx_queue->put(bxi_ack, 0, true);
    }

    if (req->process_state <= S4BXI_REQ_RECEIVED) {
        req->process_state = S4BXI_REQ_ANSWERED;

        req->maybe_issue_send();

        req->issue_ack();
    }
}

/**
 * Processing of a BXI ACK for E2E reliability
 *
 * We only need to update the request's status, so that it is simply ignored when E2E processes it
 * (at the end of the timeout)
 *
 * @param msg Incoming BXI ACK
 */
void BxiNicTarget::handle_bxi_ack(BxiMsg* msg)
{
    node->release_e2e_entry();
    msg->parent_request->process_state = S4BXI_REQ_FINISHED;

    // If we have a PUT request, check that the send event was issued at some point
    // (If not, it means we have a put that's not buffered, and we didn't ask for a portals ACK)
    if (msg->parent_request->type == S4BXI_PUT_REQUEST) {
        auto req = (BxiPutRequest*)msg->parent_request;
        if (req->ack_req == PTL_NO_ACK_REQ)
            req->maybe_issue_send();
    } else if (msg->parent_request->type == S4BXI_GET_REQUEST) {
        auto req = (BxiGetRequest*)msg->parent_request;
        maybe_issue_get(req);
    } else if (msg->parent_request->type == S4BXI_FETCH_ATOMIC_REQUEST) {
        auto req = (BxiFetchAtomicRequest*)msg->parent_request;
        maybe_issue_fetch_atomic(req);
    }
}

/**
 * Entry matching logic, works for both MEs and LEs
 *
 * That's a lot of nested `for`s, but usually we will have only one ni and one pt, so it will be fast
 *
 * @param msg Incoming message
 * @param matching Whether we are trying to match a LE or a ME
 * @return Pointer to the entry, or nullptr if none found
 */
BxiME* BxiNicTarget::match_entry(BxiMsg* msg)
{
    auto req = msg->parent_request;
    BxiME* me;
    for (auto ni : node->ni_handles) {
        if (!ni->can_match_request(req))
            continue; // The NI doesn't correspond, don't even look at what's inside

        if (req->pt_index == PTL_PT_ANY) {
            // I don't know if a request with PTL_PT_ANY is legal, specification doesn't
            // say anything about this, but I have the feeling that it should
            for (auto pt : ni->pt_indexes) {
                if (!pt.second->enabled)
                    continue; // I really don't know if that's what I should do here

                if ((me = pt.second->walk_through_lists(msg))) // Much parentheses to please clang
                    return me;
            }
        } else {
            // Check if the requested PT exists in the NI
            if (!ni->pt_indexes.count(req->pt_index))
                return nullptr;

            auto pt = ni->pt_indexes[req->pt_index];

            if (!pt->enabled)
                return nullptr; // I really don't know if that's what I should do here (maybe
                                // issue a PTL_PT_DISABLED event if flow control is active ?)

            if ((me = pt->walk_through_lists(msg)))
                return me;
        }
    }

    return nullptr;
}

/**
 * Memcpy at most the number of bytes defined in global conf
 */
void BxiNicTarget::capped_memcpy(void* dest, const void* src, size_t n)
{
    long max_memcpy = S4BXI_GLOBAL_CONFIG(max_memcpy);
    size_t to_copy  = max_memcpy == -1 ? n : (max_memcpy < n ? max_memcpy : n);
    xbt_assert(!to_copy || dest != nullptr && src != nullptr, "\n\nCan't copy user data from %p to %p\n", dest, src);
    if (to_copy)
        memcpy(dest, src, to_copy);
}

/*
 * Perform the given atomic operation
 *
 *	op - the operation
 *	type - type of the operands
 *	me - pointer to the original operand
 *	rx - pointer to the operand received from the network
 *	tx - result to send in reply on the network
 *	cst - third operand (swap only)
 *	len - size of the array of operands in bytes
 *
 * Imported from swptl and adapted to C++
 */
void BxiNicTarget::apply_atomic_op(int op, int type, unsigned char* me, unsigned char* cst, unsigned char* rx,
                                   unsigned char* tx, size_t len)
{
    int i, j, n, asize, eq, le, ge, swap;

    asize = ptl_atsize((ptl_datatype)type);
    if (len % asize != 0)
        ptl_panic_fmt("%zu: atomic size not multiple of %d\n", len, asize);

    n = len / asize;

    for (i = 0; i < n; i++) {
        if (tx != me) // Don't do useless memcpies
            memcpy(tx, me, asize);
        switch (op) {
        case PTL_MIN:
            switch (type) {
            case PTL_INT8_T:
                if (*(int8_t*)me > *(int8_t*)rx)
                    *(int8_t*)me = *(int8_t*)rx;
                break;
            case PTL_UINT8_T:
                if (*(uint8_t*)me > *(uint8_t*)rx)
                    *(uint8_t*)me = *(uint8_t*)rx;
                break;
            case PTL_INT16_T:
                if (*(int16_t*)me > *(int16_t*)rx)
                    *(int16_t*)me = *(int16_t*)rx;
                break;
            case PTL_UINT16_T:
                if (*(uint16_t*)me > *(uint16_t*)rx)
                    *(uint16_t*)me = *(uint16_t*)rx;
                break;
            case PTL_INT32_T:
                if (*(int32_t*)me > *(int32_t*)rx)
                    *(int32_t*)me = *(int32_t*)rx;
                break;
            case PTL_UINT32_T:
                if (*(uint32_t*)me > *(uint32_t*)rx)
                    *(uint32_t*)me = *(uint32_t*)rx;
                break;
            case PTL_INT64_T:
                if (*(int64_t*)me > *(int64_t*)rx)
                    *(int64_t*)me = *(int64_t*)rx;
                break;
            case PTL_UINT64_T:
                if (*(uint64_t*)me > *(uint64_t*)rx)
                    *(uint64_t*)me = *(uint64_t*)rx;
                break;
            case PTL_FLOAT:
                if (*(float*)me > *(float*)rx)
                    *(float*)me = *(float*)rx;
                break;
            case PTL_DOUBLE:
                if (*(double*)me > *(double*)rx)
                    *(double*)me = *(double*)rx;
                break;
            case PTL_LONG_DOUBLE:
                if (*(long double*)me > *(long double*)rx)
                    *(long double*)me = *(long double*)rx;
                break;
            default:
                ptl_panic("unhandled atomic type\n");
            }
            break;
        case PTL_MAX:
            switch (type) {
            case PTL_INT8_T:
                if (*(int8_t*)me < *(int8_t*)rx)
                    *(int8_t*)me = *(int8_t*)rx;
                break;
            case PTL_UINT8_T:
                if (*(uint8_t*)me < *(uint8_t*)rx)
                    *(uint8_t*)me = *(uint8_t*)rx;
                break;
            case PTL_INT16_T:
                if (*(int16_t*)me < *(int16_t*)rx)
                    *(int16_t*)me = *(int16_t*)rx;
                break;
            case PTL_UINT16_T:
                if (*(uint16_t*)me < *(uint16_t*)rx)
                    *(uint16_t*)me = *(uint16_t*)rx;
                break;
            case PTL_INT32_T:
                if (*(int32_t*)me < *(int32_t*)rx)
                    *(int32_t*)me = *(int32_t*)rx;
                break;
            case PTL_UINT32_T:
                if (*(uint32_t*)me < *(uint32_t*)rx)
                    *(uint32_t*)me = *(uint32_t*)rx;
                break;
            case PTL_INT64_T:
                if (*(int64_t*)me < *(int64_t*)rx)
                    *(int64_t*)me = *(int64_t*)rx;
                break;
            case PTL_UINT64_T:
                if (*(uint64_t*)me < *(uint64_t*)rx)
                    *(uint64_t*)me = *(uint64_t*)rx;
                break;
            case PTL_FLOAT:
                if (*(float*)me < *(float*)rx)
                    *(float*)me = *(float*)rx;
                break;
            case PTL_DOUBLE:
                if (*(double*)me < *(double*)rx)
                    *(double*)me = *(double*)rx;
                break;
            case PTL_LONG_DOUBLE:
                if (*(long double*)me <= *(long double*)rx)
                    *(long double*)me = *(long double*)rx;
                break;
            default:
                ptl_panic("unhandled atomic type\n");
            }
            break;
        case PTL_SUM:
            switch (type) {
            case PTL_INT8_T:
            case PTL_UINT8_T:
                *(int8_t*)me += *(int8_t*)rx;
                break;
            case PTL_INT16_T:
            case PTL_UINT16_T:
                *(int16_t*)me += *(int16_t*)rx;
                break;
            case PTL_INT32_T:
            case PTL_UINT32_T:
                *(int32_t*)me += *(int32_t*)rx;
                break;
            case PTL_INT64_T:
            case PTL_UINT64_T:
                *(int64_t*)me += *(int64_t*)rx;
                break;
            case PTL_FLOAT:
                *(float*)me += *(float*)rx;
                break;
            case PTL_FLOAT_COMPLEX:
                // C's `complex` keyword is not legal in C++, but `_Complex` is equivalent
                // (https://stackoverflow.com/questions/10540228/c-complex-numbers-in-c#answer-10540302)
                *(float _Complex*)me += *(float _Complex*)rx;
                break;
            case PTL_DOUBLE:
                *(double*)me += *(double*)rx;
                break;
            case PTL_DOUBLE_COMPLEX:
                *(double _Complex*)me += *(double _Complex*)rx;
                break;
            case PTL_LONG_DOUBLE:
                *(long double*)me += *(long double*)rx;
                break;
            case PTL_LONG_DOUBLE_COMPLEX:
                *(long double _Complex*)me += *(long double _Complex*)rx;
                break;
            default:
                ptl_panic("unhandled atomic type\n");
            }
            break;
        case PTL_PROD:
            switch (type) {
            case PTL_INT8_T:
                *(int8_t*)me *= *(int8_t*)rx;
                break;
            case PTL_UINT8_T:
                *(uint8_t*)me *= *(uint8_t*)rx;
                break;
            case PTL_INT16_T:
                *(int16_t*)me *= *(int16_t*)rx;
                break;
            case PTL_UINT16_T:
                *(uint16_t*)me *= *(uint16_t*)rx;
                break;
            case PTL_INT32_T:
                *(int32_t*)me *= *(int32_t*)rx;
                break;
            case PTL_UINT32_T:
                *(uint32_t*)me *= *(uint32_t*)rx;
                break;
            case PTL_INT64_T:
                *(int64_t*)me *= *(int64_t*)rx;
                break;
            case PTL_UINT64_T:
                *(uint64_t*)me *= *(uint64_t*)rx;
                break;
            case PTL_FLOAT:
                *(float*)me *= *(float*)rx;
                break;
            case PTL_FLOAT_COMPLEX:
                *(float _Complex*)me *= *(float _Complex*)rx;
                break;
            case PTL_DOUBLE:
                *(double*)me *= *(double*)rx;
                break;
            case PTL_DOUBLE_COMPLEX:
                *(double _Complex*)me *= *(double _Complex*)rx;
                break;
            case PTL_LONG_DOUBLE:
                *(long double*)me *= *(long double*)rx;
                break;
            case PTL_LONG_DOUBLE_COMPLEX:
                *(long double _Complex*)me *= *(long double _Complex*)rx;
                break;
            default:
                ptl_panic("unhandled atomic type\n");
            }
            break;
        case PTL_BOR:
            switch (type) {
            case PTL_INT8_T:
            case PTL_UINT8_T:
                *(int8_t*)me = *(int8_t*)me | *(int8_t*)rx;
                break;
            case PTL_INT16_T:
            case PTL_UINT16_T:
                *(int16_t*)me = *(int16_t*)me | *(int16_t*)rx;
                break;
            case PTL_INT32_T:
            case PTL_UINT32_T:
                *(int32_t*)me = *(int32_t*)me | *(int32_t*)rx;
                break;
            case PTL_INT64_T:
            case PTL_UINT64_T:
                *(int64_t*)me = *(int64_t*)me | *(int64_t*)rx;
                break;
            default:
                ptl_panic("unhandled atomic type\n");
            }
            break;
        case PTL_BAND:
            switch (type) {
            case PTL_INT8_T:
            case PTL_UINT8_T:
                *(int8_t*)me = *(int8_t*)me & *(int8_t*)rx;
                break;
            case PTL_INT16_T:
            case PTL_UINT16_T:
                *(int16_t*)me = *(int16_t*)me & *(int16_t*)rx;
                break;
            case PTL_INT32_T:
            case PTL_UINT32_T:
                *(int32_t*)me = *(int32_t*)me & *(int32_t*)rx;
                break;
            case PTL_INT64_T:
            case PTL_UINT64_T:
                *(int64_t*)me = *(int64_t*)me & *(int64_t*)rx;
                break;
            default:
                ptl_panic("unhandled atomic type\n");
            }
            break;
        case PTL_BXOR:
            switch (type) {
            case PTL_INT8_T:
            case PTL_UINT8_T:
                *(int8_t*)me = *(int8_t*)me ^ *(int8_t*)rx;
                break;
            case PTL_INT16_T:
            case PTL_UINT16_T:
                *(int16_t*)me = *(int16_t*)me ^ *(int16_t*)rx;
                break;
            case PTL_INT32_T:
            case PTL_UINT32_T:
                *(int32_t*)me = *(int32_t*)me ^ *(int32_t*)rx;
                break;
            case PTL_INT64_T:
            case PTL_UINT64_T:
                *(int64_t*)me = *(int64_t*)me ^ *(int64_t*)rx;
                break;
            default:
                ptl_panic("unhandled atomic type\n");
            }
            break;
        case PTL_LOR:
            switch (type) {
            case PTL_INT8_T:
            case PTL_UINT8_T:
                *(int8_t*)me = *(int8_t*)me || *(int8_t*)rx;
                break;
            case PTL_INT16_T:
            case PTL_UINT16_T:
                *(int16_t*)me = *(int16_t*)me || *(int16_t*)rx;
                break;
            case PTL_INT32_T:
            case PTL_UINT32_T:
                *(int32_t*)me = *(int32_t*)me || *(int32_t*)rx;
                break;
            case PTL_INT64_T:
            case PTL_UINT64_T:
                *(int64_t*)me = *(int64_t*)me || *(int64_t*)rx;
                break;
            default:
                ptl_panic("unhandled atomic type\n");
            }
            break;
        case PTL_LAND:
            switch (type) {
            case PTL_INT8_T:
            case PTL_UINT8_T:
                *(int8_t*)me = *(int8_t*)me && *(int8_t*)rx;
                break;
            case PTL_INT16_T:
            case PTL_UINT16_T:
                *(int16_t*)me = *(int16_t*)me && *(int16_t*)rx;
                break;
            case PTL_INT32_T:
            case PTL_UINT32_T:
                *(int32_t*)me = *(int32_t*)me && *(int32_t*)rx;
                break;
            case PTL_INT64_T:
            case PTL_UINT64_T:
                *(int64_t*)me = *(int64_t*)me && *(int64_t*)rx;
                break;
            default:
                ptl_panic("unhandled atomic type\n");
            }
            break;
        case PTL_LXOR:
            switch (type) {
            case PTL_INT8_T:
            case PTL_UINT8_T:
                *(int8_t*)me = !!*(int8_t*)me ^ !!*(int8_t*)rx;
                break;
            case PTL_INT16_T:
            case PTL_UINT16_T:
                *(int16_t*)me = !!*(int16_t*)me ^ !!*(int16_t*)rx;
                break;
            case PTL_INT32_T:
            case PTL_UINT32_T:
                *(int32_t*)me = !!*(int32_t*)me ^ !!*(int32_t*)rx;
                break;
            case PTL_INT64_T:
            case PTL_UINT64_T:
                *(int64_t*)me = !!*(int64_t*)me ^ !!*(int64_t*)rx;
                break;
            default:
                ptl_panic("unhandled atomic type\n");
            }
            break;
        case PTL_SWAP:
        case PTL_CSWAP:
        case PTL_CSWAP_NE:
        case PTL_CSWAP_LE:
        case PTL_CSWAP_LT:
        case PTL_CSWAP_GE:
        case PTL_CSWAP_GT:
            if (n != 1)
                ptl_panic("only one item allowed for swap\n");

            switch (type) {
            case PTL_INT8_T:
                eq = *(int8_t*)me == *(int8_t*)cst;
                le = *(int8_t*)me >= *(int8_t*)cst;
                ge = *(int8_t*)me <= *(int8_t*)cst;
                break;
            case PTL_UINT8_T:
                eq = *(uint8_t*)me == *(uint8_t*)cst;
                le = *(uint8_t*)me >= *(uint8_t*)cst;
                ge = *(uint8_t*)me <= *(uint8_t*)cst;
                break;
            case PTL_INT16_T:
                eq = *(int16_t*)me == *(int16_t*)cst;
                le = *(int16_t*)me >= *(int16_t*)cst;
                ge = *(int16_t*)me <= *(int16_t*)cst;
                break;
            case PTL_UINT16_T:
                eq = *(uint16_t*)me == *(uint16_t*)cst;
                le = *(uint16_t*)me >= *(uint16_t*)cst;
                ge = *(uint16_t*)me <= *(uint16_t*)cst;
                break;
            case PTL_INT32_T:
                eq = *(int32_t*)me == *(int32_t*)cst;
                le = *(int32_t*)me >= *(int32_t*)cst;
                ge = *(int32_t*)me <= *(int32_t*)cst;
                break;
            case PTL_UINT32_T:
                eq = *(uint32_t*)me == *(uint32_t*)cst;
                le = *(uint32_t*)me >= *(uint32_t*)cst;
                ge = *(uint32_t*)me <= *(uint32_t*)cst;
                break;
            case PTL_INT64_T:
                eq = *(int64_t*)me == *(int64_t*)cst;
                le = *(int64_t*)me >= *(int64_t*)cst;
                ge = *(int64_t*)me <= *(int64_t*)cst;
                break;
            case PTL_UINT64_T:
                eq = *(uint64_t*)me == *(uint64_t*)cst;
                le = *(uint64_t*)me >= *(uint64_t*)cst;
                ge = *(uint64_t*)me <= *(uint64_t*)cst;
                break;
            case PTL_FLOAT:
                eq = *(float*)me == *(float*)cst;
                le = *(float*)me >= *(float*)cst;
                ge = *(float*)me <= *(float*)cst;
                break;
            case PTL_DOUBLE:
                eq = *(double*)me == *(double*)cst;
                le = *(double*)me >= *(double*)cst;
                ge = *(double*)me <= *(double*)cst;
                break;
            case PTL_LONG_DOUBLE:
                eq = *(long double*)me == *(long double*)cst;
                le = *(long double*)me >= *(long double*)cst;
                ge = *(long double*)me <= *(long double*)cst;
                break;
            case PTL_FLOAT_COMPLEX:
                eq = *(float _Complex*)me == *(float _Complex*)cst;
                le = 0;
                ge = 0;
                break;
            case PTL_DOUBLE_COMPLEX:
                eq = *(double _Complex*)me == *(double _Complex*)cst;
                le = 0;
                ge = 0;
                break;
            case PTL_LONG_DOUBLE_COMPLEX:
                eq = *(long double _Complex*)me == *(long double _Complex*)cst;
                le = 0;
                ge = 0;
                break;
            default:
                ptl_panic("unhandled atomic type\n");
                return; /* Fix compilation warning */
            }
            switch (op) {
            case PTL_CSWAP:
                swap = eq;
                break;
            case PTL_CSWAP_NE:
                swap = !eq;
                break;
            case PTL_CSWAP_LE:
                swap = le;
                break;
            case PTL_CSWAP_LT:
                swap = le && !eq;
                break;
            case PTL_CSWAP_GE:
                swap = ge;
                break;
            case PTL_CSWAP_GT:
                swap = ge && !eq;
                break;
            case PTL_SWAP:
                swap = 1;
                break;
            default:
                swap = 0;
            }
            if (swap)
                memcpy(me, rx, asize);
            break;
        case PTL_MSWAP:
            for (j = 0; j < asize; j++) {
                ((char*)me)[j] &= ~((char*)cst)[j];
                ((char*)me)[j] |= ((char*)rx)[j] & ((char*)cst)[j];
            }
            break;
        default:
            ptl_panic_fmt("%x: unhandled atomic op\n", op);
        }
    }
}
