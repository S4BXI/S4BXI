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

#include "s4bxi/s4bxi_mailbox_pool.hpp"
#include "s4bxi/s4bxi_util.hpp"
#include <stack>

static stack<s4u::Mailbox*> pool;

s4u::Mailbox* get_random_mailbox()
{
    if (!pool.empty()) {
        s4u::Mailbox* m = pool.top();
        pool.pop();
        return m;
    }

    return s4u::Mailbox::by_name("rand_mb_" + to_string(random_int()));
}

void free_random_mailbox(s4u::Mailbox* m)
{
    // Allows the calling actor to be garbage-collected :
    // https://simgrid.org/doc/latest/app_s4u.html#declaring-a-receiving-actor
    m->set_receiver(nullptr);
    pool.push(m);
}

void free_mailbox_pool()
{
    // Actually we probably don't need to do anything, SimGrid should manage the mailboxes itself
    // while (!pool.empty())
    //     pool.pop();
}