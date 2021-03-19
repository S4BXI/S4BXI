/*
 * Copyright (c) 2012-2016 Bull S.A.S
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef PORTALS4_H
#define PORTALS4_H

#ifndef __KERNEL__
#include <stddef.h>
#include <stdint.h>
#include <poll.h>
#endif

#include "s4bxi/s4bxi_sample.h"

#include "s4bxi/s4bxi_redefine.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * API version
 */
#define PTL_MAJOR_VERSION		4
#define PTL_MINOR_VERSION		0

/*
 * first interface, likely the only one, sec 3.3.5
 */
#define PTL_IFACE_DEFAULT		0

/*
 * max ptl_size_t, sec 3.3.1, sec 3.10, sec 3.11
 */
#define PTL_SIZE_MAX			((1ULL << 48) - 1)

/*
 * max ptl_pid_t sec 3.6.2
 */
#define PTL_PID_MAX			((1 << 12) - 1)

/*
 * infinite Ptl{EQ,CT}Poll, sec 3.13.9
 */
#define PTL_TIME_FOREVER		(-1)

/*
 * special handles, sec 3.3.2
 */
#define PTL_CT_NONE			NULL
#define PTL_EQ_NONE			NULL
#define PTL_INVALID_HANDLE		NULL

/*
 * options for PtlNIInit(), sec 3.6.2
 */
#define PTL_NI_PHYSICAL		(0x1 << 0)
#define PTL_NI_MATCHING		(0x1 << 1)
#define PTL_NI_LOGICAL		(0x1 << 2)
#define PTL_NI_NO_MATCHING	(0x1 << 3)

/*
 * extensions of this implementation
 */
#define PTL_NI_PRIVATE			(0x0 << 0x2) /* XXX remove this one */
#define PTL_NI_SHARED			(0x1 << 0x2)

/*
 * features bits, sec. 3.6.1, sec 3.11
 */
#define PTL_TARGET_BIND_INACCESSIBLE	0x1
#define PTL_TOTAL_DATA_ORDERING		0x2
#define PTL_COHERENT_ATOMICS		0x4
/* extensions of this implementation */
#define PTL_UNRELIABLE			0x8

/*
 * status registers, sec 3.3.7
 */
enum ptl_sr_index {
	PTL_SR_DROP_COUNT,
	PTL_SR_PERMISSION_VIOLATIONS,
	PTL_SR_OPERATION_VIOLATIONS,
};

/*
 * special identifiers, sec 3.3.6 and 3.7.1
 */
#define PTL_PT_ANY			(~0)
#define PTL_PID_ANY			0x0
#define PTL_UID_ANY			0xffffffff
#define PTL_NID_ANY			0x0
#define PTL_RANK_ANY			0x7fffff

/*
 * PT options, sec 3.7.1
 */
#define PTL_PT_ONLY_USE_ONCE		1
#define PTL_PT_ONLY_TRUNCATE		2
#define PTL_PT_FLOWCTRL			4

/*
 * event types, sec 3.13.1
 */
enum ptl_event_kind {
	PTL_EVENT_GET,
	PTL_EVENT_GET_OVERFLOW,
	PTL_EVENT_PUT,
	PTL_EVENT_PUT_OVERFLOW,
	PTL_EVENT_ATOMIC,
	PTL_EVENT_ATOMIC_OVERFLOW,
	PTL_EVENT_FETCH_ATOMIC,
	PTL_EVENT_FETCH_ATOMIC_OVERFLOW,
	PTL_EVENT_REPLY,
	PTL_EVENT_SEND,
	PTL_EVENT_ACK,
	PTL_EVENT_PT_DISABLED,
	PTL_EVENT_AUTO_UNLINK,
	PTL_EVENT_AUTO_FREE,
	PTL_EVENT_SEARCH,
	PTL_EVENT_LINK,
};

/*
 * ACK request modes, sec 3.15.1
 */
