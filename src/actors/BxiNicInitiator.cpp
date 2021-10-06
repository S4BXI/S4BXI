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

#include "s4bxi/actors/BxiNicInitiator.hpp"

#include <utility>
#include "s4bxi/s4bxi_xbt_log.h"

using namespace std;
using namespace simgrid;

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_nic_initiator, "Messages specific to the NIC initiator");

BxiNicInitiator::BxiNicInitiator(const vector<string>& args) : BxiNicActor(args)
{
    if (node->tx_queues[vn]) {
        tx_queue = node->tx_queues[vn];
    } else {
        tx_queue = S4BXI_CONFIG_AND(node, model_pci_commands) ? make_shared<BxiQueue>(nic_tx_mailbox_name(vn))
                                                              : make_shared<BxiQueue>();
        node->tx_queues[vn] = tx_queue;
    }
}

/**
 * Processing transmit logic of NIC
 *
 * It is OK to have an infinite loop since this actor is daemonized
 */
void BxiNicInitiator::operator()()
{
    for (;;) {
        BxiMsg* msg = tx_queue->get();

        // E2E_ACKs don't have any higher level of ACKs, and retries should be prioritary (and they are only processed
        // once at target so we only get credits back once per message)
        bool flowctrl_check =
            (msg->type == S4BXI_E2E_ACK || msg->retry_count) ? true : node->check_process_flowctrl(msg);

        if (!flowctrl_check) {
            if (find(node->flowctrl_waiting_messages[vn].begin(), node->flowctrl_waiting_messages[vn].end(), msg) ==
                node->flowctrl_waiting_messages[vn].end()) {
                node->flowctrl_waiting_messages[vn].push_back(msg);
            } else {
                node->flowctrl_waiting_messages[vn].clear();
                node->initiator_waiting_flowctrl[vn].push_back(self);
                XBT_INFO("Ran out of flow control because of message %d (am VN %d), go to sleep", msg->type, vn);
                s4u::this_actor::suspend();
                XBT_INFO("Someone woke me up");
            }
            tx_queue->put(msg, 0, true);
            XBT_INFO("Put message back in queue");
            continue;
        }

        node->flowctrl_waiting_messages[vn].clear();

        switch (msg->type) {
        case S4BXI_PTL_PUT:
        case S4BXI_PTL_ATOMIC:
        case S4BXI_PTL_FETCH_ATOMIC:
            handle_put(msg);
            break;
        case S4BXI_PTL_GET:
            handle_get(msg);
            break;
        case S4BXI_PTL_ACK: // The different behaviour for PTL vs BXI
        case S4BXI_E2E_ACK: // is implemented in reliable_comm
            reliable_comm(msg);
            break;
        case S4BXI_PTL_GET_RESPONSE:
            handle_get_response(msg);
            break;
        case S4BXI_PTL_FETCH_ATOMIC_RESPONSE:
            handle_fetch_atomic_response(msg);
            break;
        }
    }
}

void BxiNicInitiator::handle_put(BxiMsg* msg)
{
    auto req  = (BxiPutRequest*)msg->parent_request;
    req->md->ni->cq->release();

    int inline_size = req->matching ? 8 : 16;
    int PIO_size    = req->matching ? 408 : 416;

    int __bxi_log_level = S4BXI_GLOBAL_CONFIG(log_level);
    if (__bxi_log_level) {
        msg->bxi_log            = make_shared<BxiLog>();
        msg->bxi_log->type      = S4BXILOG_PTL_PUT;
        msg->bxi_log->initiator = msg->initiator;
        msg->bxi_log->target    = msg->target;
    }
    s4u::CommPtr comm = reliable_comm_init(msg, false);

    // Set this now, so that acquire_e2e_entry has already been done
    if (__bxi_log_level)
        msg->bxi_log->start = s4u::Engine::get_clock();

    if (!msg->retry_count // PIO doesn't make sense for retransmissions
        && S4BXI_CONFIG_AND(node, model_pci) && req->payload_size > inline_size && req->payload_size <= PIO_size) {
        // Second part of PIO command (end of payload)

        comm->detach();
        node->pci_transfer(req->payload_size - inline_size, PCI_NIC_TO_CPU, S4BXILOG_PCI_PIO_PAYLOAD);

    } else if (S4BXI_CONFIG_AND(node, model_pci) &&
               (msg->retry_count // Retransmissions are always DMA (even small ones)
                || msg->simulated_size > PIO_size)) {
        // Ask for the memory we need to send (DMA case)

        // Actually there are (msg->simulated_size / DMA chunk size) requests in real life,
        // and I don't know if they weigh 64B or something else. (chunk size is 128, 256,
        // 512 or 1024B)
        node->pci_transfer(64, PCI_NIC_TO_CPU, S4BXILOG_PCI_DMA_REQUEST);
        comm->detach();
        node->pci_transfer(req->payload_size - inline_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_DMA_PAYLOAD);
    } else {
        comm->wait(); // Synchronize on BXI since we don't have a PCI transfer
    }

    // Buffered put
    // Now in this case the initiator only has a *very* short blocking phase
    // I don't know if it is an issue or not. Actually that could be better
    // to process more messages in parallel with few actors ?
    if (msg->simulated_size <= 64)
        req->maybe_issue_send();
}

void BxiNicInitiator::handle_get(BxiMsg* msg)
{
    ((BxiGetRequest*)msg->parent_request)->md->ni->cq->release();
    reliable_comm(msg);
}

void BxiNicInitiator::handle_response(BxiMsg* msg, bxi_log_type type)
{
    int __bxi_log_level = S4BXI_GLOBAL_CONFIG(log_level);
    if (__bxi_log_level) {
        msg->bxi_log            = make_shared<BxiLog>();
        msg->bxi_log->type      = type;
        msg->bxi_log->initiator = msg->initiator;
        msg->bxi_log->target    = msg->target;
    }
    s4u::CommPtr comm = reliable_comm_init(msg, false);

    // Set this now, so that acquire_e2e_entry has already been done
    if (__bxi_log_level)
        msg->bxi_log->start = s4u::Engine::get_clock();

    if (S4BXI_CONFIG_AND(node, model_pci) &&
        msg->simulated_size) { // Ask for the memory we need to send (Get is always DMA)
        node->pci_transfer(64, PCI_NIC_TO_CPU, S4BXILOG_PCI_DMA_REQUEST);
        comm->detach();
        node->pci_transfer(msg->simulated_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_DMA_PAYLOAD);
    } else {
        comm->wait();
    }
}

void BxiNicInitiator::handle_get_response(BxiMsg* msg)
{
    handle_response(msg, S4BXILOG_PTL_GET_RESPONSE);

    // Buffered response (just like PtlPuts)
    if (msg->simulated_size <= 64)
        maybe_issue_get((BxiGetRequest*)msg->parent_request);

    if (node->e2e_off) // If E2E is off we will never get an E2E ACK, so do this here I guess
        maybe_issue_get((BxiGetRequest*)msg->parent_request);
}

void BxiNicInitiator::handle_fetch_atomic_response(BxiMsg* msg)
{
    handle_response(msg, S4BXILOG_PTL_FETCH_ATOMIC_RESPONSE);

    // Buffered response (just like PtlPuts)
    if (msg->simulated_size <= 64)
        maybe_issue_fetch_atomic((BxiFetchAtomicRequest*)msg->parent_request);

    if (node->e2e_off) // If E2E is off we will never get an E2E ACK, so do this here I guess
        maybe_issue_fetch_atomic((BxiFetchAtomicRequest*)msg->parent_request);
}