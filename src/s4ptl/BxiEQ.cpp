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

#include "s4bxi/s4ptl.hpp"
#include "s4bxi/s4bxi_xbt_log.h"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(bxi_s4ptl_eq, "Messages specific to s4ptl EQ implementation");

BxiEQ::BxiEQ()
{
    mailbox = get_random_mailbox();
    mailbox->set_receiver(s4u::Actor::self());
}

BxiEQ::~BxiEQ()
{
    free_random_mailbox(mailbox);
}

int BxiEQ::get(ptl_event_t* event)
{
    if (!mailbox->ready())
        return PTL_EQ_EMPTY;

    return wait(event); // Instantaneous since we know the mailbox is ready
}

int BxiEQ::wait(ptl_event_t* event)
{
    auto ev_ptr = mailbox->get<ptl_event_t>();
    *event      = *ev_ptr;
    delete ev_ptr;

    return PTL_OK;
}

int BxiEQ::poll(const ptl_handle_eq_t* eq_handles, unsigned int size, ptl_time_t timeout, ptl_event_t* event,
                unsigned int* which)
{
    if (timeout < 0 && timeout != PTL_TIME_FOREVER)
        XBT_ERROR("Incorrect timeout value in BxiEQ::poll (expected >= 0 or PTL_TIME_FOREVER, got %ld)", timeout);

    // ___//!\\___In case of emergency, use this safer (albeit slow) version___
    //    unsigned int poll_count = 0;
    //
    //    double initial = s4u::Engine::get_clock();
    //    for (;;) {
    //        ++poll_count;
    //        for (unsigned int i = 0; i < size; ++i) {
    //            s4u::this_actor::sleep_for(1e-8 + (poll_count > 20 ? ((poll_count - 20) * 1e-8) : 0));
    //            auto ret = ((BxiEQ*)eq_handles[i])->get(event);
    //
    //            if (ret == PTL_OK) {
    //                *which = i;
    //                return ret;
    //            }
    //        }
    //        if (timeout != PTL_TIME_FOREVER && s4u::Engine::get_clock() >= initial + timeout / 1000)
    //            return PTL_EQ_EMPTY;
    //    }

    // Probably useless, who knows actually
    for (unsigned int i = 0; i < size; ++i) {
        auto eq = ((BxiEQ*)eq_handles[i]);

        // First check if someone is ready or close to be
        if (eq->mailbox->ready() || eq->mailbox->listen()) {
            *which = i;
            eq->wait(event); // In theory this isn't good, I guess than in some extreme cases it could make us
                             // exceed the timeout, but it seems unlikely

            return PTL_OK;
        }
    }

    auto comms = vector<s4u::CommPtr>(size);
    ptl_event_t* ev_ptr;

    // Issue a get_async to the mailbox of each EQ,
    // directly redirected to the user provided event
    // structure in case of success
    for (unsigned int i = 0; i < size; ++i)
        comms[i] = ((BxiEQ*)eq_handles[i])->mailbox->get_async<ptl_event_t>(&ev_ptr);

    // Get an event or a timeout
    unsigned int which_eq =
        timeout == PTL_TIME_FOREVER
            ? s4u::Comm::wait_any(&comms)
            : s4u::Comm::wait_any_for(&comms, timeout / 1000.0F); // Portals time is in ms and SimGrid in s

    // Thanks millian for the idea of CommPtr->cancel()
    // (it seems safe to cancel all comms, even the one
    // which gave us an event and is already finished)
    for (const auto& comm : comms)
        comm->cancel();

    // I don't know what the exact rule is here, and gdb
    // didn't help me that much, so this condition will do
    if (which_eq < 0 || which_eq >= comms.size())
        return PTL_EQ_EMPTY;

    *event = *ev_ptr;
    *which = which_eq;
    delete ev_ptr;

    return PTL_OK;
}