#define PTL_NO_ACK_REQ			0x0
#define PTL_CT_ACK_REQ			0x1
#define PTL_OC_ACK_REQ			0x2
#define PTL_ACK_REQ			0x3

/*
 * NI fail types, sec 3.13.3
 */
#define PTL_NI_OK			0x0
#define PTL_NI_PERM_VIOLATION		0x1
#define PTL_NI_SEGV			0x2
#define PTL_NI_PT_DISABLED		0x3
#define PTL_NI_DROPPED			0x4
#define PTL_NI_UNDELIVERABLE		0x5
#define PTL_FAIL			0x6
#define PTL_ARG_INVALID			0x7
#define PTL_IN_USE			0x8
#define PTL_NI_NO_MATCH			0x9
#define PTL_NI_TARGET_INVALID		0xa
#define PTL_NI_OP_VIOLATION		0xb

/*
 * Error codes, sec 3.19 (table 3.7). These are not generated by the hardware, so
 * we use a range above the 0..31 range to avoid dealing with namespace
 * conflicts
 */
#define PTL_OK				PTL_NI_OK
#define PTL_CT_NONE_REACHED		32
#define PTL_EQ_DROPPED			33
#define PTL_EQ_EMPTY			34
#define PTL_IGNORED			36
#define PTL_INTERRUPTED			37
#define PTL_LIST_TOO_LONG		38
#define PTL_NO_INIT			39
#define PTL_NO_SPACE			40
#define PTL_PID_IN_USE			41
#define PTL_PT_FULL			42
#define PTL_PT_EQ_NEEDED		43
#define PTL_PT_IN_USE			44
#define PTL_SIZE_INVALID		45	/* XXX: not in spec, remove? */
#define PTL_TRY_AGAIN			46	/* extension to the standard */

/*
 * PTL_IOVEC option common to MEs and MDs
 */
#define PTL_IOVEC			0x80

/*
 * ME options, sec 3.12.1
 */
#define PTL_ME_EVENT_COMM_DISABLE	0x1
#define PTL_ME_EVENT_SUCCESS_DISABLE	0x2
#define PTL_ME_EVENT_OVER_DISABLE	0x4
#define PTL_ME_EVENT_LINK_DISABLE	0x8
#define PTL_ME_EVENT_CT_BYTES		0x10
#define PTL_ME_EVENT_CT_OVERFLOW	0x20
#define PTL_ME_EVENT_CT_COMM		0x40
#define PTL_ME_OP_PUT			0x100
#define PTL_ME_OP_GET			0x200
#define PTL_ME_USE_ONCE			0x400
#define PTL_ME_ACK_DISABLE		0x800
#define PTL_ME_MAY_ALIGN		0x1000
#define PTL_ME_EVENT_UNLINK_DISABLE	0x2000
#define PTL_ME_MANAGE_LOCAL		0x4000
#define PTL_ME_NO_TRUNCATE		0x8000
#define PTL_ME_UNEXPECTED_HDR_DISABLE	0x10000
#define PTL_ME_EVENT_FLOWCTRL_DISABLE	0x20000
#define PTL_ME_IS_ACCESSIBLE		0x40000

/*
 * same as below, but with the LE prefix
 */
