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

#ifndef S4BXI_MAILBOX_POOL_HPP
#define S4BXI_MAILBOX_POOL_HPP

#include <simgrid/s4u/Mailbox.hpp>

simgrid::s4u::Mailbox* get_random_mailbox();

void free_random_mailbox(simgrid::s4u::Mailbox* m);

void free_mailbox_pool();

#endif // S4BXI_MAILBOX_POOL_HPP
