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

#ifndef S4BXI_BXIQUEUE_HPP
#define S4BXI_BXIQUEUE_HPP

#include <string>
#include <queue>
#include "s4ptl.hpp"

/**
 * @brief FIFO structure with variable timing
 *
 * This queue implementation has 2 different timing behaviours:
 *
 * - The mailbox approach, which is slow in wall-clock time but
 *   allows modeling a transfer in simulated-world time
 * - The semaphore + std::queue approach, which is more performant
 *   (wall-clock time) but doesn't update the simulated-world's time
 *   (although it still yields to SimGrid because of the use of a
 *   simulated-world semaphore)
 */
class BxiQueue {
    std::queue<BxiMsg*> to_process;
    simgrid::s4u::SemaphorePtr waiting;
    simgrid::s4u::Mailbox* mailbox;

  public:
    BxiQueue();
    explicit BxiQueue(const std::string& mailbox_name);

    void put(BxiMsg* msg, const uint64_t& size = 0, bool async = false);
    BxiMsg* get();
    bool ready();
    int size();
    void clear();
};

#endif // S4BXI_BXIQUEUE_HPP
