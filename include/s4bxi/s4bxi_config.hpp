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

#ifndef S4BXI_s4bxi_config_HPP
#define S4BXI_s4bxi_config_HPP

struct s4bxi_config {
    int max_retries;
    double retry_timeout;
    bool model_pci_commands;
    bool e2e_off;
    string log_folder;
    int log_level;
    string privatize_libs;
    bool keep_temps;
    long max_memcpy;
    double cpu_factor;
    double cpu_threshold;
    bool quick_acks;
};

#endif // S4BXI_s4bxi_config_HPP
