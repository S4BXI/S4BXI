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

#include "s4bxi/BxiEngine.hpp"
#include "s4bxi/actors/BxiMainActor.hpp"
#include "portals4.h"
#include "s4bxi/s4bxi_util.hpp"
#include "s4bxi/s4bxi_bench.h"

#define INSTANT_PORTALS_CALL(call)                                                                                     \
    do {                                                                                                               \
        auto main_actor = GET_CURRENT_MAIN_ACTOR;                                                                      \
        auto res        = main_actor->call;                                                                            \
        return res;                                                                                                    \
    } while (0)

#define BENCH_PORTALS_CALL(call)                                                                                       \
    do {                                                                                                               \
        auto main_actor = GET_CURRENT_MAIN_ACTOR;                                                                      \
        s4bxi_bench_end();                                                                                             \
        auto res = main_actor->call;                                                                                   \
        s4bxi_bench_begin();                                                                                           \
        return res;                                                                                                    \
    } while (0)

// Some Portals calls might not yield to SimGrid, in this case don't stop benchmarking
#define MAYBE_BENCH_PORTALS_CALL(call)                                                                                 \
    do {                                                                                                               \
        int res;                                                                                                       \
        auto main_actor = GET_CURRENT_MAIN_ACTOR;                                                                      \
        if (S4BXI_CONFIG_AND(main_actor->getNode(), model_pci_commands)) {                                             \
            s4bxi_bench_end();                                                                                         \
            res = (main_actor->call);                                                                                  \
            s4bxi_bench_begin();                                                                                       \
        } else {                                                                                                       \
            res = call;                                                                                                \
        }                                                                                                              \
        return res;                                                                                                    \
    } while (0)

int PtlInit(void)
{
    BENCH_PORTALS_CALL(PtlInit());
}

void PtlFini(void)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;
    s4bxi_bench_end();
    main_actor->PtlFini();
    s4bxi_bench_begin();
}

int PtlSetMemOps(struct ptl_mem_ops* ops)
{
    THROW_UNIMPLEMENTED;
}

int PtlActivateHookAdd(void (*cb)(void*, unsigned int, int), void* arg, struct ptl_activate_hook** rh)
{
    THROW_UNIMPLEMENTED;
}

int PtlActivateHookRemove(struct ptl_activate_hook* h)
{
    THROW_UNIMPLEMENTED;
}

int PtlNIInit(ptl_interface_t nic, unsigned int vni, ptl_pid_t pid, const struct ptl_ni_limits* desired,
              struct ptl_ni_limits* actual, ptl_handle_ni_t* retnih)
{
    MAYBE_BENCH_PORTALS_CALL(PtlNIInit(nic, vni, pid, desired, actual, retnih));
}

int PtlNIFini(ptl_handle_ni_t nih)
{
    BENCH_PORTALS_CALL(PtlNIFini(nih));
}

int PtlNIHandle(ptl_handle_any_t hdl, ptl_handle_ni_t* ret)
{
    THROW_UNIMPLEMENTED;
}

int PtlHandleIsEqual(ptl_handle_any_t handle1, ptl_handle_any_t handle2)
{
    INSTANT_PORTALS_CALL(PtlHandleIsEqual(handle1, handle2));
}

int PtlNIStatus(ptl_handle_ni_t nih, ptl_sr_index_t sr, ptl_sr_value_t* rval)
{
    THROW_UNIMPLEMENTED;
}

int PtlSetMap(ptl_handle_ni_t nih, ptl_size_t size, const union ptl_process* map)
{
    INSTANT_PORTALS_CALL(PtlSetMap(nih, size, map));
}

int PtlGetMap(ptl_handle_ni_t nih, ptl_size_t size, union ptl_process* map, ptl_size_t* retsize)
{
    INSTANT_PORTALS_CALL(PtlGetMap(nih, size, map, retsize));
}

int PtlPTAlloc(ptl_handle_ni_t nih, unsigned int options, ptl_handle_eq_t eqh, ptl_index_t pte, ptl_index_t* ret)
{
    MAYBE_BENCH_PORTALS_CALL(PtlPTAlloc(nih, options, eqh, pte, ret));
}

int PtlPTFree(ptl_handle_ni_t nih, ptl_pt_index_t pth)
{
    MAYBE_BENCH_PORTALS_CALL(PtlPTFree(nih, pth));
}

int PtlPTEnable(ptl_handle_ni_t nih, ptl_pt_index_t pth)
{
    MAYBE_BENCH_PORTALS_CALL(PtlPTEnable(nih, pth));
}

int PtlPTEnableNB(ptl_handle_ni_t nih, ptl_pt_index_t pth)
{
    MAYBE_BENCH_PORTALS_CALL(PtlPTEnableNB(nih, pth));
}

