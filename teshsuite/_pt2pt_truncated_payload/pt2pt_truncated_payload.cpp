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

#define RUNS_NUMBER 4

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
    char* buf = (char*)malloc(512 * sizeof(char));
    sprintf(buf, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");

    rc = PtlEQAlloc(nih, 64, &eqh);

    peer.phys.nid = target_nid;
    peer.phys.pid = 123;

    int buffer_size = 1;
    double comm_time, total_size_kB;

    mdpar.start     = buf;
    mdpar.length    = 512;
    mdpar.eq_handle = eqh;
    mdpar.ct_handle = PTL_CT_NONE;
    mdpar.options   = 0;

    rc = PtlMDBind(nih, &mdpar, &mdh);

    for (int i = 0; i < RUNS_NUMBER; ++i) {
        sleep(2);

        rc = PtlPut(mdh, 0, buffer_size, PTL_ACK_REQ, peer, 0, 42, 0, nullptr, 0);

        PtlEQWait(eqh, &ev);

        if (ev.type != PTL_EVENT_SEND) {
            fprintf(stderr, "Wrong event type, got %u instead of SEND (%u)", ev.type, PTL_EVENT_SEND);
            return 1;
        }

        printf("SEND mlength %ld\n", ev.mlength);

        PtlEQWait(eqh, &ev);

        if (ev.type != PTL_EVENT_ACK) {
            fprintf(stderr, "Wrong event type, got %u instead of ACK (%u)", ev.type, PTL_EVENT_ACK);
            return 1;
        }

        buffer_size *= 4;

        sleep(1);

        printf("ACK mlength %ld\n", ev.mlength);
    }

    rc = PtlMDRelease(mdh);

    rc = PtlEQFree(eqh);

    rc = PtlNIFini(nih);
    PtlFini();

    free(buf);

    return 0;
}

int server()
{
    ptl_handle_ni_t nih;
    int rc = PtlInit();

    rc = PtlNIInit(PTL_IFACE_DEFAULT, PTL_NI_MATCHING | PTL_NI_PHYSICAL, 123, NULL, NULL, &nih);

    ptl_handle_eq_t eqh;
    ptl_me_t mepar;
    ptl_handle_me_t meh;

    rc = PtlEQAlloc(nih, 64, &eqh);

    ptl_pt_index_t pte;

    rc = PtlPTAlloc(nih, 0, eqh, 0, &pte);
    ptl_event_t ev;

    int i;

    char* buf = (char*)malloc(12 * sizeof(char));
    memset(&mepar, 0, sizeof(ptl_me_t));
    mepar.start       = buf;
    mepar.length      = 11;
    mepar.ct_handle   = NULL;
    mepar.match_bits  = 42;
    mepar.ignore_bits = 0;
    mepar.uid         = PTL_UID_ANY;
    mepar.options     = PTL_LE_OP_PUT | PTL_LE_OP_GET;

    rc = PtlMEAppend(nih, pte, &mepar, PTL_PRIORITY_LIST, nullptr, &meh);
    rc = PtlEQWait(eqh, &ev);
    if (ev.type != PTL_EVENT_LINK) {
        fprintf(stderr, "Wrong event type, got %u instead of LINK (%u)", ev.type, PTL_EVENT_LINK);
        return 1;
    }

    unsigned int which;
    int buffer_size;
    double comm_time, total_size_kB, start;

    for (i = 0; i < RUNS_NUMBER; ++i) {
        for (;;) {
            rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
            if (rc == PTL_OK)
                break;
        }

        if (ev.type != PTL_EVENT_PUT) {
            fprintf(stderr, "Wrong event type, got %u instead of PUT (%u)", ev.type, PTL_EVENT_PUT);
            return 1;
        }

        sleep(1);

        buf[ev.mlength > 11 ? 11 : ev.mlength] = '\0'; // Terminate the string properly
        printf("Message : %s\n", buf);
        printf("PUT mlength : %ld\n", ev.mlength);
    }

    free(buf);

    PtlMEUnlink(meh);

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
