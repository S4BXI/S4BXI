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

#define RUNS_NUMBER    11
#define MESSAGE_NUMBER 1000

void ptlerr(std::string str, int rc)
{
    fprintf(stderr, "%s: %s\n", str.c_str(), PtlToStr(rc, PTL_STR_ERROR));
}

int client(char* target)
{
    int target_nid = atoi(target);

    ptl_handle_ni_t nih;
    int rc;
    rc = PtlInit();
    if (rc != PTL_OK) {
        ptlerr("PtlInit", rc);
        exit(rc);
    }

    rc = PtlNIInit(PTL_IFACE_DEFAULT, PTL_NI_MATCHING | PTL_NI_PHYSICAL, 123, NULL, NULL, &nih);
    if (rc != PTL_OK) {
        ptlerr("PtlNIInit", rc);
        exit(rc);
    }

    ptl_md_t mdpar;
    ptl_process_t peer;
    ptl_ct_event_t ev;

    ptl_handle_ct_t cth;

    rc = PtlCTAlloc(nih, &cth);
    if (rc != PTL_OK) {
        ptlerr("PtlCTAlloc", rc);
        exit(rc);
    }

    peer.phys.nid = target_nid;
    peer.phys.pid = 123;

    int buffer_size = 1;
    double comm_time, total_size_kB;
    unsigned int sent_messages = 0;

    ptl_ct_event_t empty_event{0, 0};
    for (int i = 0; i < RUNS_NUMBER; ++i) {
        char* buf = (char*)S4BXI_SHARED_MALLOC(buffer_size * sizeof(char));
        sprintf(buf, "Message of run %d", i);

        ptl_md_t mdpar;
        ptl_handle_md_t mdh;

        memset(&mdpar, 0, sizeof(ptl_md_t));
        mdpar.start     = buf;
        mdpar.length    = buffer_size;
        mdpar.eq_handle = PTL_EQ_NONE;
        mdpar.ct_handle = cth;
        mdpar.options   = PTL_MD_EVENT_CT_ACK;

        rc = PtlMDBind(nih, &mdpar, &mdh);
        if (rc != PTL_OK) {
            ptlerr("PtlMDBind", rc);
            exit(rc);
        }

        sleep(1);

        PtlCTSet(cth, empty_event);

        for (int j = 0; j < MESSAGE_NUMBER;) {
            // Should match with 3rd ME (40 + 2)
            rc = PtlPut(mdh, 0, buffer_size, PTL_ACK_REQ, peer, 0, 42, 0, nullptr, ~0ULL);

            PtlCTWait(cth, ++j, &ev);
        }

        rc = PtlMDRelease(mdh);

        S4BXI_SHARED_FREE(buf);

        buffer_size *= 4;
        printf("Finished run %d\n", i);
    }

    s4bxi_barrier();

    rc = PtlCTFree(cth);

    rc = PtlNIFini(nih);
    PtlFini();

    return 0;
}

int server()
{
    ptl_handle_ni_t nih;
    int rc;
    rc = PtlInit();
    if (rc != PTL_OK) {
        ptlerr("PtlInit", rc);
        exit(rc);
    }

    rc = PtlNIInit(PTL_IFACE_DEFAULT, PTL_NI_MATCHING | PTL_NI_PHYSICAL, 123, NULL, NULL, &nih);
    if (rc != PTL_OK) {
        ptlerr("PtlNIInit", rc);
        exit(rc);
    }

    ptl_md_t mdpar;
    ptl_process_t peer;
    ptl_ct_event_t ev;

    ptl_handle_ct_t cth;

    rc = PtlCTAlloc(nih, &cth);
    if (rc != PTL_OK) {
        ptlerr("PtlCTAlloc", rc);
        exit(rc);
    }

    char** bufs;
    auto meh_pool = (ptl_handle_me_t*)calloc(10, sizeof(ptl_handle_me_t));

    bufs = (char**)malloc(10 * sizeof(char*));

    ptl_pt_index_t pte;

    rc = PtlPTAlloc(nih, 0, PTL_EQ_NONE, 0, &pte);

    if (rc != PTL_OK) {
        ptlerr("PtlPTAlloc", rc);
        exit(rc);
    }

    int i, j;

    for (i = 0; i < 10; i++) {
        bufs[i] = (char*)S4BXI_SHARED_MALLOC(4200000 * sizeof(char));

        ptl_me_t mepar;
        ptl_event_t ev_link;

        memset(&mepar, 0, sizeof(ptl_me_t));
        mepar.start       = bufs[i];
        mepar.length      = 4200000;
        mepar.ct_handle   = cth;
        mepar.match_bits  = 40 + i;
        mepar.ignore_bits = 0;
        mepar.uid         = PTL_UID_ANY;
        mepar.options     = PTL_ME_OP_PUT | PTL_ME_OP_GET | PTL_ME_EVENT_CT_COMM;

        int rc = PtlMEAppend(nih, pte, &mepar, PTL_PRIORITY_LIST, NULL, &meh_pool[i]);

        if (rc != PTL_OK) {
            ptlerr("PtlMEAppend", rc);
            exit(rc);
        }
    }

    unsigned int which;
    int buffer_size;
    double comm_time, total_size_kB, start;

    ptl_size_t test = MESSAGE_NUMBER;

    ptl_ct_event_t empty_event{0, 0};
    for (i = 0; i < RUNS_NUMBER; ++i) {
        PtlCTSet(cth, empty_event);
        for (;;) {
            rc = PtlCTPoll(&cth, &test, 1, 5000, &ev, &which);
            if (rc == PTL_OK)
                break;
        }

        printf("First buffer : %s\n", bufs[0]);
        printf("Third buffer : %s\n", bufs[2]);
    }

    s4bxi_barrier();

    for (i = 0; i < 10; i++) {
        S4BXI_SHARED_FREE(bufs[i]);
        PtlMEUnlink(meh_pool[i]);
    }
    free(bufs);
    free(meh_pool);
    PtlPTFree(nih, pte);
    PtlCTFree(cth);
    PtlNIFini(nih);
    PtlFini();

    return 0;
}

int main(int argc, char* argv[])
{
    // the client has a parameter (who the server is)
    return argc > 1 ? client(argv[1]) : server();
}
