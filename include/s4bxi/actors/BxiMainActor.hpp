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

#ifndef S4BXI_BXIMAINACTOR_HPP
#define S4BXI_BXIMAINACTOR_HPP

#include <string>
#include <cmath>
#include <map>
#include <vector>
#include <xbt.h>
#include <map>

#include "../s4ptl.hpp"
#include "../BxiQueue.hpp"
#include "BxiActor.hpp"

struct cmp_str {
    bool operator()(char const* a, char const* b) const { return std::strcmp(a, b) < 0; }
};

class BxiMainActor : public BxiActor {
  protected:
    bool service_mode;
    BxiQueue* tx_queue;
    unsigned int poll_count = 0;
    uint8_t is_sampling;

    void issue_portals_command(int simulated_size);
    void issue_portals_command();

  public:
    // Amaury says there's no need to initialise with nullptrs,
    // because there will be zeros in memory wherever this is
    // stored, so if everything breaks blame him
    struct sigaction* registered_signals[33];

    /**
     * This is actually horrible I think, we shouldn't need this attribute,
     * there is no reason to have a default interface, this is going to
     * make having several NICs per node very hard, but I don't see how to
     * make MPI work otherwise for now
     */
    BxiNI* default_ni;

    map<char*, void*, cmp_str> keyval_store;

    xbt_os_timer_t timer;
    unsigned int is_polling = 0;
    int optind              = 0;
    uint8_t sampling();
    void set_sampling(uint8_t s);
    explicit BxiMainActor(const vector<string>& args);
    ~BxiMainActor();

    // ===== Portals implementation =====

    int PtlInit(void);
    void PtlFini(void) {}
    // int PtlActivateHookAdd(void (*)(void *, unsigned int, int), void *,
    //                       ptl_activate_hook_t *);
    // int PtlActivateHookRemove(ptl_activate_hook_t);
    //
    int PtlNIInit(ptl_interface_t, unsigned int, ptl_pid_t, const ptl_ni_limits_t*, ptl_ni_limits_t*, ptl_handle_ni_t*);
    int PtlNIFini(ptl_handle_ni_t);
    // int PtlNIHandle(ptl_handle_any_t, ptl_handle_ni_t *);
    // int PtlNIStatus(ptl_handle_ni_t, ptl_sr_index_t, ptl_sr_value_t *);
    int PtlSetMap(ptl_handle_ni_t, ptl_size_t, const union ptl_process*);
    int PtlGetMap(ptl_handle_ni_t, ptl_size_t, union ptl_process*, ptl_size_t*);
    //
    int PtlPTAlloc(ptl_handle_ni_t, unsigned int, ptl_handle_eq_t, ptl_index_t, ptl_index_t*);
    int PtlPTFree(ptl_handle_ni_t, ptl_index_t);
    int PtlPTEnable(ptl_handle_ni_t, ptl_pt_index_t);
    int PtlPTDisable(ptl_handle_ni_t, ptl_pt_index_t);
    //
    int PtlGetUid(ptl_handle_ni_t, ptl_uid_t*);
    int PtlGetId(ptl_handle_ni_t, ptl_process_t*);
    int PtlGetPhysId(ptl_handle_ni_t, ptl_process_t*);
    // int PtlGetHwid(ptl_handle_ni_t, uint64_t *, uint64_t *);
    //
    int PtlMDBind(ptl_handle_ni_t, const ptl_md_t*, ptl_handle_md_t*);
    int PtlMDRelease(ptl_handle_md_t);
    //
    int PtlLEAppend(ptl_handle_ni_t, ptl_index_t, const ptl_le_t*, ptl_list_t, void*, ptl_handle_le_t*);
    int PtlLEUnlink(ptl_handle_le_t);
    // int PtlLESearch(ptl_handle_ni_t, ptl_pt_index_t, const ptl_le_t *,
    //                ptl_search_op_t, void *);
    int PtlMEAppend(ptl_handle_ni_t, ptl_index_t, const ptl_me_t*, ptl_list_t, void*, ptl_handle_me_t*);
    int PtlMEUnlink(ptl_handle_me_t);
    // int PtlMESearch(ptl_handle_ni_t, ptl_pt_index_t, const ptl_me_t *,
    //                ptl_search_op_t, void *);
    //
    int PtlEQAlloc(ptl_handle_ni_t, ptl_size_t, ptl_handle_eq_t*);
    // int PtlEQAllocAsync(ptl_handle_ni_t, ptl_size_t, ptl_handle_eq_t *,
    //                    void (*)(void *, ptl_handle_eq_t), void *, int);
    int PtlEQFree(ptl_handle_eq_t);
    int PtlEQGet(ptl_handle_eq_t, ptl_event_t*);
    int PtlEQWait(ptl_handle_eq_t, ptl_event_t*);
    int PtlEQPoll(const ptl_handle_eq_t*, unsigned int, ptl_time_t, ptl_event_t*, unsigned int*);
    //
    int PtlCTAlloc(ptl_handle_ni_t, ptl_handle_ct_t*);
    int PtlCTFree(ptl_handle_ct_t);
    // int PtlCTCancelTriggered(ptl_handle_ct_t);
    int PtlCTGet(ptl_handle_ct_t, ptl_ct_event_t*);
    int PtlCTWait(ptl_handle_ct_t, ptl_size_t, ptl_ct_event_t*);
    int PtlCTPoll(const ptl_handle_ct_t*, const ptl_size_t*, unsigned int, ptl_time_t, ptl_ct_event_t*, unsigned int*);
    int PtlCTSet(ptl_handle_ct_t, ptl_ct_event_t);
    int PtlCTInc(ptl_handle_ct_t, ptl_ct_event_t);

