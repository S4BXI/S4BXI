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

#ifndef S4BXI_s4bxi_config_HPP
#define S4BXI_s4bxi_config_HPP

/**
 * @brief Global configuration of the simulation
 *
 * Most of these values are hydrated at startup based on environment variables
 */
struct s4bxi_config {
    /** @brief Maximum number of E2E retries before giving up */
    int max_retries;
    /** @brief E2E timeout between retries */
    double retry_timeout;
    /** @brief If disabled, messages' payloads won't actually be copied to save a bit of time */
    bool use_real_memory;
    /** @brief Model PCI transfers (a value of 0 implies model_pci_commands=0) */
    bool model_pci;
    /** @brief Model small PCI transfers when issuing commands to the NIC */
    bool model_pci_commands;
    /** @brief Disable all E2E processing globally */
    bool e2e_off;
    /** @brief Output folder for CSV logging */
    std::string log_folder;
    /** @brief Set to 0 to diable logging (computed based on log_folder) */
    int log_level;
    /** @brief Shared libraries to privatize and re-link to user code at runtime */
    std::string privatize_libs;
    /** @brief Disable deleting temporary libraries created by S4BXI */
    bool keep_temps;
    /** @brief Maximum amount of payload to copy on message reception */
    long max_memcpy;
    /** @brief Factor to apply to benchmarked communications before simulating them */
    double cpu_factor;
    /** @brief Threshold under which computation time is ignored (i.e. benchmarked but not simulated) */
    double cpu_threshold;
    /** @brief Accumulate small CPU operations instead of ignoring them */
    double cpu_accumulate;
    /** @brief Triggers ACK at sender side without issuing an actual ACK message on the network */
    bool quick_acks;
    /** @brief Shared memory threshold */
    double auto_shared_malloc_thresh;
    /** @brief Type of shared malloc (global, local or none) */
    int shared_malloc;
    /** @brief Huge page mount point (used for global shared mem) */
    std::string shared_malloc_hugepage;
    /** @brief Blocksize used for global shared mem */
    unsigned long shared_malloc_blocksize;
    /** @brief Maximum number of messages inflight between two nodes (0 to disable) */
    int max_inflight_to_target;
    /** @brief Maximum number of messages inflight between two processes (0 to disable) */
    int max_inflight_to_process;
    /** @brief Disable dlclosing libs, which is required to use some tools like Valgrind */
    bool no_dlclose;
    /** @brief Use PugiXML instead of SimGrid's parser for deployments */
    bool use_pugixml;
};

#endif // S4BXI_s4bxi_config_HPP
