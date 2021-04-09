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

#ifndef S4BXI_S4PTL_HPP
#define S4BXI_S4PTL_HPP

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <simgrid/s4u.hpp>
#include <memory>
#include "s4bxi_mailbox_pool.hpp"

#include "portals4.h"

// Early declaration in order not to break every single include
class BxiNode;

#define ACK_SIZE     32
#define EVENT_SIZE   96 // Currently equal to `sizeof(ptl_event_t)`
#define COMMAND_SIZE 64

using namespace std;
using namespace simgrid;

enum bxi_msg_type {
    S4BXI_E2E_ACK,
    S4BXI_PTL_ACK,
    S4BXI_PTL_RESPONSE,
    S4BXI_PTL_PUT,
    S4BXI_PTL_GET,
    S4BXI_PTL_ATOMIC,
    S4BXI_PTL_FETCH_ATOMIC,
    S4BXI_PTL_FETCH_ATOMIC_RESPONSE
};

enum bxi_req_type {
    S4BXI_FETCH_ATOMIC_REQUEST,
    S4BXI_ATOMIC_REQUEST,
    S4BXI_PUT_REQUEST,
    S4BXI_GET_REQUEST,
};

enum bxi_req_process_state {
    S4BXI_REQ_CREATED,
    S4BXI_REQ_RECEIVED,
    S4BXI_REQ_ANSWERED,
    S4BXI_REQ_FINISHED,
};

enum bxi_vn {
    S4BXI_VN_SERVICE_REQUEST,
    S4BXI_VN_COMPUTE_REQUEST,
    S4BXI_VN_SERVICE_RESPONSE,
    S4BXI_VN_COMPUTE_RESPONSE,
};

class ActorWaitingCT {
  public:
    s4u::Actor* actor;
    s4u::Mailbox* mailbox;
    ptl_size_t test;
};

// All the following /Bxi([A-Z]+)/ structs are the actual types
// behind the Portals handles (like ptl_handle_ct_t for example)

class BxiNI;
class BxiPT;
class BxiCT;
class BxiEQ;
class BxiMD;
class BxiME;
class BxiRequest;
class BxiMsg;

typedef vector<BxiME*> BxiList;
// typedef shared_ptr<BxiList> BxiListPtr;

class BxiPT {
  public:
    BxiList* priority_list;             // Stores both ME and LE
    BxiList* overflow_list;             // Same
    vector<BxiMsg*> unexpected_headers; // On the NIC UH are a type of ME, but for us I think a BxiMsg is easier
    BxiEQ* eq;
    BxiNI* ni;
    unsigned int options;
    bool enabled = true;
    ptl_index_t index;

    BxiPT(ptl_handle_ni_t ni_handle, ptl_handle_eq_t eq_handle, ptl_index_t index, unsigned int options);
    ~BxiPT();

    static int alloc(ptl_handle_ni_t ni_handle, unsigned int options, ptl_handle_eq_t eq_handle, ptl_index_t desired,
                     ptl_index_t* actual);
    static int free(ptl_handle_ni_t ni_handle, ptl_index_t pt_index);
    static BxiPT* getFromNI(ptl_handle_ni_t ni_handle, ptl_index_t pt_index);

    int enable();
    int disable();
    BxiME* walk_through_lists(BxiMsg* msg);
    bool walk_through_UHs(BxiME* me);
};

class BxiNI {
  public:
    BxiNode* node;
    ptl_interface_t iface;
    unsigned int options;
    ptl_ni_limits* limits;
    ptl_pid_t pid;
    s4u::SemaphorePtr cq;
    map<ptl_pt_index_t, BxiPT*> pt_indexes;
    vector<ptl_process_t> l2p_map;

    BxiNI(BxiNode* node, ptl_interface_t iface, unsigned int options, ptl_pid_t pid, ptl_ni_limits_t* limits);