#define PTL_LE_OP_PUT			PTL_ME_OP_PUT
#define PTL_LE_OP_GET			PTL_ME_OP_GET
#define PTL_LE_USE_ONCE			PTL_ME_USE_ONCE
#define PTL_LE_ACK_DISABLE		PTL_ME_ACK_DISABLE
#define PTL_LE_UNEXPECTED_HDR_DISABLE	PTL_ME_UNEXPECTED_HDR_DISABLE
#define PTL_LE_IS_ACCESSIBLE		PTL_ME_IS_ACCESSIBLE
#define PTL_LE_EVENT_COMM_DISABLE	PTL_ME_EVENT_COMM_DISABLE
#define PTL_LE_EVENT_FLOWCTRL_DISABLE	PTL_ME_EVENT_FLOWCTRL_DISABLE
#define PTL_LE_EVENT_SUCCESS_DISABLE	PTL_ME_EVENT_SUCCESS_DISABLE
#define PTL_LE_EVENT_OVER_DISABLE	PTL_ME_EVENT_OVER_DISABLE
#define PTL_LE_EVENT_UNLINK_DISABLE	PTL_ME_EVENT_UNLINK_DISABLE
#define PTL_LE_EVENT_LINK_DISABLE	PTL_ME_EVENT_LINK_DISABLE
#define PTL_LE_EVENT_CT_COMM		PTL_ME_EVENT_CT_COMM
#define PTL_LE_EVENT_CT_OVERFLOW	PTL_ME_EVENT_CT_OVERFLOW
#define PTL_LE_EVENT_CT_BYTES		PTL_ME_EVENT_CT_BYTES

/*
 * MD options, sec. 3.10.1
 */
#define PTL_MD_EVENT_SEND_DISABLE	0x40
#define PTL_MD_EVENT_SUCCESS_DISABLE	0x1
#define PTL_MD_EVENT_CT_SEND		0x2
#define PTL_MD_EVENT_CT_REPLY		0x4
#define PTL_MD_EVENT_CT_ACK		0x8
#define PTL_MD_EVENT_CT_BYTES		0x10
#define PTL_MD_UNORDERED		0x20
#define PTL_MD_VOLATILE			0x100

/*
 * Ptl{ME,LE}Search() op parameter, sec 3.11.4
 */
#define PTL_SEARCH_ONLY			0x0
#define PTL_SEARCH_DELETE		0x1

/*
 * list types, sec 3.11.2
 */
#define PTL_PRIORITY_LIST		0x0
#define PTL_OVERFLOW_LIST		0x1

/*
 * atomic operations, sec. 3.15.4
 */
enum ptl_op {
	PTL_MIN				= 0x0,
	PTL_MAX				= 0x1,
	PTL_SUM				= 0x2,
	PTL_PROD			= 0x3,
	PTL_LOR				= 0x4,
	PTL_LAND			= 0x5,
	PTL_LXOR			= 0x6,
	PTL_BOR				= 0x8,
	PTL_BAND			= 0x9,
	PTL_BXOR			= 0xa,
	PTL_SWAP			= 0xc,
	PTL_CSWAP_GT			= 0x10,
	PTL_CSWAP_LT			= 0x11,
	PTL_CSWAP_GE			= 0x12,
	PTL_CSWAP_LE			= 0x13,
	PTL_CSWAP			= 0x14,
	PTL_CSWAP_NE			= 0x15,
	PTL_MSWAP			= 0x18,
};

/*
 * atomic types, sec. 3.15.4
 */
enum ptl_datatype {
	PTL_INT8_T			= 0x0,
	PTL_UINT8_T			= 0x1,
	PTL_INT16_T			= 0x2,
	PTL_UINT16_T			= 0x3,
	PTL_INT32_T			= 0x4,
	PTL_UINT32_T			= 0x5,
	PTL_INT64_T			= 0x6,
	PTL_UINT64_T			= 0x7,
	PTL_FLOAT			= 0xa,
	PTL_FLOAT_COMPLEX		= 0xb,
	PTL_DOUBLE			= 0xc,
	PTL_DOUBLE_COMPLEX		= 0xd,
	PTL_LONG_DOUBLE			= 0x14,
	PTL_LONG_DOUBLE_COMPLEX		= 0x15,
};

/*
 * activate hook event types
 */
#define PTL_ACTIVATE_ATTACH		0
#define PTL_ACTIVATE_DETACH		1

/*
 * scalar types
 */
