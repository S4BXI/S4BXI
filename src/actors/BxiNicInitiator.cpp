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

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_nic_initiator, "Messages specific to the NIC initiator");

BxiNicInitiator::BxiNicInitiator(const vector<string>& args) : BxiNicActor(args)
{
    tx_queue =
        node->model_pci && S4BXI_CONFIG(model_pci_commands) ? new BxiQueue(nic_tx_mailbox_name(vn)) : new BxiQueue;
    node->tx_queues[vn] = tx_queue;
}

/**
 * Processing transmit logic of NIC
 *
 * It is OK to have an infinite loop since this actor is daemonized
 */
void BxiNicInitiator::operator()()
{
    for (;;) {
        auto msg = tx_queue->get();
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
        case S4BXI_PTL_RESPONSE:
            handle_response(msg);
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
    BxiMD* md = req->md;
    md->ni->cq->release();

    int inline_size = req->matching ? 8 : 16;
    int PIO_size    = req->matching ? 408 : 416;

    s4u::ActivityPtr comm = nullptr;

    if (!msg->retry_count // PIO doesn't make sense for retransmissions
        && node->model_pci && req->payload_size > inline_size && req->payload_size <= PIO_size) {
        // Second part of PIO command (end of payload)

        pci_transfer_async(req->payload_size - inline_size, PCI_NIC_TO_CPU, S4BXILOG_PCI_PIO_PAYLOAD);

    } else if (node->model_pci && (msg->retry_count // Retransmissions are always DMA (even small ones)
                                   || msg->simulated_size > PIO_size)) {
        // Ask for the memory we need to send (DMA case)

        // Actually there are (msg->simulated_size / DMA chunk size) requests in real life,
        // and I don't know if they weigh 64B or something else. (chunk size is 128, 256,
        // 512 or 1024B)
        pci_transfer(64, PCI_NIC_TO_CPU, S4BXILOG_PCI_DMA_REQUEST);
        pci_transfer_async(req->payload_size - inline_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_DMA_PAYLOAD);
    }

    // Buffered put
    // Now in this case the initiator only has a *very* short blocking phase
    // I don't know if it is an issue or not. Actually that could be better
    // to process more messages in parallel with few actors ?
    if (msg->simulated_size <= 64)
        req->maybe_issue_send();

    reliable_comm(msg);

    if (comm)
        comm->wait(); // Wait for any memory operation in progress (PIO or DMA)
}

void BxiNicInitiator::handle_get(BxiMsg* msg)
{
    ((BxiGetRequest*)msg->parent_request)->md->ni->cq->release();
    reliable_comm(msg);
}

// TO-DO: factorise code below

void BxiNicInitiator::handle_response(BxiMsg* msg)
{
    if (node->model_pci && msg->simulated_size) { // Ask for the memory we need to send (Get is always DMA)
        pci_transfer(64, PCI_NIC_TO_CPU, S4BXILOG_PCI_DMA_REQUEST);
        pci_transfer_async(msg->simulated_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_DMA_PAYLOAD);
    }

    // Buffered response (just like PtlPuts)
    if (msg->simulated_size <= 64)
        maybe_issue_get((BxiGetRequest*)msg->parent_request);

    reliable_comm(msg);

    if (node->e2e_off) // If E2E is off we will never get an E2E ACK, so do this here I guess
        maybe_issue_get((BxiGetRequest*)msg->parent_request);
}

void BxiNicInitiator::handle_fetch_atomic_response(BxiMsg* msg)
{
    if (node->model_pci && msg->simulated_size) { // Ask for the memory we need to send (Response is always DMA)
        pci_transfer(64, PCI_NIC_TO_CPU, S4BXILOG_PCI_DMA_REQUEST);
        pci_transfer_async(msg->simulated_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_DMA_PAYLOAD);
    }

    // Buffered response (just like PtlPuts)
    if (msg->simulated_size <= 64)
        maybe_issue_fetch_atomic((BxiFetchAtomicRequest*)msg->parent_request);

    reliable_comm(msg);

    if (node->e2e_off) // If E2E is off we will never get an E2E ACK, so do this here I guess
        maybe_issue_fetch_atomic((BxiFetchAtomicRequest*)msg->parent_request);
}