int PtlPTDisable(ptl_handle_ni_t nih, ptl_pt_index_t pth)
{
    MAYBE_BENCH_PORTALS_CALL(PtlPTDisable(nih, pth));
}

int PtlPTDisableNB(ptl_handle_ni_t nih, ptl_pt_index_t pth)
{
    MAYBE_BENCH_PORTALS_CALL(PtlPTDisableNB(nih, pth));
}

int PtlGetUid(ptl_handle_ni_t nih, ptl_uid_t* uid)
{
    INSTANT_PORTALS_CALL(PtlGetUid(nih, uid));
}

int PtlGetId(ptl_handle_ni_t nih, union ptl_process* pid)
{
    INSTANT_PORTALS_CALL(PtlGetId(nih, pid));
}

int PtlGetPhysId(ptl_handle_ni_t nih, union ptl_process* pid)
{
    INSTANT_PORTALS_CALL(PtlGetPhysId(nih, pid));
}

int PtlGetHwid(ptl_handle_ni_t nih, uint64_t* hwid, uint64_t* capabilities)
{
    THROW_UNIMPLEMENTED;
}

int PtlMDBind(ptl_handle_ni_t nih, const struct ptl_md* mdpar, ptl_handle_md_t* ret)
{
    MAYBE_BENCH_PORTALS_CALL(PtlMDBind(nih, mdpar, ret));
}

int PtlMDBindNB(ptl_handle_ni_t nih, const struct ptl_md* mdpar, ptl_handle_md_t* ret)
{
    THROW_UNIMPLEMENTED;
}

int PtlMDRelease(ptl_handle_md_t mdh)
{
    MAYBE_BENCH_PORTALS_CALL(PtlMDRelease(mdh));
}

int PtlMEAppend(ptl_handle_ni_t nih, ptl_pt_index_t pte, const struct ptl_me* me, int list, void* arg,
                ptl_handle_me_t* mehret)
{
    BENCH_PORTALS_CALL(PtlMEAppend(nih, pte, me, list, arg, mehret));
}

int PtlMEAppendNB(ptl_handle_ni_t nih, ptl_pt_index_t pte, const struct ptl_me* me, int list, void* arg,
                  ptl_handle_me_t* mehret)
{
    THROW_UNIMPLEMENTED;
}

int PtlMEUnlink(ptl_handle_me_t meh)
{
    MAYBE_BENCH_PORTALS_CALL(PtlMEUnlink(meh));
}

int PtlMESearch(ptl_handle_ni_t nih, ptl_pt_index_t pte, const ptl_me_t* me, ptl_search_op_t op, void* arg)
{
    THROW_UNIMPLEMENTED;
}

int PtlMESearchNB(ptl_handle_ni_t nih, ptl_pt_index_t pte, const ptl_me_t* me, ptl_search_op_t op, void* arg)
{
    THROW_UNIMPLEMENTED;
}

int PtlLEAppend(ptl_handle_ni_t nih, ptl_pt_index_t pte, const struct ptl_me* me, int list, void* arg,
                ptl_handle_me_t* mehret)
{
    BENCH_PORTALS_CALL(PtlLEAppend(nih, pte, me, list, arg, mehret));
}

int PtlLEAppendNB(ptl_handle_ni_t nih, ptl_pt_index_t pte, const struct ptl_me* me, int list, void* arg,
                  ptl_handle_me_t* mehret)
{
    THROW_UNIMPLEMENTED;
}

int PtlLEUnlink(ptl_handle_me_t meh)
{
    MAYBE_BENCH_PORTALS_CALL(PtlLEUnlink(meh));
}

int PtlLESearch(ptl_handle_ni_t nih, ptl_pt_index_t pte, const ptl_me_t* me, ptl_search_op_t op, void* arg)
{
    THROW_UNIMPLEMENTED;
}

int PtlLESearchNB(ptl_handle_ni_t nih, ptl_pt_index_t pte, const ptl_me_t* me, ptl_search_op_t op, void* arg)
{
    THROW_UNIMPLEMENTED;
}

int PtlEQAllocAsync(ptl_handle_ni_t nih, ptl_size_t size, ptl_handle_eq_t* reteqh, void (*cb)(void*, ptl_handle_eq_t),
                    void* arg, int hint)
{
    THROW_UNIMPLEMENTED;
}

int PtlEQAlloc(ptl_handle_ni_t nih, ptl_size_t count, ptl_handle_eq_t* reteqh)
{
    // Allocating an EQ can yield because creating an EQ might require
    // creating a mailbox, which triggers a simcall
    BENCH_PORTALS_CALL(PtlEQAlloc(nih, count, reteqh));
}

int PtlEQFree(ptl_handle_eq_t eqh)
{
    MAYBE_BENCH_PORTALS_CALL(PtlEQFree(eqh));
}