    int PtlPut(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t, ptl_process_t, ptl_index_t, ptl_match_bits_t,
               ptl_size_t, void*, ptl_hdr_data_t);
    int PtlGet(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t,
               void*);
    int PtlAtomic(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t, ptl_process_t, ptl_pt_index_t,
                  ptl_match_bits_t, ptl_size_t, void*, ptl_hdr_data_t, ptl_op_t, ptl_datatype_t);
    int PtlFetchAtomic(ptl_handle_md_t, ptl_size_t, ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_process_t,
                       ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void*, ptl_hdr_data_t, ptl_op_t, ptl_datatype_t);
    int PtlSwap(ptl_handle_md_t, ptl_size_t, ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_process_t, ptl_pt_index_t,
                ptl_match_bits_t, ptl_size_t, void*, ptl_hdr_data_t, const void*, ptl_op_t, ptl_datatype_t);
    // int PtlAtomicSync(void);
    // int PtlNIAtomicSync(ptl_handle_ni_t);
    //
    // int PtlTriggeredPut(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t,
    //                    ptl_process_t, ptl_index_t, ptl_match_bits_t,
    //                    ptl_size_t, void *, ptl_hdr_data_t,
    //                    ptl_handle_ct_t, ptl_size_t);
    // int PtlTriggeredGet(ptl_handle_md_t, ptl_size_t, ptl_size_t,
    //                    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t,
    //                    ptl_size_t, void *,
    //                    ptl_handle_ct_t, ptl_size_t);
    // int PtlTriggeredAtomic(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t,
    //                       ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    //                       ptl_hdr_data_t, ptl_op_t, ptl_datatype_t,
    //                       ptl_handle_ct_t, ptl_size_t);
    // int PtlTriggeredFetchAtomic(ptl_handle_md_t, ptl_size_t,
    //                            ptl_handle_md_t, ptl_size_t, ptl_size_t,
    //                            ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    //                            ptl_hdr_data_t, ptl_op_t, ptl_datatype_t,
    //                            ptl_handle_ct_t, ptl_size_t);
    // int PtlTriggeredSwap(ptl_handle_md_t, ptl_size_t,
    //                     ptl_handle_md_t, ptl_size_t, ptl_size_t,
    //                     ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    //                     ptl_hdr_data_t, const void *, ptl_op_t, ptl_datatype_t,
    //                     ptl_handle_ct_t, ptl_size_t);
    // int PtlTriggeredCTSet(ptl_handle_ct_t, ptl_ct_event_t,
    //                      ptl_handle_ct_t, ptl_size_t);
    // int PtlTriggeredCTInc(ptl_handle_ct_t, ptl_ct_event_t,
    //                      ptl_handle_ct_t, ptl_size_t);
    //
    // int PtlStartBundle(ptl_handle_ni_t);
    // int PtlEndBundle(ptl_handle_ni_t);
    int PtlHandleIsEqual(ptl_handle_any_t, ptl_handle_any_t);

    /*
     * Non-blocking commands; they are the same as above, except that they
     * may return PTL_TRY_AGAIN rather than blocking
     *
     * TODO: transform all these calls to return PTL_TRY_AGAIN if needed, as
     * PtlPutNB already does
     */

    int PtlPTEnableNB(ptl_handle_ni_t n, ptl_pt_index_t p) { return PtlPTEnable(n, p); }
    int PtlPTDisableNB(ptl_handle_ni_t n, ptl_pt_index_t p) { return PtlPTDisable(n, p); }

