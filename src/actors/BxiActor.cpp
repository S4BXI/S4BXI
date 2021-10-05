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

#include "s4bxi/actors/BxiActor.hpp"
#include "s4bxi/BxiEngine.hpp"

#include <sstream>
#include <regex>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace simgrid;

BxiActor::BxiActor()
{
    node = nullptr;
    self = s4u::Actor::self();

    // Slug setup (= Linux hostname)

    bool has_separate_cores;
    bool is_main_actor;
    string own_name = self->get_host()->get_name();
    if (boost::algorithm::ends_with(own_name, "_NIC")) {
        is_main_actor = false;
        slug          = own_name.substr(0, own_name.length() - 4);
    } else {
        is_main_actor = true;
        smatch what;
        if (regex_match(own_name, what, regex("(.*)_CPU([0-9]+)"), regex_constants::match_default)) {
            // what[0] contains the whole string
            // what[1] contains the slug we're looking for
            // what[2] contains the CPU number
            slug               = what[1];
            has_separate_cores = true;
        } else {
            slug               = own_name; // Backward compatibility, when there was only one core per CPU
            has_separate_cores = false;
        }
    }

    // NID setup

    int nid;

    const char* prop = s4u::Host::by_name(slug + "_NIC")->get_property("nid");
    if (prop) { // Get property if manually set in platform
        nid = atoi(prop);
    } else { // Try to guess (i.e. take the first blob of numbers in the slug)
        stringstream ss_nid;
        int i;

        // This is an ugly algorithm : it doesn't check that we're indeed finding something, that
        // the NID we guess isn't duplicate or anything, but if the user isn't as dumb as a
        // frying pan it should do the job as a shortcut for the manual "nid" property in the host
        for (i = 0; i < slug.length(); ++i)
            if (slug[i] >= '0' && slug[i] <= '9')
                break;
        for (; i < slug.length(); ++i) {
            if (slug[i] >= '0' && slug[i] <= '9')
                ss_nid << slug[i];
            else
                break;
        }
        ss_nid >> nid;
    }

    node = BxiEngine::get_instance()->get_node(nid);

    if (is_main_actor) {
        // This is very much an ugly hack, the core 0 should not be special, but whatever
        node->main_host = s4u::Host::by_name(has_separate_cores ? slug + "_CPU0" : slug);
    }
    node->nic_host = s4u::Host::by_name(slug + "_NIC");
}

string BxiActor::nic_tx_mailbox_name(const bxi_vn vn)
{
    return nic_tx_mailbox_name(node->nid, vn);
}

string BxiActor::nic_rx_mailbox_name(const bxi_vn vn)
{
    return nic_rx_mailbox_name(node->nid, vn);
}

string BxiActor::nic_tx_mailbox_name(const int nid, const bxi_vn vn)
{
    return to_string(nid) + "_nic_tx_" + to_string(vn);
}

string BxiActor::nic_rx_mailbox_name(const int nid, const bxi_vn vn)
{
    return to_string(nid) + "_nic_rx_" + to_string(vn);
}

void BxiActor::issue_event(BxiEQ* eq, ptl_event_t* ev)
{
    node->issue_event(eq, ev);
}

s4u::Actor* BxiActor::getSimgridActor()
{
    return self;
}

ptl_nid_t BxiActor::getNid()
{
    return node->nid;
}

string BxiActor::getSlug()
{
    return slug;
}

const shared_ptr<BxiNode> BxiActor::getNode()
{
    return node;
}