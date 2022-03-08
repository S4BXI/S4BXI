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

    PtlInit();
    ptl_handle_ni_t nih;
    int rc = PtlNIInit(PTL_IFACE_DEFAULT, PTL_NI_NO_MATCHING | PTL_NI_PHYSICAL, 123, NULL, NULL, &nih);

    ptl_md_t mdpar;
    ptl_handle_eq_t eqh;
    ptl_handle_md_t mdh;
    ptl_process_t peer;
    ptl_event_t ev;

    rc = PtlEQAlloc(nih, 64, &eqh);

    peer.phys.nid = target_nid;
    peer.phys.pid = 123;

    int buffer_size = 1;

    sleep(1); // Wait a second to make sure server is ready

    for (int i = 0; i < RUNS_NUMBER; ++i) {
        char* buf = (char*)S4BXI_SHARED_MALLOC(buffer_size * sizeof(char));
        sprintf(buf, "Message of run %d", i);

        mdpar.start     = buf;
        mdpar.length    = buffer_size;
        mdpar.eq_handle = eqh;
        mdpar.ct_handle = PTL_CT_NONE;
        mdpar.options   = PTL_MD_EVENT_SEND_DISABLE;

        rc = PtlMDBind(nih, &mdpar, &mdh);

        int got_ack = 0;

        for (int j = 0; j < MESSAGE_NUMBER; ++j) {
            rc = PtlPut(mdh, 0, buffer_size, PTL_ACK_REQ, peer, 0, ~0ULL, 0, nullptr, ~0ULL);

            if (buffer_size > 64) {
                PtlEQWait(eqh, &ev);
                ++got_ack;
            }
        }

        for (int j = got_ack; j < MESSAGE_NUMBER; ++j)
            PtlEQWait(eqh, &ev);

        rc = PtlMDRelease(mdh);

        S4BXI_SHARED_FREE(buf);

        buffer_size *= 4;
        printf("Finished run %d\n", i);
    }

    rc = PtlEQFree(eqh);

    rc = PtlNIFini(nih);
    PtlFini();

    return 0;
}

int server()
{
    ptl_handle_ni_t nih;
    int rc = PtlInit();

    rc = PtlNIInit(PTL_IFACE_DEFAULT, PTL_NI_NO_MATCHING | PTL_NI_PHYSICAL, 123, NULL, NULL, &nih);

    ptl_handle_eq_t eqh;
    char** bufs;
    auto lepar    = (ptl_le_t*)calloc(10, sizeof(ptl_le_t));
    auto leh_pool = (ptl_handle_le_t*)calloc(10, sizeof(ptl_handle_le_t));

    rc = PtlEQAlloc(nih, 64, &eqh);

    bufs = (char**)malloc(10 * sizeof(char*));

    ptl_pt_index_t pte;

    rc = PtlPTAlloc(nih, 0, eqh, 0, &pte);
    ptl_event_t ev;

    int i, j;

    for (i = 0; i < 10; i++) {
        bufs[i] = (char*)S4BXI_SHARED_MALLOC(4200000 * sizeof(char));
        memset(&lepar[i], 0, sizeof(ptl_le_t));
        lepar[i].start     = bufs[i];
        lepar[i].length    = 4200000;
        lepar[i].ct_handle = NULL;
        lepar[i].uid       = PTL_UID_ANY;
        lepar[i].options   = PTL_LE_OP_PUT | PTL_LE_OP_GET;

        rc = PtlLEAppend(nih, pte, &lepar[i], PTL_PRIORITY_LIST, leh_pool + i, &leh_pool[i]);
        rc = PtlEQWait(eqh, &ev);
    }

    unsigned int which;

    for (i = 0; i < RUNS_NUMBER; ++i) {
        for (j = 0; j < MESSAGE_NUMBER; ++j) {
            for (;;) {
                rc = PtlEQPoll(&eqh, 1, 5000, &ev, &which);
                if (rc == PTL_OK)
                    break;
            }

            if (j == 0)
                printf("Received : %s\n", bufs[0]);
        }
    }

    for (i = 0; i < 10; i++)
        S4BXI_SHARED_FREE(bufs[i]);
    free(bufs);

    for (i = 0; i < 10; i++) {
        PtlLEUnlink(leh_pool[i]);
    }

    free(leh_pool);
    free(lepar);


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
