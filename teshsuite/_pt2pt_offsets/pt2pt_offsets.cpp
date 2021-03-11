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

    int64_t* i64 = (int64_t*)malloc(4 * sizeof(int64_t));
    *i64         = -666;
    *(i64 + 1)   = 27;
    *(i64 + 2)   = 42;
    *(i64 + 3)   = 666;

    mdpar.start     = i64;
    mdpar.length    = 4 * sizeof(int64_t);
    mdpar.eq_handle = eqh;
    mdpar.ct_handle = PTL_CT_NONE;
    mdpar.options   = PTL_MD_EVENT_SEND_DISABLE;

    rc = PtlMDBind(nih, &mdpar, &mdh);

    // =================
    // Local offset test
    // =================

    printf("--- Local offset test\n");
    sleep(1); // Make sure Server is done posting ME

    rc = PtlPut(mdh, 2 * sizeof(int64_t), sizeof(int64_t), PTL_ACK_REQ, peer, 0, 42, 0, nullptr, ~0ULL);

    PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_ACK) {
        fprintf(stderr, "Wrong event type, got %u instead of ACK (%u)", ev.type, PTL_EVENT_ACK);
        exit(1);
    }

    rc = PtlFetchAtomic(mdh, 3 * sizeof(int64_t), mdh, sizeof(int64_t), sizeof(int64_t), peer, 0, 42, 0, nullptr, ~0ULL,
                        PTL_SUM, PTL_INT64_T);
    PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_REPLY) {
        fprintf(stderr, "Wrong event type, got %u instead of REPLY (%u)", ev.type, PTL_EVENT_REPLY);
        exit(1);
    }
    printf("Target value fetched in atomic : %ld\n", *(i64 + 3));

    // ==================
    // Remote offset test
    // ==================

    printf("--- Remote offset test\n");
    sleep(1); // Make sure Server is done posting ME

    rc = PtlPut(mdh, 2 * sizeof(int64_t), sizeof(int64_t), PTL_ACK_REQ, peer, 0, 42, sizeof(int64_t), nullptr, ~0ULL);
    PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_ACK) {
        fprintf(stderr, "Wrong event type, got %u instead of ACK (%u)", ev.type, PTL_EVENT_ACK);
        exit(1);
    }

    // =================
    // MANAGE_LOCAL test
    // =================

    printf("--- MANAGE_LOCAL test\n");
    sleep(1); // Make sure Server is done posting ME

    rc = PtlPut(mdh, sizeof(int64_t), sizeof(int64_t), PTL_ACK_REQ, peer, 0, 42, 666 * sizeof(int64_t), nullptr, ~0ULL);
    rc = PtlPut(mdh, 2 * sizeof(int64_t), sizeof(int64_t), PTL_ACK_REQ, peer, 0, 42, 666 * sizeof(int64_t), nullptr,
                ~0ULL);
    PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_ACK) {
        fprintf(stderr, "Wrong event type, got %u instead of ACK (%u)", ev.type, PTL_EVENT_ACK);
        exit(1);
    }
    PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_ACK) {
        fprintf(stderr, "Wrong event type, got %u instead of ACK (%u)", ev.type, PTL_EVENT_ACK);
        exit(1);
    }

    rc = PtlGet(mdh, 3 * sizeof(int64_t), sizeof(int64_t), peer, 0, 42, 666 * sizeof(int64_t), nullptr);

    PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_REPLY) {
        fprintf(stderr, "Wrong event type, got %u instead of REPLY %u", ev.type, PTL_EVENT_REPLY);
        exit(1);
    }

    sleep(1);

    printf("Target value fetched in get : %ld\n", *(i64 + 3));

    // Cleanup

    rc = PtlMDRelease(mdh);

    free(i64);

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

    // =================
    // Local offset test
    // =================

    auto i64 = (int64_t*)malloc(sizeof(int64_t));
    *i64     = 0;

    ptl_me_t mepar_i64;
    auto meh_i64 = (ptl_handle_me_t*)malloc(sizeof(ptl_handle_me_t));

    memset(&mepar_i64, 0, sizeof(ptl_me_t));
    mepar_i64.start       = i64;
    mepar_i64.length      = sizeof(int64_t);
    mepar_i64.ct_handle   = NULL;
    mepar_i64.match_bits  = 42;
    mepar_i64.ignore_bits = 0;
    mepar_i64.uid         = PTL_UID_ANY;
    mepar_i64.options     = PTL_ME_OP_PUT | PTL_ME_OP_GET;

    rc = PtlMEAppend(nih, pte, &mepar_i64, PTL_PRIORITY_LIST, meh_i64, meh_i64);
    rc = PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_LINK) {
        fprintf(stderr, "Wrong event type, got %u instead of LINK (%u)", ev.type, PTL_EVENT_LINK);
        exit(1);
    }

    // Wait for requests

    for (;;) {
        rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
        if (rc == PTL_OK)
            break;
    }
    if (ev.type != PTL_EVENT_PUT) {
        fprintf(stderr, "Wrong event type, got %u instead of PUT (%u)", ev.type, PTL_EVENT_PUT);
        exit(1);
    }
    printf("Target after PUT : %ld\n", *i64);

    for (;;) {
        rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
        if (rc == PTL_OK)
            break;
    }
    if (ev.type != PTL_EVENT_FETCH_ATOMIC) {
        fprintf(stderr, "Wrong event type, got %u instead of FETCH_ATOMIC (%u)", ev.type, PTL_EVENT_FETCH_ATOMIC);
        exit(1);
    }
    printf("Target after FETCH_ATOMIC : %ld\n", *i64);

    PtlMEUnlink(*meh_i64);
    free(i64);
    free(meh_i64);

    // ==================
    // Remote offset test
    // ==================

    i64        = (int64_t*)malloc(3 * sizeof(int64_t));
    *i64       = 0;
    *(i64 + 1) = 0;
    *(i64 + 2) = 0;

    meh_i64 = (ptl_handle_me_t*)malloc(sizeof(ptl_handle_me_t));

    memset(&mepar_i64, 0, sizeof(ptl_me_t));
    mepar_i64.start       = i64;
    mepar_i64.length      = 3 * sizeof(int64_t);
    mepar_i64.ct_handle   = NULL;
    mepar_i64.match_bits  = 42;
    mepar_i64.ignore_bits = 0;
    mepar_i64.uid         = PTL_UID_ANY;
    mepar_i64.options     = PTL_ME_OP_PUT;

    rc = PtlMEAppend(nih, pte, &mepar_i64, PTL_PRIORITY_LIST, meh_i64, meh_i64);
    rc = PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_LINK) {
        fprintf(stderr, "Wrong event type, got %u instead of LINK (%u)", ev.type, PTL_EVENT_LINK);
        exit(1);
    }

    // Wait for requests

    for (;;) {
        rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
        if (rc == PTL_OK)
            break;
    }
    if (ev.type != PTL_EVENT_PUT) {
        fprintf(stderr, "Wrong event type, got %u instead of PUT (%u)", ev.type, PTL_EVENT_PUT);
        exit(1);
    }
    printf("Target[0] after PUT : %ld\n", *i64);
    printf("Target[1] after PUT : %ld\n", *(i64 + 1));
    printf("Target[2] after PUT : %ld\n", *(i64 + 2));

    PtlMEUnlink(*meh_i64);
    free(i64);
    free(meh_i64);

    // =================
    // MANAGE_LOCAL test
    // =================

    i64        = (int64_t*)malloc(4 * sizeof(int64_t));
    *i64       = 999;
    *(i64 + 1) = 999;
    *(i64 + 2) = 69;
    *(i64 + 3) = 999;

    meh_i64 = (ptl_handle_me_t*)malloc(sizeof(ptl_handle_me_t));

    memset(&mepar_i64, 0, sizeof(ptl_me_t));
    mepar_i64.start       = i64;
    mepar_i64.length      = 4 * sizeof(int64_t);
    mepar_i64.ct_handle   = NULL;
    mepar_i64.match_bits  = 42;
    mepar_i64.ignore_bits = 0;
    mepar_i64.min_free    = sizeof(int64_t) + 1; // So that the *(i64 + 3) isn't used
    mepar_i64.uid         = PTL_UID_ANY;
    mepar_i64.options     = PTL_ME_OP_PUT | PTL_ME_OP_GET | PTL_ME_MANAGE_LOCAL;

    rc = PtlMEAppend(nih, pte, &mepar_i64, PTL_PRIORITY_LIST, meh_i64, meh_i64);
    rc = PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_LINK) {
        fprintf(stderr, "Wrong event type, got %u instead of LINK (%u)", ev.type, PTL_EVENT_LINK);
        exit(1);
    }

    // Wait for 2 Put
    for (int k = 0; k < 2; ++k) {
        for (;;) {
            rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
            if (rc == PTL_OK)
                break;
        }
        if (ev.type != PTL_EVENT_PUT) {
            fprintf(stderr, "Wrong event type, got %u instead of PUT (%u)", ev.type, PTL_EVENT_PUT);
            exit(1);
        }
        printf("Target[0] after PUT : %ld\n", *i64);
        printf("Target[1] after PUT : %ld\n", *(i64 + 1));
        printf("Target[2] after PUT : %ld\n", *(i64 + 2));
        printf("Target[3] after PUT : %ld\n", *(i64 + 3));
    }

    // Wait for 1 Get and the auto unlink (because the offset has reached min_free)
    for (int k = 0; k < 2; ++k) {
        for (;;) {
            rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
            if (rc == PTL_OK)
                break;
        }
        if (ev.type != PTL_EVENT_GET && ev.type != PTL_EVENT_AUTO_UNLINK) {
            fprintf(stderr, "Wrong event type, got %u instead of GET or AUTO_UNLINK (%u or %u)", ev.type, PTL_EVENT_GET,
                    PTL_EVENT_AUTO_UNLINK);
            exit(1);
        }
    }
    printf("Target[0] after GET : %ld\n", *i64);
    printf("Target[1] after GET : %ld\n", *(i64 + 1));
    printf("Target[2] after GET : %ld\n", *(i64 + 2));
    printf("Target[3] after GET : %ld\n", *(i64 + 3));

    sleep(1); // Make sure Client is done copying data (/!\ this should not be necessary)

    free(i64);
    free(meh_i64);

    // Cleanup

    rc = PtlPTFree(nih, pte);
    if (rc != PTL_OK) {
        fprintf(stderr, "PtlPTFree bug\n");
        exit(rc);
    }

    rc = PtlEQFree(eqh);
    if (rc != PTL_OK) {
        fprintf(stderr, "PtlEQFree bug\n");
        exit(rc);
    }

    rc = PtlNIFini(nih);
    if (rc != PTL_OK) {
        fprintf(stderr, "PtlNIFini bug\n");
        exit(rc);
    }

    PtlFini();

    return 0;
}

int main(int argc, char* argv[])
{
    // the client has a parameter (who the server is)
    return argc > 1 ? client(argv[1]) : server();
}
