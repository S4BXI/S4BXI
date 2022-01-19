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

#include "s4bxi/BxiQueue.hpp"

using namespace std;
using namespace simgrid;

BxiQueue::BxiQueue() : mailbox(nullptr)
{
    waiting = s4u::Semaphore::create(0);
}

BxiQueue::BxiQueue(const string& mailbox_name)
{
    mailbox = s4u::Mailbox::by_name(mailbox_name);
    mailbox->set_receiver(s4u::Actor::self());
}

void BxiQueue::put(BxiMsg* msg, const uint64_t& size, bool async)
{
    if (mailbox) {
        if (async)
            mailbox->put_init(msg, size)->set_copy_data_callback(&s4u::Comm::copy_pointer_callback)->detach();
        else
            mailbox->put_init(msg, size)->set_copy_data_callback(&s4u::Comm::copy_pointer_callback)->wait();
        return;
    }

    to_process.push(msg);
    waiting->release();
}

BxiMsg* BxiQueue::get()
{
    if (mailbox) {
        BxiMsg* msg;
        mailbox->get_init()
            ->set_dst_data(reinterpret_cast<void**>(&msg), sizeof(void*))
            ->set_copy_data_callback(&s4u::Comm::copy_pointer_callback)
            ->wait();

        return msg;
    }

    waiting->acquire();
    auto msg = to_process.front();
    to_process.pop();

    return msg;
}

bool BxiQueue::ready()
{
    if (mailbox)
        return mailbox->ready();

    return !waiting->would_block();
}

int BxiQueue::size() {
    if (mailbox)
        return mailbox->size();

    return to_process.size();
}

void BxiQueue::clear()
{
    // We can't do much in the "mailbox" case : we're not allowed to do blocking `get`s in on_exit functions,
    // and this would be used mostly in on_exit functions

    while (!this->to_process.empty()) {
        BxiMsg::unref(to_process.front());
        to_process.pop();
    }
}