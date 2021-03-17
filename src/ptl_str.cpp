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

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdlib.h>
#endif
#include <cstdint>
#include "portals4.h"
#include "portals4_bxiext.h"

#define PTL_STR_STRINGIFY(x...) #x

struct ptl_const_to_str {
    int rc;
    char const* str; // Add a const since C++ doesn't allow string litteral cast to char *
};

const struct ptl_const_to_str ptl_errors[] = {
    /* Portals4.1 Return codes Spec Table 3.7 */
    {PTL_ARG_INVALID, PTL_STR_STRINGIFY(PTL_ARG_INVALID)},
    {PTL_CT_NONE_REACHED, PTL_STR_STRINGIFY(PTL_CT_NONE_REACHED)},
    {PTL_EQ_DROPPED, PTL_STR_STRINGIFY(PTL_EQ_DROPPED)},
    {PTL_EQ_EMPTY, PTL_STR_STRINGIFY(PTL_EQ_EMPTY)},
    {PTL_FAIL, PTL_STR_STRINGIFY(PTL_FAIL)},
    {PTL_IGNORED, PTL_STR_STRINGIFY(PTL_IGNORED)},
    {PTL_IN_USE, PTL_STR_STRINGIFY(PTL_IN_USE)},
    {PTL_INTERRUPTED, PTL_STR_STRINGIFY(PTL_INTERRUPTED)},
    {PTL_LIST_TOO_LONG, PTL_STR_STRINGIFY(PTL_LIST_TOO_LONG)},
    {PTL_NO_INIT, PTL_STR_STRINGIFY(PTL_NO_INIT)},
    {PTL_NO_SPACE, PTL_STR_STRINGIFY(PTL_NO_SPACE)},
    {PTL_OK, PTL_STR_STRINGIFY(PTL_OK)},
    {PTL_PID_IN_USE, PTL_STR_STRINGIFY(PTL_PID_IN_USE)},
    {PTL_PT_EQ_NEEDED, PTL_STR_STRINGIFY(PTL_PT_EQ_NEEDED)},
    {PTL_PT_FULL, PTL_STR_STRINGIFY(PTL_PT_FULL)},
    {PTL_PT_IN_USE, PTL_STR_STRINGIFY(PTL_PT_IN_USE)},
    {PTL_FAIL, PTL_STR_STRINGIFY(PTL_FAIL)}};

const struct ptl_const_to_str ptl_events[] = {
    /* Portals4 Events */
    {PTL_EVENT_GET, PTL_STR_STRINGIFY(PTL_EVENT_GET)},
    {PTL_EVENT_GET_OVERFLOW, PTL_STR_STRINGIFY(PTL_EVENT_GET_OVERFLOW)},
    {PTL_EVENT_PUT, PTL_STR_STRINGIFY(PTL_EVENT_PUT)},
    {PTL_EVENT_PUT_OVERFLOW, PTL_STR_STRINGIFY(PTL_EVENT_PUT_OVERFLOW)},
    {PTL_EVENT_ATOMIC, PTL_STR_STRINGIFY(PTL_EVENT_ATOMIC)},
    {PTL_EVENT_ATOMIC_OVERFLOW, PTL_STR_STRINGIFY(PTL_EVENT_ATOMIC_OVERFLOW)},
    {PTL_EVENT_FETCH_ATOMIC, PTL_STR_STRINGIFY(PTL_EVENT_FETCH_ATOMIC)},
    {PTL_EVENT_FETCH_ATOMIC_OVERFLOW, PTL_STR_STRINGIFY(PTL_EVENT_FETCH_ATOMIC_OVERFLOW)},
    {PTL_EVENT_REPLY, PTL_STR_STRINGIFY(PTL_EVENT_REPLY)},
    {PTL_EVENT_SEND, PTL_STR_STRINGIFY(PTL_EVENT_SEND)},
    {PTL_EVENT_ACK, PTL_STR_STRINGIFY(PTL_EVENT_ACK)},
    {PTL_EVENT_PT_DISABLED, PTL_STR_STRINGIFY(PTL_EVENT_PT_DISABLED)},
    {PTL_EVENT_AUTO_UNLINK, PTL_STR_STRINGIFY(PTL_EVENT_AUTO_UNLINK)},
    {PTL_EVENT_AUTO_FREE, PTL_STR_STRINGIFY(PTL_EVENT_AUTO_FREE)},
    {PTL_EVENT_SEARCH, PTL_STR_STRINGIFY(PTL_EVENT_SEARCH)},
    {PTL_EVENT_LINK, PTL_STR_STRINGIFY(PTL_EVENT_LINK)}};

const struct ptl_const_to_str ptl_fail_type[] = {
    /* Portals4 failure type */
    {PTL_NI_OK, PTL_STR_STRINGIFY(PTL_NI_OK)},
    {PTL_NI_PERM_VIOLATION, PTL_STR_STRINGIFY(PTL_NI_PERM_VIOLATION)},
    {PTL_NI_SEGV, PTL_STR_STRINGIFY(PTL_NI_SEGV)},
    {PTL_NI_PT_DISABLED, PTL_STR_STRINGIFY(PTL_NI_PT_DISABLED)},
    {PTL_NI_DROPPED, PTL_STR_STRINGIFY(PTL_NI_DROPPED)},
    {PTL_NI_UNDELIVERABLE, PTL_STR_STRINGIFY(PTL_NI_UNDELIVERABLE)},
    {PTL_FAIL, PTL_STR_STRINGIFY(PTL_FAIL)},
    {PTL_ARG_INVALID, PTL_STR_STRINGIFY(PTL_ARG_INVALID)},
    {PTL_IN_USE, PTL_STR_STRINGIFY(PTL_IN_USE)},
    {PTL_NI_NO_MATCH, PTL_STR_STRINGIFY(PTL_NI_NO_MATCH)},
    {PTL_NI_TARGET_INVALID, PTL_STR_STRINGIFY(PTL_NI_TARGET_INVALID)},
    {PTL_NI_OP_VIOLATION, PTL_STR_STRINGIFY(PTL_NI_OP_VIOLATION)}};

const char* PtlToStr(int rc, enum ptl_str_type type)
{
    int i;
    const struct ptl_const_to_str* p;
    size_t size;

    switch (type) {
    case PTL_STR_ERROR:
        p    = ptl_errors;
        size = sizeof(ptl_errors);
        break;
    case PTL_STR_EVENT:
        p    = ptl_events;
        size = sizeof(ptl_events);
        break;
    case PTL_STR_FAIL_TYPE:
        p    = ptl_fail_type;
        size = sizeof(ptl_fail_type);
        break;
    default:
        return "Unknown";
    };
    for (i = 0; i < size / sizeof(struct ptl_const_to_str); i++) {
        if (rc == p[i].rc)
            return p[i].str;
    }
    return "Unknown";
}

int PtlFailTypeSize(void)
{
    return sizeof(ptl_fail_type) / sizeof(struct ptl_const_to_str);
}
