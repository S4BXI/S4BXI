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

#include <random>

#include "s4bxi/s4bxi_util.hpp"
#include "portals4.h"
#include "s4bxi/s4bxi_xbt_log.h"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_util, "Messages generated in util functions");

using namespace std;

/*
 * Return the size in bytes of the given atomic type.
 *
 * Imported from swptl and adapted to C++
 */
int ptl_atsize(enum ptl_datatype atype)
{
    switch (atype) {
    case PTL_INT8_T:
    case PTL_UINT8_T:
        return 1;
    case PTL_INT16_T:
    case PTL_UINT16_T:
        return 2;
    case PTL_INT32_T:
    case PTL_UINT32_T:
        return 4;
    case PTL_INT64_T:
    case PTL_UINT64_T:
        return 8;
    case PTL_FLOAT:
        return sizeof(float);
    case PTL_DOUBLE:
        return sizeof(double);
    case PTL_LONG_DOUBLE:
        return sizeof(long double);
    case PTL_FLOAT_COMPLEX:
        return sizeof(float _Complex);
    case PTL_DOUBLE_COMPLEX:
        return sizeof(double _Complex);
    case PTL_LONG_DOUBLE_COMPLEX:
        return sizeof(long double _Complex);
    default:
        ptl_panic("ptl_atsize: unknown type\n");
        return 0; /* Suppress compilation warning */
    }
}

int random_int(int start, int end)
{
    static random_device dev;
    static mt19937 rng(dev());
    uniform_int_distribution<mt19937::result_type> dist(start, end); // Distribution in range [start, end]

    return dist(rng);
}

const char* msg_type_c_str(BxiMsg* msg)
{
    switch (msg->type) {
    case S4BXI_E2E_ACK:
        return "S4BXI_E2E_ACK";

    case S4BXI_PTL_ACK:
        return "S4BXI_PTL_ACK";

    case S4BXI_PTL_GET_RESPONSE:
        return "S4BXI_PTL_GET_RESPONSE";

    case S4BXI_PTL_PUT:
        return "S4BXI_PTL_PUT";

    case S4BXI_PTL_GET:
        return "S4BXI_PTL_GET";

    case S4BXI_PTL_ATOMIC:
        return "S4BXI_PTL_ATOMIC";

    case S4BXI_PTL_FETCH_ATOMIC:
        return "S4BXI_PTL_FETCH_ATOMIC";

    case S4BXI_PTL_FETCH_ATOMIC_RESPONSE:
        return "S4BXI_PTL_FETCH_ATOMIC_RESPONSE";

    default:
        return "UNKNOWN";
    }
}