typedef unsigned int ptl_pid_t;
typedef unsigned int ptl_nid_t;
typedef int ptl_ack_req_t;
typedef int ptl_interface_t;
typedef unsigned int ptl_rank_t;
typedef int ptl_sr_value_t;
typedef int ptl_uid_t;
typedef int ptl_list_t;
typedef unsigned int ptl_index_t;
typedef uint64_t ptl_size_t;
typedef uint64_t ptl_match_bits_t;
typedef uint64_t ptl_hdr_data_t;
typedef long ptl_time_t;
typedef void *ptl_handle_any_t;
typedef void *ptl_handle_ni_t;
typedef void *ptl_handle_md_t;
typedef void *ptl_handle_eq_t;
typedef void *ptl_handle_ct_t;
typedef void *ptl_handle_le_t;
typedef void *ptl_handle_me_t;
typedef unsigned int ptl_search_op_t;
#if defined __KERNEL__
typedef phys_addr_t ptl_addr_t;
#else
typedef void *ptl_addr_t;
#endif
typedef struct ptl_activate_hook *ptl_activate_hook_t;

typedef unsigned int ptl_pt_index_t;
typedef unsigned int ptl_ni_fail_t;
typedef enum ptl_sr_index ptl_sr_index_t;
typedef enum ptl_datatype ptl_datatype_t;
typedef enum ptl_op ptl_op_t;
typedef enum ptl_event_kind ptl_event_kind_t;

/*
 * nic limits, sec. 3.6.1
 */
struct ptl_ni_limits {
	int max_entries;
	int max_unexpected_headers;
	int max_mds;
	int max_cts;
	int max_eqs;
	int max_pt_index;
	int max_iovecs;
	int max_list_size;
	int max_triggered_ops;
	ptl_size_t max_msg_size;
	ptl_size_t max_atomic_size;
	ptl_size_t max_fetch_atomic_size;
	ptl_size_t max_waw_ordered_size;
	ptl_size_t max_war_ordered_size;
	ptl_size_t max_volatile_size;
	unsigned int features;
};

/*
 * logical interface to physical (nid, pid) mapping entry, sec. 3.9.1
 */
union ptl_process {
	struct {
		ptl_nid_t nid;
		ptl_pid_t pid;
	} phys;
	ptl_rank_t rank;
};

/*
 * memory descriptor, sec. 3.10.1
 */
struct ptl_md {
	ptl_addr_t start;
	ptl_size_t length;
	unsigned int options;
	ptl_handle_eq_t eq_handle;
	ptl_handle_ct_t ct_handle;
};

/*
 * matching list entry, sec. 3.12.1
 */
struct ptl_me {
	ptl_addr_t start;
	ptl_size_t length;
	ptl_handle_ct_t ct_handle;
	ptl_uid_t uid;
	unsigned int options;
	union ptl_process match_id;
	ptl_match_bits_t match_bits;
	ptl_match_bits_t ignore_bits;
	ptl_size_t min_free;
};

/*
 * event, sec. 3.13.4
 */
struct ptl_event {
	ptl_addr_t start;
	void *user_ptr;
	ptl_hdr_data_t hdr_data;
	ptl_match_bits_t match_bits;
	ptl_size_t rlength, mlength, remote_offset;
	ptl_uid_t uid;
	union ptl_process initiator;
	ptl_event_kind_t type;
	ptl_list_t ptl_list;
	ptl_pt_index_t pt_index;
	ptl_ni_fail_t ni_fail_type;
	ptl_op_t atomic_operation;
	ptl_datatype_t atomic_type;
};

/*
 * I/O Vector, sec. 3.10.2
 */
struct ptl_iovec {
	ptl_addr_t iov_base;
	ptl_size_t iov_len;
};

/*
 * CT event, sec. 3.14.1
 */
struct ptl_ct_event {
	ptl_size_t success;
	ptl_size_t failure;
};

/*
 * people love typedefs
 */
