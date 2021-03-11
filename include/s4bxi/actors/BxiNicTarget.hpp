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

class BxiNicTarget : public BxiNicActor {
    s4u::Mailbox* nic_rx_mailbox;
    BxiQueue* tx_queue;

    void handle_put_request(BxiMsg* msg);
    void handle_get_request(BxiMsg* msg);
    void handle_atomic_request(BxiMsg* msg);
    void handle_fetch_atomic_request(BxiMsg* msg);
    void handle_get_response(BxiMsg* msg);
    void handle_fetch_atomic_response(BxiMsg* msg);
    void handle_ptl_ack(BxiMsg* msg);
    void handle_bxi_ack(BxiMsg* msg);
    BxiME* match_entry(BxiMsg* msg);
    void handle_response(BxiMsg* msg, BxiMD* md, BxiME* matched_me, ptl_size_t offset);
    void apply_atomic_op(int op, int type, unsigned char* me, unsigned char* cst, unsigned char* rx, unsigned char* tx,
                         size_t len);
    void capped_memcpy(void* dest, const void* src, size_t n);

  public:
    BxiNicTarget(const vector<string>& args);

    void operator()();
};

#endif // S4BXI_BXINICTARGET_HPP
