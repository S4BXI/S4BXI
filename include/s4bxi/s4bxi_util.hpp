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

#ifndef S4BXI_S4BXI_UTIL_HPP
#define S4BXI_S4BXI_UTIL_HPP

#include <xbt.h>
#include <climits>

#include "BxiEngine.hpp"
#include "BxiLog.hpp"
#include "s4bxi/plugins/BxiActorExt.hpp"

#define GET_CURRENT_MAIN_ACTOR get_cached_current_main_actor()

#define PCI_CPU_TO_NIC true
#define PCI_NIC_TO_CPU false

#define CQ_INITIAL_CAPACITY 16
#define MAX_E2E_ENTRIES     8192

#define TRUTHY_CHAR(prop) (strcmp((prop), "TRUE") == 0 || strcmp((prop), "true") == 0 || strcmp((prop), "1") == 0)

#define HAS_PTL_OPTION(ptl_object, flag) (((ptl_object)->options & (flag)) != 0)

#define S4BXI_GLOBAL_CONFIG(name)    (BxiEngine::get_config()->name)
#define S4BXI_CONFIG_AND(node, name) (node->name && S4BXI_GLOBAL_CONFIG(name))
#define S4BXI_CONFIG_OR(node, name)  (node->name || S4BXI_GLOBAL_CONFIG(name))

// int __bxi_log_level = S4BXI_GLOBAL_CONFIG(log_level) && (log_type == S4BXILOG_PTL_PUT || log_type ==
// S4BXILOG_PTL_GET_RESPONSE)
#define S4BXI_STARTLOG(log_type, log_initiator, log_target)                                                            \
    BxiLog __bxi_log;                                                                                                  \
    int __bxi_log_level = S4BXI_GLOBAL_CONFIG(log_level);                                                              \
    if (__bxi_log_level) {                                                                                             \
        __bxi_log.start     = simgrid::s4u::Engine::get_clock();                                                       \
        __bxi_log.type      = log_type;                                                                                \
        __bxi_log.initiator = log_initiator;                                                                           \
        __bxi_log.target    = log_target;                                                                              \
    }

#define S4BXI_WRITELOG()                                                                                               \
    if (__bxi_log_level) {                                                                                             \
        __bxi_log.end = simgrid::s4u::Engine::get_clock();                                                             \
        BxiEngine::get_instance()->log(__bxi_log);                                                                     \
    }

#define ptl_panic(msg)                                                                                                 \
    do {                                                                                                               \
        if (msg)                                                                                                       \
            XBT_ERROR("%s", msg);                                                                                      \
        xbt_backtrace_display_current();                                                                               \
        abort();                                                                                                       \
    } while (0)

#define ptl_panic_fmt(fmt, ...)                                                                                        \
    do {                                                                                                               \
        XBT_ERROR(fmt, __VA_ARGS__);                                                                                   \
        xbt_backtrace_display_current();                                                                               \
        abort();                                                                                                       \
    } while (0)

int ptl_atsize(enum ptl_datatype atype);

int random_int(int start = 0, int end = INT_MAX);

const char* msg_type_c_str(BxiMsg* msg);

#endif // S4BXI_S4BXI_UTIL_HPP
