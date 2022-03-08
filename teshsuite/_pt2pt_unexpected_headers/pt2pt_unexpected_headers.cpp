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

#include <portals4.h>
#include <portals4_bxiext.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>

void ptlerr(std::string str, int rc)
{
    fprintf(stderr, "%s: %s\n", str.c_str(), PtlToStr(rc, PTL_STR_ERROR));
}

int client(char* target)
{
    int target_nid = atoi(target);

    int rc = PtlInit();
    if (rc != PTL_OK) {
        ptlerr("client: PtlInit", rc);
        return rc;
    }
    ptl_handle_ni_t nih;
    rc = PtlNIInit(PTL_IFACE_DEFAULT, PTL_NI_MATCHING | PTL_NI_PHYSICAL, 2345, NULL, NULL, &nih);
    if (rc != PTL_OK) {
        ptlerr("client: PtlNIInit", rc);
        return rc;
    }

    ptl_md_t mdpar;
    ptl_handle_eq_t eqh;
    ptl_handle_md_t mdh;
    ptl_process_t peer;
    ptl_event_t ev;

    rc = PtlEQAlloc(nih, 64, &eqh);
    if (rc != PTL_OK) {
        ptlerr("client: PtlEQAlloc", rc);
        return rc;
    }

    peer.phys.nid = target_nid;
    peer.phys.pid = 2345;

    double comm_time, total_size_kB;

    int64_t* i64 = (int64_t*)S4BXI_SHARED_MALLOC(sizeof(int64_t));
    *i64         = 42;

    mdpar.start     = i64;
    mdpar.length    = sizeof(int64_t);
    mdpar.eq_handle = eqh;
    mdpar.ct_handle = PTL_CT_NONE;
    mdpar.options   = PTL_MD_EVENT_SEND_DISABLE;

    rc = PtlMDBind(nih, &mdpar, &mdh);
    if (rc != PTL_OK) {
        ptlerr("client: PtlMDBind", rc);
        return rc;
    }

    s4bxi_barrier();

    rc = PtlPut(mdh, 0, sizeof(int64_t), PTL_ACK_REQ, peer, 0, 42, 0, NULL, ~0ULL);
    if (rc != PTL_OK) {
        ptlerr("client: PtlPut", rc);
        return rc;
    }

    PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_ACK) {
        fprintf(stderr, "Wrong event type, got %u instead of ACK (%u)", ev.type, PTL_EVENT_ACK);
        return 1;
    }

    rc = PtlMDRelease(mdh);
    if (rc != PTL_OK) {
        ptlerr("client: PtlMDRelease", rc);
        return rc;
    }

    S4BXI_SHARED_FREE(i64);

    rc = PtlEQFree(eqh);
    if (rc != PTL_OK) {
        ptlerr("client: PtlEQFree", rc);
        return rc;
    }

    rc = PtlNIFini(nih);
    if (rc != PTL_OK) {
        ptlerr("client: PtlNIFini", rc);
        return rc;
    }
    PtlFini();

    return 0;
}

