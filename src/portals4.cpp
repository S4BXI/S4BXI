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

#include "s4bxi/BxiEngine.hpp"
#include "s4bxi/actors/BxiMainActor.hpp"
#include "portals4.h"
#include "s4bxi/s4bxi_util.hpp"
#include "s4bxi/s4bxi_bench.hpp"

int PtlInit(void)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlInit();
    s4bxi_bench_begin(main_actor);
    return res;
}

void PtlFini(void)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    main_actor->PtlFini();
    s4bxi_bench_begin(main_actor);
}

int PtlSetMemOps(struct ptl_mem_ops* ops)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlActivateHookAdd(void (*cb)(void*, unsigned int, int), void* arg, struct ptl_activate_hook** rh)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlActivateHookRemove(struct ptl_activate_hook* h)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlNIInit(ptl_interface_t nic, unsigned int vni, ptl_pid_t pid, const struct ptl_ni_limits* desired,
              struct ptl_ni_limits* actual, ptl_handle_ni_t* retnih)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlNIInit(nic, vni, pid, desired, actual, retnih);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlNIFini(ptl_handle_ni_t nih)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlNIFini(nih);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlNIHandle(ptl_handle_any_t hdl, ptl_handle_ni_t* ret)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlHandleIsEqual(ptl_handle_any_t handle1, ptl_handle_any_t handle2)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlHandleIsEqual(handle1, handle2);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlNIStatus(ptl_handle_ni_t nih, ptl_sr_index_t sr, ptl_sr_value_t* rval)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlSetMap(ptl_handle_ni_t nih, ptl_size_t size, const union ptl_process* map)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlSetMap(nih, size, map);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlGetMap(ptl_handle_ni_t nih, ptl_size_t size, union ptl_process* map, ptl_size_t* retsize)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlPTAlloc(ptl_handle_ni_t nih, unsigned int options, ptl_handle_eq_t eqh, ptl_index_t pte, ptl_index_t* ret)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlPTAlloc(nih, options, eqh, pte, ret);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlPTFree(ptl_handle_ni_t nih, ptl_pt_index_t pth)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlPTFree(nih, pth);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlPTEnable(ptl_handle_ni_t nih, ptl_pt_index_t pth)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlPTEnable(nih, pth);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlPTEnableNB(ptl_handle_ni_t nih, ptl_pt_index_t pth)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlPTEnableNB(nih, pth);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlPTDisable(ptl_handle_ni_t nih, ptl_pt_index_t pth)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlPTDisable(nih, pth);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlPTDisableNB(ptl_handle_ni_t nih, ptl_pt_index_t pth)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlPTDisableNB(nih, pth);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlGetUid(ptl_handle_ni_t nih, ptl_uid_t* uid)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlGetUid(nih, uid);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlGetId(ptl_handle_ni_t nih, union ptl_process* pid)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlGetId(nih, pid);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlGetPhysId(ptl_handle_ni_t nih, union ptl_process* pid)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlGetPhysId(nih, pid);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlGetHwid(ptl_handle_ni_t nih, uint64_t* hwid, uint64_t* capabilities)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlMDBind(ptl_handle_ni_t nih, const struct ptl_md* mdpar, ptl_handle_md_t* ret)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlMDBind(nih, mdpar, ret);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlMDBindNB(ptl_handle_ni_t nih, const struct ptl_md* mdpar, ptl_handle_md_t* ret)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlMDRelease(ptl_handle_md_t mdh)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlMDRelease(mdh);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlMEAppend(ptl_handle_ni_t nih, ptl_pt_index_t pte, const struct ptl_me* me, int list, void* arg,
                ptl_handle_me_t* mehret)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlMEAppend(nih, pte, me, list, arg, mehret);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlMEAppendNB(ptl_handle_ni_t nih, ptl_pt_index_t pte, const struct ptl_me* me, int list, void* arg,
                  ptl_handle_me_t* mehret)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlMEUnlink(ptl_handle_me_t meh)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlMEUnlink(meh);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlMESearch(ptl_handle_ni_t nih, ptl_pt_index_t pte, const ptl_me_t* me, ptl_search_op_t op, void* arg)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlMESearchNB(ptl_handle_ni_t nih, ptl_pt_index_t pte, const ptl_me_t* me, ptl_search_op_t op, void* arg)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlLEAppend(ptl_handle_ni_t nih, ptl_pt_index_t pte, const struct ptl_me* me, int list, void* arg,
                ptl_handle_me_t* mehret)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlLEAppend(nih, pte, me, list, arg, mehret);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlLEAppendNB(ptl_handle_ni_t nih, ptl_pt_index_t pte, const struct ptl_me* me, int list, void* arg,
                  ptl_handle_me_t* mehret)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlLEUnlink(ptl_handle_me_t meh)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlLEUnlink(meh);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlLESearch(ptl_handle_ni_t nih, ptl_pt_index_t pte, const ptl_me_t* me, ptl_search_op_t op, void* arg)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlLESearchNB(ptl_handle_ni_t nih, ptl_pt_index_t pte, const ptl_me_t* me, ptl_search_op_t op, void* arg)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlEQAllocAsync(ptl_handle_ni_t nih, ptl_size_t size, ptl_handle_eq_t* reteqh, void (*cb)(void*, ptl_handle_eq_t),
                    void* arg, int hint)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlEQAlloc(ptl_handle_ni_t nih, ptl_size_t count, ptl_handle_eq_t* reteqh)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlEQAlloc(nih, count, reteqh);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlEQFree(ptl_handle_eq_t eqh)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlEQFree(eqh);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlEQGet(ptl_handle_eq_t eqh, struct ptl_event* rev)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlEQGet(eqh, rev);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlEQPoll(const ptl_handle_eq_t* eqhlist, unsigned int size, ptl_time_t timeout, struct ptl_event* rev,
              unsigned int* rwhich)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlEQPoll(eqhlist, size, timeout, rev, rwhich);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlEQWait(ptl_handle_eq_t eqh, struct ptl_event* rev)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlEQWait(eqh, rev);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlCTAlloc(ptl_handle_ni_t nih, ptl_handle_ct_t* retcth)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlCTAlloc(nih, retcth);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlCTFree(ptl_handle_ct_t cth)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlCTFree(cth);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlCTGet(ptl_handle_ct_t cth, struct ptl_ct_event* rev)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlCTGet(cth, rev);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlCTPoll(const ptl_handle_ct_t* cthlist, const ptl_size_t* test, unsigned int size, ptl_time_t timeout,
              struct ptl_ct_event* rev, unsigned int* rwhich)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlCTPoll(cthlist, test, size, timeout, rev, rwhich);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlCTWait(ptl_handle_ct_t cth, ptl_size_t test, struct ptl_ct_event* rev)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlCTWait(cth, test, rev);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlCTSet(ptl_handle_ct_t cth, struct ptl_ct_event arg)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlCTSet(cth, arg);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlCTInc(ptl_handle_ct_t cth, struct ptl_ct_event arg)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlCTInc(cth, arg);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlCTCancelTriggered(ptl_handle_ct_t cth)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlPut(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, int ack, union ptl_process rank, ptl_pt_index_t pte,
           ptl_match_bits_t bits, ptl_size_t roffs, void* arg, ptl_hdr_data_t hdr)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlPut(mdh, loffs, len, ack, rank, pte, bits, roffs, arg, hdr);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlPutNB(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, int ack, union ptl_process rank, ptl_pt_index_t pte,
             ptl_match_bits_t bits, ptl_size_t roffs, void* arg, ptl_hdr_data_t hdr)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlPutNB(mdh, loffs, len, ack, rank, pte, bits, roffs, arg, hdr);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlGet(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, union ptl_process rank, ptl_pt_index_t pte,
           ptl_match_bits_t bits, ptl_size_t roffs, void* arg)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlGet(mdh, loffs, len, rank, pte, bits, roffs, arg);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlGetNB(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, union ptl_process rank, ptl_pt_index_t pte,
             ptl_match_bits_t bits, ptl_size_t roffs, void* arg)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlGetNB(mdh, loffs, len, rank, pte, bits, roffs, arg);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlAtomic(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, ptl_ack_req_t ack, ptl_process_t rank,
              ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* uptr, ptl_hdr_data_t hdr, ptl_op_t aop,
              ptl_datatype_t atype)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlAtomic(mdh, loffs, len, ack, rank, pte, bits, roffs, uptr, hdr, aop, atype);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlAtomicNB(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, ptl_ack_req_t ack, ptl_process_t rank,
                ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* uptr, ptl_hdr_data_t hdr,
                ptl_op_t aop, ptl_datatype_t atype)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end(main_actor);
    auto res = main_actor->PtlAtomicNB(mdh, loffs, len, ack, rank, pte, bits, roffs, uptr, hdr, aop, atype);
    s4bxi_bench_begin(main_actor);
    return res;
}

