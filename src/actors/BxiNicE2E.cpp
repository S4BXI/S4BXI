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

#include "s4bxi/actors/BxiNicE2E.hpp"
#include "s4bxi/s4bxi_xbt_log.h"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_nic_e2e, "Messages specific to the NIC E2E circuit");

BxiNicE2E::BxiNicE2E(const vector<string>& args) : BxiActor()
{
    // If E2E was disabled globally, immediately kill any E2E actor
    // This also forces at most 1 E2E actor per node
    if (S4BXI_CONFIG(e2e_off) || node->e2e_actor)
        self->kill();

    self->daemonize();

    node->e2e_actor = this;

    s4u::this_actor::on_exit([this](bool) {
        if (current_msg)
            BxiMsg::unref(current_msg);

        queue.clear();
        XBT_INFO("Retried %lu times, gave up %lu times", node->e2e_retried, node->e2e_gave_up);
    });

    XBT_INFO("E2E was configured with timeout = %f and retries = %d", S4BXI_CONFIG(retry_timeout),
             S4BXI_CONFIG(max_retries));
}

/**
 * E2E logic of NIC
 *
 * It is OK to have an infinite loop since this actor is daemonized.
 *
 * One E2E actor for each NIC is enough : because `retry_timeout` is constant,
 * we know retries must be processed in the order messages are issued, so
 * waiting until the next retry doesn't break the order of timeout processing
 *
 * This property is very important : if `retry_timeout` were to change between
 * messages, we would need to create a new E2E actor for each message and then
 * kill it (or make this one way more complex). Happily in the real world
 * `retry_timeout` is a kernel module parameter, and isn't expected to change
 */
void BxiNicE2E::operator()()
{
    // If for some reason E2E was disabled globally, immediately kill any E2E actor
    if (S4BXI_CONFIG(e2e_off))
        return;

    for (;;) {
        auto msg    = queue.get();
        current_msg = msg;

        double wake_up_time = msg->send_init_time + S4BXI_CONFIG(retry_timeout);

        if (wake_up_time < s4u::Engine::get_clock()) {
            XBT_INFO("Expected sleep > %f, got %f", s4u::Engine::get_clock(), wake_up_time);
            ptl_panic("Negative sleep expected in E2E, this is catastrophic");
        }

        s4u::this_actor::sleep_until(wake_up_time);

        // Message got ACKed in time, ignore E2E processing and go to next one
        if (msg->parent_request->process_state >=
            (msg->type == S4BXI_PTL_ACK ? S4BXI_REQ_FINISHED : S4BXI_REQ_ANSWERED)) {
            BxiMsg::unref(msg);
            current_msg = nullptr;
            continue;
        }

        // All hope is lost for this message, just give up and go to the next one
        if (msg->retry_count == S4BXI_CONFIG(max_retries)) {
            ++node->e2e_gave_up;
            BxiMsg::unref(msg);
            current_msg = nullptr;
            // burn_the_whole_cluster_I_guess();
            continue;
        }

        // Message didn't get an ACK yet, and can be retransmitted :
        // give it back to BxiNicInitiator and go to the next one
        if (msg->retry_count < S4BXI_CONFIG(max_retries)) {
            ++node->e2e_retried;
            ++msg->retry_count;

            get_retransmit_mailbox(msg)->put_init(new BxiMsg(*msg), 0)->detach();

            BxiMsg::unref(msg);
            current_msg = nullptr;
            continue;
        }

        XBT_INFO("Expected retry count > %d, got %d", S4BXI_CONFIG(max_retries), msg->retry_count);
        ptl_panic("Retry count is bigger than MAX_RETRY_COUNT in E2E, which shouldn't be possible");
    }
}

s4u::Mailbox* BxiNicE2E::get_retransmit_mailbox(const BxiMsg* msg)
{
    int vn_num = (msg->type == S4BXI_PTL_PUT || msg->type == S4BXI_PTL_GET) ? S4BXI_VN_SERVICE_REQUEST
                                                                            : S4BXI_VN_SERVICE_RESPONSE;
    if (!msg->parent_request->service_vn)
        vn_num += 1; // Switch to compute version
    auto vn = (bxi_vn)vn_num;

    if (!nic_cmd_mailboxes[vn_num])
        nic_cmd_mailboxes[vn_num] = s4u::Mailbox::by_name(nic_tx_mailbox_name(vn));
    return nic_cmd_mailboxes[vn_num];
}

/**
 * Used by other actors to ask E2E to process messages
 */
void BxiNicE2E::process_message(BxiMsg* msg)
{
    ++msg->ref_count;
    msg->send_init_time = s4u::Engine::get_clock();
    queue.put(msg);
}