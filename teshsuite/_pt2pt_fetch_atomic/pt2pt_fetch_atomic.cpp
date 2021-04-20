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
#define SWAP_ME        3

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

    ptl_md_t mdpar_put, mdpar_get;
    ptl_handle_eq_t eqh;
    ptl_handle_md_t mdh_put, mdh_get;
    ptl_process_t peer;
    ptl_event_t ev;

    rc = PtlEQAlloc(nih, 64, &eqh);

    peer.phys.nid = target_nid;
    peer.phys.pid = 123;

    double comm_time, total_size_kB;

    // INT64

    auto i64_put = (int64_t*)S4BXI_SHARED_MALLOC(sizeof(int64_t));
    auto i64_get = (int64_t*)S4BXI_SHARED_MALLOC(sizeof(int64_t));
    *i64_put     = 2;
    *i64_get     = 666;

    mdpar_put.start     = i64_put;
    mdpar_put.length    = sizeof(int64_t);
    mdpar_put.eq_handle = eqh;
    mdpar_put.ct_handle = PTL_CT_NONE;
    mdpar_put.options   = PTL_MD_EVENT_SEND_DISABLE;

    mdpar_get.start     = i64_get;
    mdpar_get.length    = sizeof(int64_t);
    mdpar_get.eq_handle = eqh;
    mdpar_get.ct_handle = PTL_CT_NONE;
    mdpar_get.options   = PTL_MD_EVENT_SEND_DISABLE;

    rc = PtlMDBind(nih, &mdpar_put, &mdh_put);
    rc = PtlMDBind(nih, &mdpar_get, &mdh_get);

    sleep(1); // Make sure Server is done posting ME

    rc = PtlFetchAtomic(mdh_get, 0, mdh_put, 0, sizeof(int64_t), peer, 0, INT64T_ME, 0, nullptr, 1337, PTL_SUM,
                        PTL_INT64_T);

    PtlEQWait(eqh, &ev);

    rc = PtlMDRelease(mdh_put);
    rc = PtlMDRelease(mdh_get);

    s4bxi_barrier();

    printf("Initial INT64 : %ld\n", *i64_get);

    S4BXI_SHARED_FREE(i64_put);
    S4BXI_SHARED_FREE(i64_get);

    // LONG_DOUBLE

    auto ld_put = (long double*)S4BXI_SHARED_MALLOC(sizeof(long double));
    auto ld_get = (long double*)S4BXI_SHARED_MALLOC(sizeof(long double));
    *ld_put     = 3;
    *ld_get     = 666;

    mdpar_put.start     = ld_put;
    mdpar_put.length    = sizeof(long double);
    mdpar_put.eq_handle = eqh;
    mdpar_put.ct_handle = PTL_CT_NONE;
    mdpar_put.options   = PTL_MD_EVENT_SEND_DISABLE;

    mdpar_get.start     = ld_get;
    mdpar_get.length    = sizeof(long double);
    mdpar_get.eq_handle = eqh;
    mdpar_get.ct_handle = PTL_CT_NONE;
    mdpar_get.options   = PTL_MD_EVENT_SEND_DISABLE;

    rc = PtlMDBind(nih, &mdpar_put, &mdh_put);
    rc = PtlMDBind(nih, &mdpar_get, &mdh_get);

    rc = PtlFetchAtomic(mdh_get, 0, mdh_put, 0, sizeof(long double), peer, 0, LONG_DOUBLE_ME, 0, nullptr, 7331,
                        PTL_PROD, PTL_LONG_DOUBLE);

    PtlEQWait(eqh, &ev);

    rc = PtlMDRelease(mdh_put);
    rc = PtlMDRelease(mdh_get);

    s4bxi_barrier();

    printf("Initial LONG DOUBLE : %Lf\n", *ld_get);

    S4BXI_SHARED_FREE(ld_put);
    S4BXI_SHARED_FREE(ld_get);

    // SWAP

    auto swap_put = (long double*)S4BXI_SHARED_MALLOC(sizeof(long double));
    auto swap_get = (long double*)S4BXI_SHARED_MALLOC(sizeof(long double));
    *swap_put     = 42;
    *swap_get     = 666;

    mdpar_put.start     = swap_put;
    mdpar_put.length    = sizeof(long double);
    mdpar_put.eq_handle = eqh;
    mdpar_put.ct_handle = PTL_CT_NONE;
    mdpar_put.options   = PTL_MD_EVENT_SEND_DISABLE;

    mdpar_get.start     = swap_get;
    mdpar_get.length    = sizeof(long double);
    mdpar_get.eq_handle = eqh;
    mdpar_get.ct_handle = PTL_CT_NONE;
    mdpar_get.options   = PTL_MD_EVENT_SEND_DISABLE;

    rc = PtlMDBind(nih, &mdpar_put, &mdh_put);
    rc = PtlMDBind(nih, &mdpar_get, &mdh_get);

    rc = PtlSwap(mdh_get, 0, mdh_put, 0, sizeof(long double), peer, 0, SWAP_ME, 0, nullptr, 3333, (void*)0x42, PTL_SWAP,
                 PTL_LONG_DOUBLE);

    PtlEQWait(eqh, &ev);

    rc = PtlMDRelease(mdh_put);
    rc = PtlMDRelease(mdh_get);

    s4bxi_barrier();

    printf("Initial SWAP : %Lf\n", *swap_get);

    S4BXI_SHARED_FREE(swap_put);
    S4BXI_SHARED_FREE(swap_get);

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

    auto i64 = (int64_t*)S4BXI_SHARED_MALLOC(sizeof(int64_t));
    *i64     = 40;

    ptl_me_t mepar_i64;
    auto meh_i64 = (ptl_handle_me_t*)malloc(sizeof(ptl_handle_me_t));

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

    auto ld = (long double*)S4BXI_SHARED_MALLOC(sizeof(long double));
    *ld     = 23;

    ptl_me_t mepar_ld;
    auto meh_ld = (ptl_handle_me_t*)malloc(sizeof(ptl_handle_me_t));

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
        exit(1);
    }

    // SWAP

    auto swap = (long double*)S4BXI_SHARED_MALLOC(sizeof(long double));
    *swap     = 12;

    ptl_me_t mepar_swap;
    auto meh_swap = (ptl_handle_me_t*)malloc(sizeof(ptl_handle_me_t));

    memset(&mepar_swap, 0, sizeof(ptl_me_t));
    mepar_swap.start       = swap;
    mepar_swap.length      = sizeof(long double);
    mepar_swap.ct_handle   = NULL;
    mepar_swap.match_bits  = SWAP_ME;
    mepar_swap.ignore_bits = 0;
    mepar_swap.uid         = PTL_UID_ANY;
    mepar_swap.options     = PTL_ME_OP_PUT | PTL_ME_OP_GET;

    rc = PtlMEAppend(nih, pte, &mepar_swap, PTL_PRIORITY_LIST, meh_swap, meh_swap);
    rc = PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_LINK) {
        fprintf(stderr, "Wrong event type, got %u instead of LINK (%u)", ev.type, PTL_EVENT_LINK);
        exit(1);
    }

    // Wait for requests

    // INT64

    for (;;) {
        rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
        if (rc == PTL_OK)
            break;
    }
    if (ev.type != PTL_EVENT_FETCH_ATOMIC) {
        fprintf(stderr, "Wrong event type, got %u instead of FETCH_ATOMIC (%u)", ev.type, PTL_EVENT_FETCH_ATOMIC);
        exit(1);
    }
    printf("INT64 : %ld\n", *i64);
    printf("HDR data : %lu\n", ev.hdr_data);

    s4bxi_barrier();

    PtlMEUnlink(*meh_i64);
    S4BXI_SHARED_FREE(i64);
    free(meh_i64);

    // LONG DOUBLE

    for (;;) {
        rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
        if (rc == PTL_OK)
            break;
    }
    if (ev.type != PTL_EVENT_FETCH_ATOMIC) {
        fprintf(stderr, "Wrong event type, got %u instead of FETCH_ATOMIC (%u)", ev.type, PTL_EVENT_FETCH_ATOMIC);
        exit(1);
    }
    printf("LONG DOUBLE : %Lf\n", *ld);
    printf("HDR data : %lu\n", ev.hdr_data);

    s4bxi_barrier();

    PtlMEUnlink(*meh_ld);
    S4BXI_SHARED_FREE(ld);
    free(meh_ld);

    // SWAP

    for (;;) {
        rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
        if (rc == PTL_OK)
            break;
    }
    if (ev.type != PTL_EVENT_FETCH_ATOMIC) { // /!\ No specific SWAP Portals event
        fprintf(stderr, "Wrong event type, got %u instead of FETCH_ATOMIC (%u)", ev.type, PTL_EVENT_FETCH_ATOMIC);
        exit(1);
    }
    printf("SWAP : %Lf\n", *swap);
    printf("HDR data : %lu\n", ev.hdr_data);

    s4bxi_barrier();

    PtlMEUnlink(*meh_swap);
    S4BXI_SHARED_FREE(swap);
    free(meh_swap);

    // Cleanup

    rc = PtlPTFree(nih, pte);
    if (rc != PTL_OK) {
        ptlerr("PtlPTFree bug", rc);
        exit(rc);
    }

    rc = PtlEQFree(eqh);
    if (rc != PTL_OK) {
        ptlerr("PtlEQFree bug", rc);
        exit(rc);
    }

    rc = PtlNIFini(nih);
    if (rc != PTL_OK) {
        ptlerr("PtlNIFini bug", rc);
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
