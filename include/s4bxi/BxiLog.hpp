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

#ifndef S4BXI_BxiLog_HPP
#define S4BXI_BxiLog_HPP

#include <iostream>
#include <iomanip> // setprecision

#include "s4ptl.hpp"

using namespace std;

enum bxi_log_type {
    // Same as bxi_msg_type, it's important
    S4BXILOG_E2E_ACK,
    S4BXILOG_PTL_ACK,
    S4BXILOG_PTL_RESPONSE,
    S4BXILOG_PTL_PUT,
    S4BXILOG_PTL_GET,
    S4BXILOG_PTL_ATOMIC,
    S4BXILOG_PTL_FETCH_ATOMIC,
    S4BXILOG_PTL_FETCH_ATOMIC_RESPONSE,

    // PCI ones
    S4BXILOG_PCI_PIO_PAYLOAD,
    S4BXILOG_PCI_DMA_PAYLOAD,
    S4BXILOG_PCI_DMA_REQUEST,
    S4BXILOG_PCI_PAYLOAD_WRITE,
    S4BXILOG_PCI_EVENT,
    S4BXILOG_PCI_COMMAND,

    // Compute ones
    S4BXILOG_COMPUTE,
};

class BxiLog {
  public:
    double start;
    double end;
    bxi_log_type type;
    ptl_nid_t initiator;
    ptl_nid_t target;

    friend ostream& operator<<(ostream& os, BxiLog const& log)
    {
        int precision    = 9;
        unsigned int tmp = (unsigned int)log.start;
        while (tmp >= 1) {
            tmp /= 10;
            ++precision;
        }

        os << setprecision(precision) << log.type << ',' << log.initiator << ',' << log.target << ',' << log.start
           << ',' << log.end;

        return os;
    }
};

#endif // S4BXI_BxiLog_HPP