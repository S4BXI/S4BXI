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
#include "s4bxi/s4bxi_util.hpp"
#include "s4bxi/s4bxi_xbt_log.h"

#include <xbt.h>

S4BXI_LOG_NEW_DEFAULT_CATEGORY(bxi_s4ptl_ct, "Messages specific to s4ptl CT implementation");

BxiCT::BxiCT()
{
    ptl_ct_event_t ev{0, 0};
    event = ev;
}

void BxiCT::on_update()
{
    for (auto it = waiting.begin(); it != waiting.end(); ++it) {
        if (it->test == event.success || event.failure) {
            s4u::Actor* actor             = it->actor;
            s4u::Mailbox* mailbox = it->mailbox;
            waiting.erase(it--);

            // We need to erase `it` from the vector before making any simcall, otherwise
            // other actors could wreck the `waiting` vector during the execution of this
            // very function (remember that any number of actor can be waken up and do a
            // lot of things during a simcall such as `actor->resume()` for example)

            if (actor) { // Wake up any actor that was suspended (by PtlCTWait)
                actor->resume();
            } else if (mailbox) { // Unlock any actor that is waiting on a PtlCTPoll
                bool garbage = true;
                mailbox->put(&garbage, 0);
                free_random_mailbox(mailbox);
            } else {
                ptl_panic("We have a waiting struct in counter, but no actor to wake up or mailbox to contact");
            }
        }
    }
}

void BxiCT::increment_success(ptl_size_t amount)
{
    event.success += amount;
    on_update();
}

int BxiCT::set_value(ptl_ct_event_t new_ct)
{
    event = new_ct;

    on_update();

    return PTL_OK;
}

int BxiCT::increment(ptl_ct_event_t increment)
{
    event.success += increment.success;
    event.failure += increment.failure;

    on_update();

    return PTL_OK;
}

int BxiCT::wait(ptl_size_t test, ptl_ct_event_t* ev)
{
    auto self = s4u::Actor::self();
    if (event.success < test && !event.failure) {
        waiting.push_back(ActorWaitingCT{self, nullptr, test});
        self->suspend(); // When somebody increments the CT, they will wake us up if necessary
    }

    // If we get here, we were awakened by somebody
    *ev = event;

    return PTL_OK;
}

int BxiCT::poll(const ptl_handle_ct_t* ct_handles, const ptl_size_t* tests, unsigned int size, ptl_time_t timeout,
                ptl_ct_event_t* event, unsigned int* which)
{
    if (timeout < 0 && timeout != PTL_TIME_FOREVER)
        XBT_ERROR("Incorrect timeout value in BxiCT::poll (expected >= 0 or PTL_TIME_FOREVER, got %ld)", timeout);

    unsigned int i;
    BxiCT* cts[size];

    for (i = 0; i < size; ++i)
        cts[i] = (BxiCT*)ct_handles[i];

    // Check if a CT already fullfills the corresponding test
    for (i = 0; i < size; ++i) {
        if (cts[i]->event.success >= tests[i] || cts[i]->event.failure) {
            *which = i;
            *event = cts[i]->event;

            return PTL_OK;
        }
    }

    // This is somewhat similar to BxiEQ::poll
    auto comms = vector<s4u::CommPtr>(size);
    bool* garbage;

    for (i = 0; i < size; ++i) {
        auto ct      = cts[i];
        auto waiting = ActorWaitingCT{nullptr, get_random_mailbox(), tests[i]};
        ct->waiting.push_back(waiting);
        comms[i] = waiting.mailbox->get_async<bool>(&garbage);
    }

    unsigned int which_ct =
        timeout == PTL_TIME_FOREVER ? s4u::Comm::wait_any(&comms) : s4u::Comm::wait_any_for(&comms, timeout / 1000.0F);

    for (const auto& comm : comms)
        comm->cancel();

    if (which_ct < 0 || which_ct >= comms.size())
        return PTL_CT_NONE_REACHED;

    *event = cts[which_ct]->event;
    *which = which_ct;

    return PTL_OK;
}