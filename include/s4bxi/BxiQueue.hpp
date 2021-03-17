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

#ifndef S4BXI_BXIQUEUE_HPP
#define S4BXI_BXIQUEUE_HPP

#include <string>
#include <queue>
#include "s4ptl.hpp"

using namespace std;

class BxiQueue {
    queue<BxiMsg*> to_process;
    s4u::SemaphorePtr waiting;
    s4u::Mailbox* mailbox;

  public:
    BxiQueue();
    explicit BxiQueue(const string& mailbox_name);

    void put(BxiMsg* msg, const uint64_t &size = 0, bool async = false);
    BxiMsg* get();
    bool ready();
    void clear();
};

#endif // S4BXI_BXIQUEUE_HPP