typedef struct ptl_md ptl_md_t;
typedef struct ptl_me ptl_le_t;
typedef struct ptl_me ptl_me_t;
typedef struct ptl_ni_limits ptl_ni_limits_t;
typedef union ptl_process ptl_process_t;
typedef struct ptl_event ptl_event_t;
typedef struct ptl_iovec ptl_iovec_t;
typedef struct ptl_ct_event ptl_ct_event_t;

int PtlInit(void);
void PtlFini(void);
int PtlActivateHookAdd(void (*)(void *, unsigned int, int), void *,
    ptl_activate_hook_t *);
int PtlActivateHookRemove(ptl_activate_hook_t);

int PtlNIInit(ptl_interface_t, unsigned int, ptl_pid_t,
    const ptl_ni_limits_t *, ptl_ni_limits_t *, ptl_handle_ni_t *);
int PtlNIFini(ptl_handle_ni_t);
int PtlNIHandle(ptl_handle_any_t, ptl_handle_ni_t *);
int PtlNIStatus(ptl_handle_ni_t, ptl_sr_index_t, ptl_sr_value_t *);
int PtlSetMap(ptl_handle_ni_t, ptl_size_t, const union ptl_process *);
int PtlGetMap(ptl_handle_ni_t, ptl_size_t, union ptl_process *, ptl_size_t *);

int PtlPTAlloc(ptl_handle_ni_t, unsigned int, ptl_handle_eq_t,
    ptl_index_t, ptl_index_t *);
int PtlPTFree(ptl_handle_ni_t, ptl_index_t);
int PtlPTEnable(ptl_handle_ni_t, ptl_pt_index_t);
int PtlPTDisable(ptl_handle_ni_t, ptl_pt_index_t);

int PtlGetUid(ptl_handle_ni_t, ptl_uid_t *);
int PtlGetId(ptl_handle_ni_t, ptl_process_t *);
int PtlGetPhysId(ptl_handle_ni_t, ptl_process_t *);
int PtlGetHwid(ptl_handle_ni_t, uint64_t *, uint64_t *);

int PtlMDBind(ptl_handle_ni_t, const ptl_md_t *, ptl_handle_md_t *);
int PtlMDRelease(ptl_handle_md_t);

int PtlLEAppend(ptl_handle_ni_t, ptl_index_t, const ptl_le_t *,
    ptl_list_t, void *, ptl_handle_le_t *);
int PtlLEUnlink(ptl_handle_le_t);
int PtlLESearch(ptl_handle_ni_t, ptl_pt_index_t, const ptl_le_t *,
    ptl_search_op_t, void *);
int PtlMEAppend(ptl_handle_ni_t, ptl_index_t, const ptl_me_t *,
    ptl_list_t, void *, ptl_handle_me_t *);
int PtlMEUnlink(ptl_handle_me_t);
int PtlMESearch(ptl_handle_ni_t, ptl_pt_index_t, const ptl_me_t *,
    ptl_search_op_t, void *);

int PtlEQAlloc(ptl_handle_ni_t, ptl_size_t, ptl_handle_eq_t *);
int PtlEQAllocAsync(ptl_handle_ni_t, ptl_size_t, ptl_handle_eq_t *,
    void (*)(void *, ptl_handle_eq_t), void *, int);
int PtlEQFree(ptl_handle_eq_t);
int PtlEQGet(ptl_handle_eq_t, ptl_event_t *);
int PtlEQWait(ptl_handle_eq_t, ptl_event_t *);
int PtlEQPoll(const ptl_handle_eq_t *, unsigned int,
    ptl_time_t, ptl_event_t *, unsigned int *);

int PtlCTAlloc(ptl_handle_ni_t, ptl_handle_ct_t *);
int PtlCTFree(ptl_handle_ct_t);
int PtlCTCancelTriggered(ptl_handle_ct_t);
int PtlCTGet(ptl_handle_ct_t, ptl_ct_event_t *);
int PtlCTWait(ptl_handle_ct_t, ptl_size_t, ptl_ct_event_t *);
int PtlCTPoll(const ptl_handle_ct_t *, const ptl_size_t *, unsigned int,
    ptl_time_t, ptl_ct_event_t *, unsigned int *);