int PtlEQGet(ptl_handle_eq_t eqh, struct ptl_event* rev)
{
    BENCH_PORTALS_CALL(PtlEQGet(eqh, rev));
}

int PtlEQPoll(const ptl_handle_eq_t* eqhlist, unsigned int size, ptl_time_t timeout, struct ptl_event* rev,
              unsigned int* rwhich)
{
    BENCH_PORTALS_CALL(PtlEQPoll(eqhlist, size, timeout, rev, rwhich));
}

int PtlEQWait(ptl_handle_eq_t eqh, struct ptl_event* rev)
{
    BENCH_PORTALS_CALL(PtlEQWait(eqh, rev));
}

int PtlCTAlloc(ptl_handle_ni_t nih, ptl_handle_ct_t* retcth)
{
    MAYBE_BENCH_PORTALS_CALL(PtlCTAlloc(nih, retcth));
}

int PtlCTFree(ptl_handle_ct_t cth)
{
    MAYBE_BENCH_PORTALS_CALL(PtlCTFree(cth));
}

int PtlCTGet(ptl_handle_ct_t cth, struct ptl_ct_event* rev)
{
    INSTANT_PORTALS_CALL(PtlCTGet(cth, rev));
}

int PtlCTPoll(const ptl_handle_ct_t* cthlist, const ptl_size_t* test, unsigned int size, ptl_time_t timeout,
              struct ptl_ct_event* rev, unsigned int* rwhich)
{
    BENCH_PORTALS_CALL(PtlCTPoll(cthlist, test, size, timeout, rev, rwhich));
}

int PtlCTWait(ptl_handle_ct_t cth, ptl_size_t test, struct ptl_ct_event* rev)
{
    BENCH_PORTALS_CALL(PtlCTWait(cth, test, rev));
}

int PtlCTSet(ptl_handle_ct_t cth, struct ptl_ct_event arg)
{
    BENCH_PORTALS_CALL(PtlCTSet(cth, arg));
}

int PtlCTInc(ptl_handle_ct_t cth, struct ptl_ct_event arg)
{
    BENCH_PORTALS_CALL(PtlCTInc(cth, arg));
}

int PtlCTCancelTriggered(ptl_handle_ct_t cth)
{
    THROW_UNIMPLEMENTED;
}

int PtlPut(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, int ack, union ptl_process rank, ptl_pt_index_t pte,
           ptl_match_bits_t bits, ptl_size_t roffs, void* arg, ptl_hdr_data_t hdr)
{
    BENCH_PORTALS_CALL(PtlPut(mdh, loffs, len, ack, rank, pte, bits, roffs, arg, hdr));
}

int PtlPutNB(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, int ack, union ptl_process rank, ptl_pt_index_t pte,
             ptl_match_bits_t bits, ptl_size_t roffs, void* arg, ptl_hdr_data_t hdr)
{
    BENCH_PORTALS_CALL(PtlPutNB(mdh, loffs, len, ack, rank, pte, bits, roffs, arg, hdr));
}

int PtlGet(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, union ptl_process rank, ptl_pt_index_t pte,
           ptl_match_bits_t bits, ptl_size_t roffs, void* arg)
{
    BENCH_PORTALS_CALL(PtlGet(mdh, loffs, len, rank, pte, bits, roffs, arg));
}

int PtlGetNB(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, union ptl_process rank, ptl_pt_index_t pte,
             ptl_match_bits_t bits, ptl_size_t roffs, void* arg)
{
    BENCH_PORTALS_CALL(PtlGetNB(mdh, loffs, len, rank, pte, bits, roffs, arg));
}

int PtlAtomic(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, ptl_ack_req_t ack, ptl_process_t rank,
              ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* uptr, ptl_hdr_data_t hdr, ptl_op_t aop,
              ptl_datatype_t atype)
{
    BENCH_PORTALS_CALL(PtlAtomic(mdh, loffs, len, ack, rank, pte, bits, roffs, uptr, hdr, aop, atype));
}

int PtlAtomicNB(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, ptl_ack_req_t ack, ptl_process_t rank,
                ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* uptr, ptl_hdr_data_t hdr,
                ptl_op_t aop, ptl_datatype_t atype)
{
    BENCH_PORTALS_CALL(PtlAtomicNB(mdh, loffs, len, ack, rank, pte, bits, roffs, uptr, hdr, aop, atype));
}

int PtlFetchAtomic(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
                   ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs,
                   void* uptr, ptl_hdr_data_t hdr, ptl_op_t aop, ptl_datatype_t atype)
{
    BENCH_PORTALS_CALL(
        PtlFetchAtomic(get_mdh, get_loffs, put_mdh, put_loffs, len, rank, pte, bits, roffs, uptr, hdr, aop, atype));
}