    static BxiNI* init(BxiNode* node, ptl_interface_t iface, unsigned int options, ptl_pid_t pid,
                       const ptl_ni_limits_t* desired, ptl_ni_limits_t* actual);
    static void fini(ptl_handle_ni_t handle);
    bool can_match_request(BxiRequest* req);
    ptl_rank_t get_l2p_rank();
    const ptl_process_t get_physical_proc(const ptl_process_t& proc);
};

class BxiCT {
  public:
    void on_update();
    vector<ActorWaitingCT> waiting;
    ptl_ct_event_t event;

    BxiCT();
    int increment(ptl_ct_event_t);
    int set_value(ptl_ct_event_t);
    void increment_success(ptl_size_t);
    int wait(ptl_size_t test, ptl_ct_event_t* ev);

    static int poll(const ptl_handle_ct_t* ct_handles, const ptl_size_t* tests, unsigned int size, ptl_time_t timeout,
                    ptl_ct_event_t* event, unsigned int* which);
};

class BxiEQ {
  public:
    s4u::Mailbox* mailbox;

    BxiEQ();
    ~BxiEQ();
    int get(ptl_event_t* event);
    int wait(ptl_event_t* event);

    static int poll(const ptl_handle_eq_t* eq_handles, unsigned int size, ptl_time_t timeout, ptl_event_t* event,
                    unsigned int* which);
};

class BxiMD {
  public:
    BxiNI* ni;
    ptl_md_t* md;

    BxiMD(ptl_handle_ni_t ni_handle, const ptl_md_t* md_t);
    BxiMD(const BxiMD& md);
    ~BxiMD();
    void increment_ct(ptl_size_t byte_count);
};

class BxiME {
  public:
    bool used = false;
    BxiPT* pt;
    ptl_me_t* me; // Both LE and ME are ptl_me internally (cf Portals typedefs)
    void* user_ptr;
    ptl_size_t manage_local_offset = 0;
    ptl_list_t list;

    BxiME(BxiPT* pt, const ptl_me_t* me_t, ptl_list_t list, void* user_ptr);
    BxiME(const BxiME& me);
    ~BxiME();
    void increment_ct(ptl_size_t byte_count);
    bool matches_request(BxiRequest* req);
    ptl_addr_t get_offsetted_addr(BxiMsg* msg, bool update_manage_local_offset = false);
    BxiList* get_list();
    BxiList* get_list(BxiPT* pt);

    static void append(BxiPT* pt, const ptl_me_t* me_t, ptl_list_t list, void* user_ptr, ptl_handle_me_t* me_handle);
    static void unlink(ptl_handle_me_t me_handle);
    static void maybe_auto_unlink(BxiME* me);
};

// Next two classes are our representation of messages on the
// networks (the structures that are exchanged in mailboxes)
// They are only for internal use and are not the representation
// of a Portals entity (no associated handle, etc.)

/**
 * A request models a Portals operation (like a Put or Get),
 * so most likely several messages will be associated with each
 * request (a PTL_PUT, a _PTL_ACK and a _BXI_ACK typically)
 */
class BxiRequest {
  public:
    bxi_req_type type;
    uint64_t payload_size; // In bytes
    BxiMD* md;
    bool matching;
    ptl_match_bits_t match_bits;
    ptl_pid_t target_pid;
    ptl_pt_index_t pt_index;
    bxi_req_process_state process_state;
    unsigned int msg_ref_count = 0;
    void* user_ptr;
    bool service_vn;
    ptl_size_t local_offset;
    ptl_size_t remote_offset;
    ptl_addr_t start; // "start" as in a ptl_event_t, it's easier to store it in the request than to re-compute it when
                      // issuing events, so there it is
    BxiME* matched_me = nullptr; // Unused for PUT and ATOMIC on priority list

    BxiRequest(bxi_req_type type, BxiMD* md, ptl_size_t payload_size, bool matching, ptl_match_bits_t match_bits,
               ptl_pid_t target_pid, ptl_pt_index_t pt_index, void* user_ptr, bool service_vn, ptl_size_t local_offset,
               ptl_size_t remote_offset);
    ~BxiRequest();
};