int PtlCTSet(ptl_handle_ct_t, ptl_ct_event_t);
int PtlCTInc(ptl_handle_ct_t, ptl_ct_event_t);

int PtlPut(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t,
    ptl_process_t, ptl_index_t, ptl_match_bits_t,
    ptl_size_t, void *, ptl_hdr_data_t);
int PtlGet(ptl_handle_md_t, ptl_size_t, ptl_size_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t,
    ptl_size_t, void *);
int PtlAtomic(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    ptl_hdr_data_t, ptl_op_t, ptl_datatype_t);
int PtlFetchAtomic(ptl_handle_md_t, ptl_size_t,
    ptl_handle_md_t, ptl_size_t, ptl_size_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    ptl_hdr_data_t, ptl_op_t, ptl_datatype_t);
int PtlSwap(ptl_handle_md_t, ptl_size_t,
    ptl_handle_md_t, ptl_size_t, ptl_size_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    ptl_hdr_data_t, const void *, ptl_op_t, ptl_datatype_t);
int PtlAtomicSync(void);
int PtlNIAtomicSync(ptl_handle_ni_t);

int PtlTriggeredPut(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t,
    ptl_process_t, ptl_index_t, ptl_match_bits_t,
    ptl_size_t, void *, ptl_hdr_data_t,
    ptl_handle_ct_t, ptl_size_t);
int PtlTriggeredGet(ptl_handle_md_t, ptl_size_t, ptl_size_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t,
    ptl_size_t, void *,
    ptl_handle_ct_t, ptl_size_t);
int PtlTriggeredAtomic(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    ptl_hdr_data_t, ptl_op_t, ptl_datatype_t,
    ptl_handle_ct_t, ptl_size_t);
int PtlTriggeredFetchAtomic(ptl_handle_md_t, ptl_size_t,
    ptl_handle_md_t, ptl_size_t, ptl_size_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    ptl_hdr_data_t, ptl_op_t, ptl_datatype_t,
    ptl_handle_ct_t, ptl_size_t);
int PtlTriggeredSwap(ptl_handle_md_t, ptl_size_t,
    ptl_handle_md_t, ptl_size_t, ptl_size_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    ptl_hdr_data_t, const void *, ptl_op_t, ptl_datatype_t,
    ptl_handle_ct_t, ptl_size_t);
int PtlTriggeredCTSet(ptl_handle_ct_t, ptl_ct_event_t,
    ptl_handle_ct_t, ptl_size_t);
int PtlTriggeredCTInc(ptl_handle_ct_t, ptl_ct_event_t,
    ptl_handle_ct_t, ptl_size_t);

int PtlStartBundle(ptl_handle_ni_t);
int PtlEndBundle(ptl_handle_ni_t);
int PtlHandleIsEqual(ptl_handle_any_t, ptl_handle_any_t);

/*
 * Non-blocking commands; they are the same as above, except that they
 * may return PTL_TRY_AGAIN rather than blocking
 */

int PtlPTEnableNB(ptl_handle_ni_t, ptl_pt_index_t);
int PtlPTDisableNB(ptl_handle_ni_t, ptl_pt_index_t);

int PtlLEAppendNB(ptl_handle_ni_t, ptl_index_t, const ptl_le_t *,
    ptl_list_t, void *, ptl_handle_le_t *);
int PtlLESearchNB(ptl_handle_ni_t, ptl_pt_index_t, const ptl_le_t *,
    ptl_search_op_t, void *);
int PtlMEAppendNB(ptl_handle_ni_t, ptl_index_t, const ptl_me_t *,
    ptl_list_t, void *, ptl_handle_me_t *);
int PtlMESearchNB(ptl_handle_ni_t, ptl_pt_index_t, const ptl_me_t *,
    ptl_search_op_t, void *);

