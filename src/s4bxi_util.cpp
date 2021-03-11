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

#include "s4bxi/s4bxi_util.hpp"
#include "portals4.h"
#include "s4bxi/s4bxi_xbt_log.h"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_util, "Messages generated in util functions");

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