int server()
{
    unsigned int which;

    ptl_handle_ni_t nih;
    int rc = PtlInit();
    if (rc != PTL_OK) {
        ptlerr("server: PtlInit", rc);
        return rc;
    }

    rc = PtlNIInit(PTL_IFACE_DEFAULT, PTL_NI_MATCHING | PTL_NI_PHYSICAL, 2345, NULL, NULL, &nih);

    ptl_handle_eq_t eqh;

    rc = PtlEQAlloc(nih, 64, &eqh);
    if (rc != PTL_OK) {
        ptlerr("server: PtlEQAlloc", rc);
        return rc;
    }

    ptl_pt_index_t pte;

    rc = PtlPTAlloc(nih, 0, eqh, 0, &pte);
    if (rc != PTL_OK) {
        ptlerr("server: PtlPTAlloc", rc);
        return rc;
    }
    ptl_event_t ev;

    int64_t* overflow_buf = (int64_t*)S4BXI_SHARED_MALLOC(sizeof(int64_t));
    *overflow_buf         = 0;

    ptl_me_t mepar_overflow;
    ptl_handle_me_t* meh_overflow = (ptl_handle_me_t*)malloc(sizeof(ptl_handle_me_t));

    memset(&mepar_overflow, 0, sizeof(ptl_me_t));
    mepar_overflow.start       = overflow_buf;
    mepar_overflow.length      = sizeof(int64_t);
    mepar_overflow.ct_handle   = NULL;
    mepar_overflow.match_bits  = 42;
    mepar_overflow.ignore_bits = 0;
    mepar_overflow.uid         = PTL_UID_ANY;
    mepar_overflow.options     = PTL_ME_OP_PUT | PTL_ME_USE_ONCE;

    rc = PtlMEAppend(nih, pte, &mepar_overflow, PTL_OVERFLOW_LIST, meh_overflow, meh_overflow);
    if (rc != PTL_OK) {
        ptlerr("server: PtlMEAppend", rc);
        return rc;
    }

    rc = PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_LINK) {
        fprintf(stderr, "Wrong event type, got %u instead of LINK (%u)", ev.type, PTL_EVENT_LINK);
        return 1;
    }

    s4bxi_barrier();

    // Wait for request

    for (;;) {
        rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
        if (rc == PTL_OK)
            break;
    }
    if (ev.type != PTL_EVENT_AUTO_UNLINK) {
        fprintf(stderr, "Wrong event type, got %u instead of PTL_EVENT_AUTO_UNLINK (%u)", ev.type,
                PTL_EVENT_AUTO_UNLINK);
        return 1;
    }

    int64_t* priority_buf = (int64_t*)S4BXI_SHARED_MALLOC(sizeof(int64_t));
    *priority_buf         = 0;

    ptl_me_t mepar_priority;
    ptl_handle_me_t* meh_priority = (ptl_handle_me_t*)malloc(sizeof(ptl_handle_me_t));

    memset(&mepar_priority, 0, sizeof(ptl_me_t));
    mepar_priority.start       = priority_buf;
    mepar_priority.length      = sizeof(int64_t);
    mepar_priority.ct_handle   = NULL;
    mepar_priority.match_bits  = 42;
    mepar_priority.ignore_bits = 0;
    mepar_priority.uid         = PTL_UID_ANY;
    mepar_priority.options     = PTL_ME_OP_PUT | PTL_ME_USE_ONCE;

    rc = PtlMEAppend(nih, pte, &mepar_priority, PTL_PRIORITY_LIST, meh_priority, meh_priority);
    if (rc != PTL_OK) {
        ptlerr("server: PtlMEAppend", rc);
        return rc;
    }

    rc = PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_PUT_OVERFLOW) {
        fprintf(stderr, "Wrong event type, got %u instead of PUT_OVERFLOW (%u)", ev.type, PTL_EVENT_PUT_OVERFLOW);
        return 1;
    }

    // No link event : ME is not inserted

    printf("Target after PUT : overflow %ld - priority %ld\n", *overflow_buf, *priority_buf);

    // PtlMEUnlink(*meh_priority);
    S4BXI_SHARED_FREE(overflow_buf);
    S4BXI_SHARED_FREE(priority_buf);
    free(meh_overflow);
    free(meh_priority);

    // Cleanup

    rc = PtlPTFree(nih, pte);
    if (rc != PTL_OK) {
        ptlerr("server: PtlPTFree", rc);
        return rc;
    }

    rc = PtlEQFree(eqh);
    if (rc != PTL_OK) {
        ptlerr("server: PtlEQFree", rc);
        return rc;
    }

    rc = PtlNIFini(nih);
    if (rc != PTL_OK) {
        ptlerr("server: PtlNIFini", rc);
        return rc;
    }

    PtlFini();

    return 0;
}

int main(int argc, char* argv[])
{
    // the client has a parameter (who the server is)
    return argc > 1 ? client(argv[1]) : server();
}