int PtlFetchAtomicNB(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
                     ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs,
                     void* uptr, ptl_hdr_data_t hdr, ptl_op_t aop, ptl_datatype_t atype)
{
    BENCH_PORTALS_CALL(
        PtlFetchAtomicNB(get_mdh, get_loffs, put_mdh, put_loffs, len, rank, pte, bits, roffs, uptr, hdr, aop, atype));
}

int PtlSwap(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
            ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* uptr,
            ptl_hdr_data_t hdr, const void* cst, ptl_op_t aop, ptl_datatype_t atype)
{
    BENCH_PORTALS_CALL(
        PtlSwap(get_mdh, get_loffs, put_mdh, put_loffs, len, rank, pte, bits, roffs, uptr, hdr, cst, aop, atype));
}

int PtlSwapNB(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
              ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs,
              void* uptr, ptl_hdr_data_t hdr, const void* cst, ptl_op_t aop, ptl_datatype_t atype)
{
    BENCH_PORTALS_CALL(
        PtlSwapNB(get_mdh, get_loffs, put_mdh, put_loffs, len, rank, pte, bits, roffs, uptr, hdr, cst, aop, atype));
}

int PtlTriggeredPut(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, int ack, union ptl_process rank,
                    ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* arg, ptl_hdr_data_t hdr,
                    ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredPutNB(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, int ack, union ptl_process rank,
                      ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* arg, ptl_hdr_data_t hdr,
                      ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredGet(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, union ptl_process rank, ptl_pt_index_t pte,
                    ptl_match_bits_t bits, ptl_size_t roffs, void* arg, ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredGetNB(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, union ptl_process rank, ptl_pt_index_t pte,
                      ptl_match_bits_t bits, ptl_size_t roffs, void* arg, ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredAtomic(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, ptl_ack_req_t ack, ptl_process_t rank,
                       ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* uptr, ptl_hdr_data_t hdr,
                       ptl_op_t aop, ptl_datatype_t atype, ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredAtomicNB(ptl_handle_md_t mdh, ptl_size_t loffs, ptl_size_t len, ptl_ack_req_t ack, ptl_process_t rank,
                         ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs, void* uptr, ptl_hdr_data_t hdr,
                         ptl_op_t aop, ptl_datatype_t atype, ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredFetchAtomic(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh,
                            ptl_size_t put_loffs, ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte,
                            ptl_match_bits_t bits, ptl_size_t roffs, void* uptr, ptl_hdr_data_t hdr, ptl_op_t aop,
                            ptl_datatype_t atype, ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredFetchAtomicNB(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh,
                              ptl_size_t put_loffs, ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte,
                              ptl_match_bits_t bits, ptl_size_t roffs, void* uptr, ptl_hdr_data_t hdr, ptl_op_t aop,
                              ptl_datatype_t atype, ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredSwap(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
                     ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs,
                     void* uptr, ptl_hdr_data_t hdr, const void* cst, ptl_op_t aop, ptl_datatype_t atype,
                     ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredSwapNB(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
                       ptl_size_t len, ptl_process_t rank, ptl_pt_index_t pte, ptl_match_bits_t bits, ptl_size_t roffs,
                       void* uptr, ptl_hdr_data_t hdr, const void* cst, ptl_op_t aop, ptl_datatype_t atype,
                       ptl_handle_ct_t cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredCTSet(ptl_handle_ct_t cth, struct ptl_ct_event newval, ptl_handle_ct_t trig_cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredCTSetNB(ptl_handle_ct_t cth, struct ptl_ct_event newval, ptl_handle_ct_t trig_cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredCTInc(ptl_handle_ct_t cth, struct ptl_ct_event delta, ptl_handle_ct_t trig_cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

int PtlTriggeredCTIncNB(ptl_handle_ct_t cth, struct ptl_ct_event delta, ptl_handle_ct_t trig_cth, ptl_size_t thres)
{
    THROW_UNIMPLEMENTED;
}

/**
 * For now I just need this to exist. Not sure if
 * we'll ever need to do anything actually because
 * there is not real concurrency in monothread
 * simulation
 */
int PtlAtomicSync(void)
{
    return PTL_OK;
}

int PtlNIAtomicSync(ptl_handle_ni_t nih)
{
    THROW_UNIMPLEMENTED;
}

int PtlNFDs(ptl_handle_ni_t nih)
{
    THROW_UNIMPLEMENTED;
}

int PtlPollFDs(ptl_handle_ni_t nih, struct pollfd* pfds, int events)
{
    THROW_UNIMPLEMENTED;
}

int PtlPollEvents(ptl_handle_ni_t nih, struct pollfd* pfds)
{
    THROW_UNIMPLEMENTED;
}

int PtlStartBundle(ptl_handle_ni_t nih)
{
    THROW_UNIMPLEMENTED;
}

int PtlEndBundle(ptl_handle_ni_t nih)
{
    THROW_UNIMPLEMENTED;
}
