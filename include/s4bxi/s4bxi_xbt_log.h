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

#ifndef S4BXI_XBT_LOG_H
#define S4BXI_XBT_LOG_H

#include <xbt/log.h>

#ifdef __cplusplus
extern "C" {
#endif

// Copy-paste of SimGrid's log.h, so that I can find this info more easily again
// xbt_log_priority_trace = 1,          /**< enter and return of some functions */
// xbt_log_priority_debug = 2,          /**< crufty output  */
// xbt_log_priority_verbose = 3,        /**< verbose output for the user wanting more */
// xbt_log_priority_info = 4,           /**< output about the regular functioning */
// xbt_log_priority_warning = 5,        /**< minor issue encountered */
// xbt_log_priority_error = 6,          /**< issue encountered */
// xbt_log_priority_critical = 7,       /**< major issue encountered */

#ifdef MAKE_NEW_LOG_CATEGORY
XBT_LOG_NEW_CATEGORY(s4bxi, "Messages relative to S4BXI");
#else
XBT_LOG_EXTERNAL_CATEGORY(s4bxi);
#endif

#define S4BXI_LOG_NEW_DEFAULT_CATEGORY(cname, desc) XBT_LOG_NEW_DEFAULT_SUBCATEGORY(cname, s4bxi, desc);
#define S4BXI_LOG_NEW_CATEGORY(cname, desc)         XBT_LOG_NEW_SUBCATEGORY(cname, s4bxi, desc);
#define S4BXI_LOG_ISENABLED(priority)               XBT_LOG_ISENABLED(s4bxi, priority)

#ifdef __cplusplus
}
#endif

#endif // S4BXI_XBT_LOG_H