int PtlFetchAtomic(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
                   ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs,
                   void* uptr, ptl_hdr_data_t hdr, ptl_op_t aop, ptl_datatype_t atype)
{
    return GET_CURRENT_MAIN_ACTOR->PtlFetchAtomic(get_mdh, get_loffs, put_mdh, put_loffs, len, rank, pte, bits, roffs,
                                                  uptr, hdr, aop, atype);
}

int PtlFetchAtomicNB(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
                     ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs,
                     void* uptr, ptl_hdr_data_t hdr, ptl_op_t aop, ptl_datatype_t atype)
{
    return GET_CURRENT_MAIN_ACTOR->PtlFetchAtomicNB(get_mdh, get_loffs, put_mdh, put_loffs, len, rank, pte, bits, roffs,
                                                    uptr, hdr, aop, atype);
}

int PtlSwap(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
            ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* uptr,
            ptl_hdr_data_t hdr, const void* cst, ptl_op_t aop, ptl_datatype_t atype)
{
    return GET_CURRENT_MAIN_ACTOR->PtlSwap(get_mdh, get_loffs, put_mdh, put_loffs, len, rank, pte, bits, roffs,
                                           uptr, hdr, cst, aop, atype);
}

int PtlSwapNB(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
              ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs,
              void* uptr, ptl_hdr_data_t hdr, const void* cst, ptl_op_t aop, ptl_datatype_t atype)
{
    return GET_CURRENT_MAIN_ACTOR->PtlSwapNB(get_mdh, get_loffs, put_mdh, put_loffs, len, rank, pte, bits, roffs,
                                             uptr, hdr, cst, aop, atype);
}

