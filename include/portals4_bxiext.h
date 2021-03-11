/*
 * Copyright (c) 2012-2016 Bull S.A.S
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef PORTALS4_BXIEXT_H
#define PORTALS4_BXIEXT_H

#include <stddef.h>

/* Size of the buffer to be passed to 'ptl_evtostr' function */
#define PTL_EV_STR_SIZE 256

#ifdef __cplusplus
extern "C" {
#endif

enum ptl_str_type {
    PTL_STR_ERROR, /* Return codes */
    PTL_STR_EVENT, /* Events */
    PTL_STR_FAIL_TYPE /* Failure type */
};

struct ptl_ev_desc {
    char *name;
    int type;
#define PTL_EV_HAS_START 0x1
#define PTL_EV_HAS_UPTR  0x2
#define PTL_EV_HAS_HDR   0x4
#define PTL_EV_HAS_BITS  0x8
#define PTL_EV_HAS_RLEN  0x10
#define PTL_EV_HAS_MLEN  0x20
#define PTL_EV_HAS_ROFFS 0x40
#define PTL_EV_HAS_UID   0x80
#define PTL_EV_HAS_INIT  0x100
#define PTL_EV_HAS_LIST  0x200
#define PTL_EV_HAS_PT    0x400
#define PTL_EV_HAS_AOP   0x800
#define PTL_EV_HAS_ATYP  0x1000
    int flags, fail_flags;
};

extern struct ptl_ev_desc ptl_ev_desc[] __attribute__((visibility("default")));

const char *PtlToStr(int rc, enum ptl_str_type type);
int PtlFailTypeSize(void);

#ifdef __cplusplus
}
#endif

#endif /* PORTALS4_BXIEXT_H */
