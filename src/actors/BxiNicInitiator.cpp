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
    auto flowctrl_msq_queue = &node->flowctrl_waiting_messages[vn];

    for (;;) {
        BxiMsg* msg = tx_queue->get();

        if (!node->check_flowctrl(msg)) {
            // If we ran out of flow control, simply store the message in a queue and move on, it will be the
            // responsability of actors that wake us up to put the messages back in our queue

            if (find(flowctrl_msq_queue->begin(), flowctrl_msq_queue->end(), msg) == flowctrl_msq_queue->end())
                flowctrl_msq_queue->push_back(msg);

            continue;
        }

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
    s4u::CommPtr dma = nullptr;

    auto req = (BxiPutRequest*)msg->parent_request;
    req->md->ni->cq->release();

    s4u::this_actor::execute(300); // Approximation of the time it takes the NIC to process a command

    int inline_size = req->matching ? 8 : 16;
    int PIO_size    = req->matching ? 408 : 416;

    int _bxi_log_level = S4BXI_GLOBAL_CONFIG(log_level);
    if (_bxi_log_level) {
        msg->bxi_log       = make_shared<BxiLog>();
        msg->bxi_log->type = (bxi_log_type)msg->type; // Highly unsafe cast, see comment in BxiNicActor::reliable_comm
        msg->bxi_log->initiator = msg->initiator;
        msg->bxi_log->target    = msg->target;
    }
    s4u::CommPtr comm = reliable_comm_init(msg, false);

    if (!msg->is_PIO && S4BXI_CONFIG_AND(node, model_pci) &&
        (msg->retry_count && msg->simulated_size > 64 // Retransmissions are always DMA (except small ones)
         || (!msg->retry_count && msg->simulated_size > inline_size))) {
        // Ask for the memory we need to send (DMA case)
        s4u::this_actor::execute(300);

        // Actually there are (msg->simulated_size / DMA chunk size) requests in real life,
        // and I don't know if they weigh 64B or something else. (chunk size is 128, 256,
        // 512 or 1024B)
        node->pci_transfer(64, PCI_NIC_TO_CPU, S4BXILOG_PCI_DMA_REQUEST);
        // S4BXI_STARTLOG(S4BXILOG_PCI_DMA_PAYLOAD, node->nid, node->nid)
        dma = node->pci_transfer_async(req->payload_size - inline_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_DMA_PAYLOAD);
        // Wait for first packet (very approximate heuristic)
        double wait_time = msg->simulated_size >= 512 ? ONE_PCI_PACKET_TRANSFER
                                                      : (PCI_LATENCY + ((double)msg->simulated_size) / 15.75e9);
        s4u::this_actor::sleep_for(wait_time);

        if (_bxi_log_level)
            msg->bxi_log->start = s4u::Engine::get_clock();

        comm->detach(); // Starts the comm
    } else {
        if (_bxi_log_level)
            msg->bxi_log->start = s4u::Engine::get_clock();
        comm->start(); // Starts the comm
    }

    // Buffered put
    // Now in this case the initiator only has a *very* short blocking phase
    // I don't know if it is an issue or not. Actually that could be better
    // to process more messages in parallel with few actors ?
    if (msg->simulated_size <= 64) {
        s4u::this_actor::execute(400);
        req->maybe_issue_send();
    }

    if (dma)
        dma->wait();
    else
        comm->wait();
}

void BxiNicInitiator::handle_get(BxiMsg* msg)
{
    ((BxiGetRequest*)msg->parent_request)->md->ni->cq->release();
    reliable_comm(msg);
}

void BxiNicInitiator::handle_response(BxiMsg* msg, bxi_log_type type)
{
    s4u::CommPtr dma = nullptr;

    // s4u::this_actor::execute(400);

    int _bxi_log_level = S4BXI_GLOBAL_CONFIG(log_level);
    if (_bxi_log_level) {
        msg->bxi_log            = make_shared<BxiLog>();
        msg->bxi_log->type      = type;
        msg->bxi_log->initiator = msg->initiator;
        msg->bxi_log->target    = msg->target;
    }
    s4u::CommPtr comm = reliable_comm_init(msg, false);

    if (S4BXI_CONFIG_AND(node, model_pci) && msg->simulated_size) {
        // Ask for the memory we need to send (Get is always DMA)
        node->pci_transfer(64, PCI_NIC_TO_CPU, S4BXILOG_PCI_DMA_REQUEST);
        dma = node->pci_transfer_async(msg->simulated_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_DMA_PAYLOAD);
        // Wait for first packet (very approximate heuristic)
        double wait_time = msg->simulated_size >= 512 ? ONE_PCI_PACKET_TRANSFER
                                                      : (PCI_LATENCY + ((double)msg->simulated_size) / 15.75e9);
        s4u::this_actor::sleep_for(wait_time);

        if (_bxi_log_level)
            msg->bxi_log->start = s4u::Engine::get_clock();

        comm->detach(); // Starts the comm
    } else {
        if (_bxi_log_level)
            msg->bxi_log->start = s4u::Engine::get_clock();
        comm->start(); // Starts the comm
    }

    if (dma)
        dma->wait();
    else
        comm->wait();
}

void BxiNicInitiator::handle_get_response(BxiMsg* msg)
{
    handle_response(msg, S4BXILOG_PTL_GET_RESPONSE);

    if (node->e2e_off) // If E2E is off we will never get an E2E ACK, so do this here I guess
        maybe_issue_get((BxiGetRequest*)msg->parent_request);
}

void BxiNicInitiator::handle_fetch_atomic_response(BxiMsg* msg)
{
    handle_response(msg, S4BXILOG_PTL_FETCH_ATOMIC_RESPONSE);

    if (node->e2e_off) // If E2E is off we will never get an E2E ACK, so do this here I guess
        maybe_issue_fetch_atomic((BxiFetchAtomicRequest*)msg->parent_request);
}