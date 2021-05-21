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

#include "s4bxi/s4ptl.hpp"
#include "s4bxi/s4bxi_util.hpp"
#include "s4bxi/s4bxi_xbt_log.h"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(bxi_s4ptl_md, "Messages specific to s4ptl MD implementation");

BxiMD::BxiMD(ptl_handle_ni_t ni_handle, const ptl_md_t* md_t) : ni((BxiNI*)ni_handle), md(ptl_md_t(*md_t)) {}

BxiMD::BxiMD(const BxiMD& md) : ni(md.ni), md(ptl_md_t(md.md)) {}

/**
 * We need to pass the byte_count from the message manually,
 * because it could be smaller than the actual MD length
 *
 * @param md
 * @param byte_count Byte count from incoming message
 */
void BxiMD::increment_ct(ptl_size_t byte_count)
{
    auto ct = (BxiCT*)md.ct_handle;
    if (ct == PTL_CT_NONE)
        return;

    auto amount = HAS_PTL_OPTION(&md, PTL_MD_EVENT_CT_BYTES) ? byte_count : 1;
    ct->increment_success(amount);
}