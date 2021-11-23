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

#include <portals4.h>
#include <portals4_bxiext.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>

#define INT64T_ME      1
#define LONG_DOUBLE_ME 2

void ptlerr(std::string str, int rc)
{
    fprintf(stderr, "%s: %s\n", str.c_str(), PtlToStr(rc, PTL_STR_ERROR));
}

int client(char* target)
{
    int target_nid = atoi(target);

    PtlInit();
    ptl_handle_ni_t nih;
    int rc = PtlNIInit(PTL_IFACE_DEFAULT, PTL_NI_MATCHING | PTL_NI_PHYSICAL, 123, NULL, NULL, &nih);

    ptl_md_t mdpar;
    ptl_handle_eq_t eqh;
    ptl_handle_md_t mdh;
    ptl_process_t peer;
    ptl_event_t ev;

    rc = PtlEQAlloc(nih, 64, &eqh);

    peer.phys.nid = target_nid;
    peer.phys.pid = 123;

    double comm_time, total_size_kB;

    // INT64

    auto i64 = new int64_t(2);

    mdpar.start     = i64;
    mdpar.length    = sizeof(int64_t);
    mdpar.eq_handle = eqh;
    mdpar.ct_handle = PTL_CT_NONE;
    mdpar.options   = PTL_MD_EVENT_SEND_DISABLE;

    rc = PtlMDBind(nih, &mdpar, &mdh);

    sleep(1); // Make sure Server is done posting ME

    rc = PtlAtomic(mdh, 0, sizeof(int64_t), PTL_ACK_REQ, peer, 0, INT64T_ME, 0, nullptr, 1337, PTL_SUM, PTL_INT64_T);

    PtlEQWait(eqh, &ev);

    rc = PtlMDRelease(mdh);

    delete i64;

    // LONG_DOUBLE

    auto ld = new long double(3);

    mdpar.start     = ld;
    mdpar.length    = sizeof(long double);
    mdpar.eq_handle = eqh;
    mdpar.ct_handle = PTL_CT_NONE;
    mdpar.options   = PTL_MD_EVENT_SEND_DISABLE;

    rc = PtlMDBind(nih, &mdpar, &mdh);

    rc = PtlAtomic(mdh, 0, sizeof(long double), PTL_ACK_REQ, peer, 0, LONG_DOUBLE_ME, 0, nullptr, 7331, PTL_PROD,
                   PTL_LONG_DOUBLE);

    PtlEQWait(eqh, &ev);

    rc = PtlMDRelease(mdh);

    delete ld;

    rc = PtlEQFree(eqh);

    rc = PtlNIFini(nih);
    PtlFini();

    return 0;
}

int server()
{
    unsigned int which;

    ptl_handle_ni_t nih;
    int rc = PtlInit();

    rc = PtlNIInit(PTL_IFACE_DEFAULT, PTL_NI_MATCHING | PTL_NI_PHYSICAL, 123, NULL, NULL, &nih);

    ptl_handle_eq_t eqh;

    rc = PtlEQAlloc(nih, 64, &eqh);

    ptl_pt_index_t pte;

    rc = PtlPTAlloc(nih, 0, eqh, 0, &pte);
    ptl_event_t ev;

    // INT64

    auto i64 = new int64_t;
    *i64     = 40;

    ptl_me_t mepar_i64;
    auto meh_i64 = new ptl_handle_me_t;

    memset(&mepar_i64, 0, sizeof(ptl_me_t));
    mepar_i64.start       = i64;
    mepar_i64.length      = sizeof(int64_t);
    mepar_i64.ct_handle   = NULL;
    mepar_i64.match_bits  = INT64T_ME;
    mepar_i64.ignore_bits = 0;
    mepar_i64.uid         = PTL_UID_ANY;
    mepar_i64.options     = PTL_ME_OP_PUT | PTL_ME_OP_GET;

    rc = PtlMEAppend(nih, pte, &mepar_i64, PTL_PRIORITY_LIST, meh_i64, meh_i64);
    rc = PtlEQWait(eqh, &ev);

    // LONG DOUBLE

    auto ld = new long double;
    *ld     = 23;

    ptl_me_t mepar_ld;
    auto meh_ld = new ptl_handle_me_t;

    memset(&mepar_ld, 0, sizeof(ptl_me_t));
    mepar_ld.start       = ld;
    mepar_ld.length      = sizeof(long double);
    mepar_ld.ct_handle   = NULL;
    mepar_ld.match_bits  = LONG_DOUBLE_ME;
    mepar_ld.ignore_bits = 0;
    mepar_ld.uid         = PTL_UID_ANY;
    mepar_ld.options     = PTL_ME_OP_PUT | PTL_ME_OP_GET;

    rc = PtlMEAppend(nih, pte, &mepar_ld, PTL_PRIORITY_LIST, meh_ld, meh_ld);
    rc = PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_LINK) {
        fprintf(stderr, "Wrong event type, got %u instead of LINK (%u)", ev.type, PTL_EVENT_LINK);
        _exit(1);
    }

    // Wait for requests

    // INT64

    for (;;) {
        rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
        if (rc == PTL_OK)
            break;
    }
    if (ev.type != PTL_EVENT_ATOMIC) {
        fprintf(stderr, "Wrong event type, got %u instead of ATOMIC (%u)", ev.type, PTL_EVENT_ATOMIC);
        _exit(1);
    }
    printf("INT64 : %ld\n", *i64);
    printf("HDR data : %lu\n", ev.hdr_data);
    PtlMEUnlink(*meh_i64);
    delete i64;
    delete meh_i64;

    // LONG DOUBLE

    for (;;) {
        rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
        if (rc == PTL_OK)
            break;
    }
    if (ev.type != PTL_EVENT_ATOMIC) {
        fprintf(stderr, "Wrong event type, got %u instead of ATOMIC (%u)", ev.type, PTL_EVENT_ATOMIC);
        _exit(1);
    }
    printf("LONG DOUBLE : %Lf\n", *ld);
    printf("HDR data : %lu\n", ev.hdr_data);
    PtlMEUnlink(*meh_ld);
    delete ld;
    delete meh_ld;

    // Cleanup

    rc = PtlPTFree(nih, pte);
    if (rc != PTL_OK) {
        ptlerr("PtlPTFree bug", rc);
        _exit(rc);
    }

    rc = PtlEQFree(eqh);
    if (rc != PTL_OK) {
        ptlerr("PtlEQFree bug", rc);
        _exit(rc);
    }

    rc = PtlNIFini(nih);
    if (rc != PTL_OK) {
        ptlerr("PtlNIFini bug", rc);
        _exit(rc);
    }

    PtlFini();

    return 0;
}

int main(int argc, char* argv[])
{
    // the client has a parameter (who the server is)
    return argc > 1 ? client(argv[1]) : server();
}