int PtlMDBindNB(ptl_handle_ni_t, const ptl_md_t *, ptl_handle_md_t *);

int PtlPutNB(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t,
    ptl_process_t, ptl_index_t, ptl_match_bits_t,
    ptl_size_t, void *, ptl_hdr_data_t);
int PtlGetNB(ptl_handle_md_t, ptl_size_t, ptl_size_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t,
    ptl_size_t, void *);
int PtlAtomicNB(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    ptl_hdr_data_t, ptl_op_t, ptl_datatype_t);
int PtlFetchAtomicNB(ptl_handle_md_t, ptl_size_t,
    ptl_handle_md_t, ptl_size_t, ptl_size_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    ptl_hdr_data_t, ptl_op_t, ptl_datatype_t);
int PtlSwapNB(ptl_handle_md_t, ptl_size_t,
    ptl_handle_md_t, ptl_size_t, ptl_size_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    ptl_hdr_data_t, const void *, ptl_op_t, ptl_datatype_t);

int PtlTriggeredPutNB(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t,
    ptl_process_t, ptl_index_t, ptl_match_bits_t,
    ptl_size_t, void *, ptl_hdr_data_t,
    ptl_handle_ct_t, ptl_size_t);
int PtlTriggeredGetNB(ptl_handle_md_t, ptl_size_t, ptl_size_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t,
    ptl_size_t, void *,
    ptl_handle_ct_t, ptl_size_t);
int PtlTriggeredAtomicNB(ptl_handle_md_t, ptl_size_t, ptl_size_t, ptl_ack_req_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    ptl_hdr_data_t, ptl_op_t, ptl_datatype_t,
    ptl_handle_ct_t, ptl_size_t);
int PtlTriggeredFetchAtomicNB(ptl_handle_md_t, ptl_size_t,
    ptl_handle_md_t, ptl_size_t, ptl_size_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    ptl_hdr_data_t, ptl_op_t, ptl_datatype_t,
    ptl_handle_ct_t, ptl_size_t);
int PtlTriggeredSwapNB(ptl_handle_md_t, ptl_size_t,
    ptl_handle_md_t, ptl_size_t, ptl_size_t,
    ptl_process_t, ptl_pt_index_t, ptl_match_bits_t, ptl_size_t, void *,
    ptl_hdr_data_t, const void *, ptl_op_t, ptl_datatype_t,
    ptl_handle_ct_t, ptl_size_t);
int PtlTriggeredCTSetNB(ptl_handle_ct_t, ptl_ct_event_t,
    ptl_handle_ct_t, ptl_size_t);
int PtlTriggeredCTIncNB(ptl_handle_ct_t, ptl_ct_event_t,
    ptl_handle_ct_t, ptl_size_t);

#ifndef __KERNEL__
int PtlNFDs(ptl_handle_ni_t);
int PtlPollFDs(ptl_handle_ni_t, struct pollfd *, int);
int PtlPollEvents(ptl_handle_ni_t, struct pollfd *);
#endif

// S4BXI specific things
// (this should probably be elsewhere but it's so much easier this way)

#include <stdio.h>

int s4bxi_fprintf(FILE* stream, const char* fmt, ...);
int s4bxi_printf(const char* fmt, ...);
void s4bxi_compute(double flops);
uint32_t s4bxi_get_my_rank();
uint32_t s4bxi_get_rank_number();
char* s4bxi_get_hostname_from_rank(int rank);
int s4bxi_get_ptl_process_from_rank(int rank, ptl_process_t* out);
double s4bxi_simtime();
unsigned int s4bxi_is_polling();
void s4bxi_set_polling(unsigned int p);
void s4bxi_exit();
void s4bxi_barrier();
void s4bxi_set_ptl_process_for_rank(ptl_handle_ni_t ni);

#ifdef __cplusplus
}
#endif

#endif
