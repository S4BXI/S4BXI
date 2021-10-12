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

#ifndef S4BXI_BXINICTARGET_HPP
#define S4BXI_BXINICTARGET_HPP

#include <vector>
#include <string>

#include "BxiNicActor.hpp"
#include "../s4ptl.hpp"
#include "../BxiQueue.hpp"

/**
 * @brief RX side of the NIC
 *
 * This actor listens for messages from the BXI network and processes
 * them to issue the correct event and/or send a response
 */
class BxiNicTarget : public BxiNicActor {
    simgrid::s4u::Mailbox* nic_rx_mailbox;
    std::shared_ptr<BxiQueue> tx_queue;

    void handle_put_request(BxiMsg* msg);
    void handle_get_request(BxiMsg* msg);
    void handle_atomic_request(BxiMsg* msg);
    void handle_fetch_atomic_request(BxiMsg* msg);
    void handle_get_response(BxiMsg* msg);
    void handle_fetch_atomic_response(BxiMsg* msg);
    void handle_ptl_ack(BxiMsg* msg);
    void handle_bxi_ack(BxiMsg* msg);
    int match_entry(BxiMsg* msg, BxiME** me);
    void handle_response(BxiMsg* msg);
    void apply_atomic_op(int op, int type, unsigned char* me, unsigned char* cst, unsigned char* rx, unsigned char* tx,
                         size_t len);
    void capped_memcpy(void* dest, const void* src, size_t n);
    void send_ack(BxiMsg* msg, bxi_msg_type ack_type, int ni_fail_type);

  public:
    BxiNicTarget(const std::vector<std::string>& args);

    void operator()();
};

#endif // S4BXI_BXINICTARGET_HPP