    int PtlLEAppendNB(ptl_handle_ni_t n, ptl_index_t i, const ptl_le_t* le, ptl_list_t li, void* v, ptl_handle_le_t* h)
    {
        return PtlLEAppend(n, i, le, li, v, h);
    }
    // int PtlLESearchNB(ptl_handle_ni_t, ptl_pt_index_t, const ptl_le_t *,
    //                  ptl_search_op_t, void *);
    int PtlMEAppendNB(ptl_handle_ni_t n, ptl_index_t i, const ptl_me_t* m, ptl_list_t l, void* v, ptl_handle_me_t* h)
    {
        return PtlMEAppend(n, i, m, l, v, h);
    }
    // int PtlMESearchNB(ptl_handle_ni_t, ptl_pt_index_t, const ptl_me_t *,
    //                  ptl_search_op_t, void *);
    //
    int PtlMDBindNB(ptl_handle_ni_t n, const ptl_md_t* m, ptl_handle_md_t* h) { return PtlMDBind(n, m, h); }

    int PtlPutNB(ptl_handle_md_t h, ptl_size_t s, ptl_size_t si, ptl_ack_req_t a, ptl_process_t p, ptl_index_t id,
                 ptl_match_bits_t m, ptl_size_t siz, void* v, ptl_hdr_data_t d);
    int PtlGetNB(ptl_handle_md_t h, ptl_size_t s, ptl_size_t si, ptl_process_t p, ptl_pt_index_t i, ptl_match_bits_t m,
                 ptl_size_t siz, void* v);
    int PtlAtomicNB(ptl_handle_md_t m, ptl_size_t s, ptl_size_t si, ptl_ack_req_t a, ptl_process_t p, ptl_pt_index_t i,
                    ptl_match_bits_t mb, ptl_size_t siz, void* v, ptl_hdr_data_t h, ptl_op_t o, ptl_datatype_t d)
    {
        return PtlAtomic(m, s, si, a, p, i, mb, siz, v, h, o, d);
    }
    int PtlFetchAtomicNB(ptl_handle_md_t m, ptl_size_t s, ptl_handle_md_t md, ptl_size_t si, ptl_size_t siz,
                         ptl_process_t p, ptl_pt_index_t i, ptl_match_bits_t mb, ptl_size_t size, void* v,
                         ptl_hdr_data_t h, ptl_op_t o, ptl_datatype_t d)
    {
        return PtlFetchAtomic(m, s, md, si, siz, p, i, mb, size, v, h, o, d);
    }
    int PtlSwapNB(ptl_handle_md_t m, ptl_size_t s, ptl_handle_md_t md, ptl_size_t si, ptl_size_t siz, ptl_process_t p,
                  ptl_pt_index_t i, ptl_match_bits_t mb, ptl_size_t size, void* v, ptl_hdr_data_t h, const void* vo,
                  ptl_op_t o, ptl_datatype_t d)
    {
        return PtlSwap(m, s, md, si, siz, p, i, mb, size, v, h, vo, o, d);
    }

    // int PtlTriggeredPutNB(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t,
    //                      ptl_process_t, ptl_index_t, ptl_match_bits_t,
    //                      ptl_size_t, void *, ptl_hdr_data_t,
    //                      ptl_handle_ct_t, ptl_size_t);
    // int PtlTriggeredGetNB(ptl_handle_md_t, ptl_size_t, ptl_size_t,
    //                      ptl_process_t, ptl_pt_index_t, ptl_match_bits_t,
    //                      ptl_size_t, void *,
    //                      ptl_handle_ct_t, ptl_size_t);
    // int PtlTriggeredAtomicNB(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t,
    //                         ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    //                         ptl_hdr_data_t, ptl_op_t, ptl_datatype_t,
    //                         ptl_handle_ct_t, ptl_size_t);
    // int PtlTriggeredFetchAtomicNB(ptl_handle_md_t, ptl_size_t,
    //                              ptl_handle_md_t, ptl_size_t, ptl_size_t,
    //                              ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    //                              ptl_hdr_data_t, ptl_op_t, ptl_datatype_t,
    //                              ptl_handle_ct_t, ptl_size_t);
    // int PtlTriggeredSwapNB(ptl_handle_md_t, ptl_size_t,
    //                       ptl_handle_md_t, ptl_size_t, ptl_size_t,
    //                       ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    //                       ptl_hdr_data_t, const void *, ptl_op_t, ptl_datatype_t,
    //                       ptl_handle_ct_t, ptl_size_t);
    // int PtlTriggeredCTSetNB(ptl_handle_ct_t, ptl_ct_event_t,
    //                        ptl_handle_ct_t, ptl_size_t);
    // int PtlTriggeredCTIncNB(ptl_handle_ct_t, ptl_ct_event_t,
    //                        ptl_handle_ct_t, ptl_size_t);
};

#endif // S4BXI_BXIMAINACTOR_HPP