class BxiPutRequest : public BxiRequest {
  public:
    ptl_ack_req_t ack_req;
    ptl_hdr_data_t hdr;
    bool send_event_issued = false;

    BxiPutRequest(BxiMD* md, ptl_size_t payload_size, bool matching, ptl_match_bits_t match_bits, ptl_pid_t target_pid,
                  ptl_pt_index_t pt_index, void* user_ptr, bool service_vn, ptl_size_t local_offset,
                  ptl_size_t remote_offset, ptl_ack_req_t ack_req, ptl_hdr_data_t hdr);

    void issue_ack();
    void maybe_issue_send();
};

class BxiAtomicRequest : public BxiPutRequest {
  public:
    ptl_op_t op;
    ptl_datatype_t datatype;

    BxiAtomicRequest(BxiMD* md, ptl_size_t payload_size, bool matching, ptl_match_bits_t match_bits,
                     ptl_pid_t target_pid, ptl_pt_index_t pt_index, void* user_ptr, bool service_vn,
                     ptl_size_t local_offset, ptl_size_t remote_offset, ptl_ack_req_t ack_req, ptl_hdr_data_t hdr,
                     ptl_op_t op, ptl_datatype_t datatype);
};

class BxiFetchAtomicRequest : public BxiAtomicRequest {
  public:
    BxiMD* get_md;
    ptl_size_t get_local_offset;
    bool fetch_atomic_event_issued = false;

    BxiFetchAtomicRequest(BxiMD* put_md, ptl_size_t payload_size, bool matching, ptl_match_bits_t match_bits,
                          ptl_pid_t target_pid, ptl_pt_index_t pt_index, void* user_ptr, bool service_vn,
                          ptl_size_t local_offset, ptl_size_t remote_offset, ptl_hdr_data_t hdr, ptl_op_t op,
                          ptl_datatype_t datatype, BxiMD* get_md, ptl_size_t get_local_offset);
    ~BxiFetchAtomicRequest();
    bool is_swap_request();
};

class BxiSwapRequest : public BxiFetchAtomicRequest {
  public:
    const void* cst;

    BxiSwapRequest(BxiMD* put_md, ptl_size_t payload_size, bool matching, ptl_match_bits_t match_bits,
                   ptl_pid_t target_pid, ptl_pt_index_t pt_index, void* user_ptr, bool service_vn,
                   ptl_size_t local_offset, ptl_size_t remote_offset, ptl_hdr_data_t hdr, ptl_op_t op,
                   ptl_datatype_t datatype, BxiMD* get_md, ptl_size_t get_local_offset, const void* cst);
    ~BxiSwapRequest();
};

class BxiGetRequest : public BxiRequest {
  public:
    bool get_event_issued = false;

    BxiGetRequest(BxiMD* md, ptl_size_t payload_size, bool matching, ptl_match_bits_t match_bits, ptl_pid_t target_pid,
                  ptl_pt_index_t pt_index, void* user_ptr, bool service_vn, ptl_size_t local_offset,
                  ptl_size_t remote_offset);
    ~BxiGetRequest();
};

/**
 * A single message, which is part of a bigger request
 */
class BxiMsg {
  public:
    ptl_nid_t initiator;
    ptl_nid_t target;
    uint64_t simulated_size; // In bytes
    double send_init_time;   // In seconds
    int retry_count;
    bxi_msg_type type;
    BxiRequest* parent_request;
    unsigned int ref_count;

    BxiMsg(ptl_nid_t initiator, ptl_nid_t target, bxi_msg_type type, ptl_size_t simulated_size,
           BxiRequest* parent_request);
    BxiMsg(const BxiMsg& msg);
    ~BxiMsg();

    static void unref(BxiMsg* msg);
};

#endif // S4BXI_S4PTL_HPP