int PtlTriggeredPut(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, int ack, union ptl_process rank,
                    ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* arg, ptl_hdr_data_t hdr,
                    ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredPutNB(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, int ack, union ptl_process rank,
                      ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* arg, ptl_hdr_data_t hdr,
                      ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredGet(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, union ptl_process rank, ptl_pt_index_t pte,
                    ptl_match_bits_t bits, ptl_size_t roffs, void* arg, ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredGetNB(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, union ptl_process rank, ptl_pt_index_t pte,
                      ptl_match_bits_t bits, ptl_size_t roffs, void* arg, ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredAtomic(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, ptl_ack_req_t ack, ptl_process_t rank,
                       ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* uptr, ptl_hdr_data_t hdr,
                       ptl_op_t aop, ptl_datatype_t atype, ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredAtomicNB(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, ptl_ack_req_t ack, ptl_process_t rank,
                         ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* uptr, ptl_hdr_data_t hdr,
                         ptl_op_t aop, ptl_datatype_t atype, ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredFetchAtomic(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh,
                            ptl_size_t put_loffs, ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte,
                            ptl_match_bits_t bits, ptl_size_t roffs, void* uptr, ptl_hdr_data_t hdr, ptl_op_t aop,
                            ptl_datatype_t atype, ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredFetchAtomicNB(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh,
                              ptl_size_t put_loffs, ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte,
                              ptl_match_bits_t bits, ptl_size_t roffs, void* uptr, ptl_hdr_data_t hdr, ptl_op_t aop,
                              ptl_datatype_t atype, ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredSwap(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
                     ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs,
                     void* uptr, ptl_hdr_data_t hdr, const void* cst, ptl_op_t aop, ptl_datatype_t atype,
                     ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredSwapNB(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
                       ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs,
                       void* uptr, ptl_hdr_data_t hdr, const void* cst, ptl_op_t aop, ptl_datatype_t atype,
                       ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredCTSet(ptl_handle_ct_t cth, struct ptl_ct_event newval, ptl_handle_ct_t trig_cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredCTSetNB(ptl_handle_ct_t cth, struct ptl_ct_event newval, ptl_handle_ct_t trig_cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredCTInc(ptl_handle_ct_t cth, struct ptl_ct_event delta, ptl_handle_ct_t trig_cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlTriggeredCTIncNB(ptl_handle_ct_t cth, struct ptl_ct_event delta, ptl_handle_ct_t trig_cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlAtomicSync(void)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlNIAtomicSync(ptl_handle_ni_t nih)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlNFDs(ptl_handle_ni_t nih)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlPollFDs(ptl_handle_ni_t nih, struct pollfd* pfds, int events)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlPollEvents(ptl_handle_ni_t nih, struct pollfd* pfds)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlStartBundle(ptl_handle_ni_t nih)
{
    THROW_UNIMPLEMENTED;
    return 1;
}

int PtlEndBundle(ptl_handle_ni_t nih)
{
    THROW_UNIMPLEMENTED;
    return 1;
}
