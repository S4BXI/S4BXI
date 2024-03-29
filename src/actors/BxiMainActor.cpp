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

#include "s4bxi/actors/BxiMainActor.hpp"

#include <algorithm>
#include <xbt.h>
#include "s4bxi/s4bxi_xbt_log.h"

// VVVVV geteuid VVVVV
#include <unistd.h>

using namespace std;
using namespace simgrid;

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_main_actor, "Messages specific to the main actor")

s4u::BarrierPtr BxiMainActor::_barrier = nullptr;

BxiMainActor::BxiMainActor(const vector<string>& args)
{
    BxiEngine::get_instance()->register_main_actor(this);

    bull_mpi_ops = nullptr;

    char* env       = getenv("S4BXI_SMPI_IMPLEM");
    use_smpi_implem = env ? TRUTHY_CHAR(env) : 0;

    is_sampling           = 0;
    timer                 = xbt_os_timer_new();
    const char* prop      = self->get_property("use_real_memory");
    node->use_real_memory = prop && TRUTHY_CHAR(prop);

    prop            = self->get_property("model_pci");
    node->model_pci = !prop || TRUTHY_CHAR(prop);

    prop                     = self->get_property("model_pci_commands");
    node->model_pci_commands = node->model_pci && (!prop || TRUTHY_CHAR(prop));

    prop         = self->get_property("service_mode");
    service_mode = prop && TRUTHY_CHAR(prop);

    prop = self->get_property("is_daemon");
    if (prop && TRUTHY_CHAR(prop)) {
        XBT_INFO("I've been daemonized");
        self->daemonize();
    }

    if (S4BXI_GLOBAL_CONFIG(e2e_off)) {
        node->e2e_off = true;
    } else {
        auto nic_actors = node->nic_host->get_all_actors();
        for (const auto& it : nic_actors) {
            // Enable E2E only if the NIC has an actor for that
            if (strcmp(it->get_cname(), "nic_e2e") == 0) {
                node->e2e_off = false;
                break;
            }
        }
    }

    XBT_INFO("Setup with nid = %d, model_pci = %u ; e2e_off = %u", node->nid, node->model_pci ? 1 : 0,
             node->e2e_off ? 1 : 0);

    self->on_exit([](bool) { XBT_DEBUG("Dying"); });
}

BxiMainActor::~BxiMainActor()
{
    xbt_os_timer_free(timer);
}

void BxiMainActor::issue_portals_command(int simulated_size)
{
    if (S4BXI_CONFIG_AND(node, model_pci_commands) && simulated_size)
        node->pci_transfer(simulated_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_COMMAND);
}

void BxiMainActor::issue_portals_command()
{
    issue_portals_command(COMMAND_SIZE);
}

uint8_t BxiMainActor::sampling()
{
    return is_sampling;
}

void BxiMainActor::set_sampling(uint8_t s)
{
    is_sampling = s;
}

bool BxiMainActor::is_PIO(BxiMsg* msg)
{
    BxiPutRequest* request = (BxiPutRequest*)msg->parent_request;
    int inline_size        = INLINE_SIZE(request);
    int PIO_size           = PIO_SIZE(request);

    return !msg->retry_count // PIO doesn't make sense for retransmissions
           && S4BXI_CONFIG_AND(node, model_pci) && request->payload_size > inline_size &&
           request->payload_size <= PIO_size &&
           HAS_PTL_OPTION(&request->md->md, PTL_MD_VOLATILE); // In BXI PIO mode is only enabled for volatile MDs
}

void BxiMainActor::setup_barrier()
{
    // If we don't yield a lot for some reason actors start to wait on the Barrier before it's properly constructed. In
    // my opinion this makes no sense but I'm not going to question it: it works well with 2 yields and as this function
    // is only called once per MainActor the overhead is totally negligible
    s4u::this_actor::yield();
    if (!_barrier)
        _barrier = s4u::Barrier::create(s4bxi_get_rank_number());
    s4u::this_actor::yield();
}

void BxiMainActor::barrier()
{
    _barrier->wait();
}

// ==================================
// =                                =
// =     Portals implementation     =
// =                                =
// ==================================

int BxiMainActor::PtlInit(void)
{
    // Yes, this sleep is ugly, but it's the simplest way to make sure the Initiator are done setting up the queues
    s4u::this_actor::sleep_for(1e-9);

    vn       = service_mode ? S4BXI_VN_SERVICE_REQUEST : S4BXI_VN_COMPUTE_REQUEST;
    tx_queue = node->tx_queues[vn];

    return PTL_OK;
}

/**
 * This is straight out of Bull's implementation of Portals
 */
int BxiMainActor::PtlHandleIsEqual(ptl_handle_any_t handle1, ptl_handle_any_t handle2)
{
    return handle1 == handle2;
}

/**
 * In BXI the UID is the effective ID of the Linux user
 * I don't have any concept of simulated-world user, so we'll just
 * assume a single user launches all applications (which is probably
 * what happens in the real world anyway when someone runs several
 * threads using slurm)
 */
int BxiMainActor::PtlGetUid(ptl_handle_ni_t, ptl_uid_t* uid)
{
    *uid = geteuid(); // This is a high quality algorithm right there

    return PTL_OK;
}

/**
 * I guess this is temporary and we should do something else here,
 * let's say there is no logically addressed thing (seems like a bold
 * assumption for MPI though, not sure)
 */
int BxiMainActor::PtlGetId(ptl_handle_ni_t ni_handle, ptl_process_t* id)
{
    auto ni = ((BxiNI*)ni_handle);

    if (HAS_PTL_OPTION(ni, PTL_NI_PHYSICAL))
        return PtlGetPhysId(ni_handle, id);

    id->rank = ni->get_l2p_rank();

    return PTL_OK;
}

/**
 * So far we didn't consider the rank in any way (only NID / PID).
 * If it is needed by any application (probably any MPI one ?) we'll
 * have to implement that somehow (usually ranks are allocated by SLURM)
 *
 * @param ni_handle Considered network interface
 * @param id Struct that will be filled with physical information
 * @return Portals status
 */
int BxiMainActor::PtlGetPhysId(ptl_handle_ni_t ni_handle, ptl_process_t* id)
{
    id->phys.nid = node->nid;
    id->phys.pid = ((BxiNI*)ni_handle)->pid;

    return PTL_OK;
}

// ==============
// ===== NI =====
// ==============

int BxiMainActor::PtlNIInit(ptl_interface_t iface, unsigned int options, ptl_pid_t pid, const ptl_ni_limits_t* desired,
                            ptl_ni_limits_t* actual, ptl_handle_ni_t* ni_handle)
{
    issue_portals_command();

    if (pid == PTL_PID_ANY) {
        // If no specific PID is requested, find an unused one
        pid = 2048;
        while (find(node->used_pids.begin(), node->used_pids.end(), pid) != node->used_pids.end()) {
            XBT_DEBUG("PID %d in use, continue search", pid);
            ++pid;
        }
        XBT_DEBUG("Found satisfying PID for NIInit %d", pid);
    } else if (find(node->used_pids.begin(), node->used_pids.end(), pid) != node->used_pids.end()) {
        // If a specific PID is requested, check that it isn't already used
        ptl_panic_fmt("PID %d is already in use", pid);
    }

    node->used_pids.push_back(pid);

    auto ni    = BxiNI::init(node, iface, options, pid, desired, actual);
    *ni_handle = ni;
    node->ni_handles.push_back(ni);

    return PTL_OK;
}

int BxiMainActor::PtlNIFini(ptl_handle_ni_t handle)
{
    issue_portals_command();

    node->used_pids.erase(remove(node->used_pids.begin(), node->used_pids.end(), ((BxiNI*)handle)->pid));
    BxiNI::fini(handle);
    node->ni_handles.erase(remove(node->ni_handles.begin(), node->ni_handles.end(), handle));

    return PTL_OK;
}

// ==============
// ===== PT =====
// ==============

int BxiMainActor::PtlPTAlloc(ptl_handle_ni_t ni_handle, unsigned int options, ptl_handle_eq_t eq_handle,
                             ptl_index_t desired, ptl_index_t* actual)
{
    issue_portals_command();

    return BxiPT::alloc(ni_handle, options, eq_handle, desired, actual);
}

/**
 * Spec says that "The function blocks until the portal table entry is
 * enabled.", but I don't see why this would take a significant amount
 * of time
 */
int BxiMainActor::PtlPTEnable(ptl_handle_ni_t ni_handle, ptl_pt_index_t pt_index)
{
    issue_portals_command();

    return BxiPT::getFromNI(ni_handle, pt_index)->enable();
}

int BxiMainActor::PtlPTDisable(ptl_handle_ni_t ni_handle, ptl_pt_index_t pt_index)
{
    issue_portals_command();

    return BxiPT::getFromNI(ni_handle, pt_index)->disable();
}

int BxiMainActor::PtlPTFree(ptl_handle_ni_t ni_handle, ptl_index_t pt_index)
{
    issue_portals_command();

    return BxiPT::free(ni_handle, pt_index);
}

// ==============
// ===== EQ =====
// ==============

int BxiMainActor::PtlEQAlloc(ptl_handle_ni_t ni_handle, ptl_size_t, ptl_handle_eq_t* eq_handle)
{
    issue_portals_command();

    *eq_handle = new BxiEQ;

    return PTL_OK;
}

int BxiMainActor::PtlEQFree(ptl_handle_eq_t eq_handle)
{
    issue_portals_command();

    delete (BxiEQ*)eq_handle;

    return PTL_OK;
}

int BxiMainActor::PtlEQGet(ptl_handle_eq_t eq_handle, ptl_event_t* event)
{
    double active_polling_delay = S4BXI_GLOBAL_CONFIG(active_polling_delay);
    if (is_polling == -1) {
        s4u::this_actor::sleep_for(active_polling_delay);
        return ((BxiEQ*)eq_handle)->get(event);
    }

    // If polling, try to do clever things to poll less
    ++poll_count;

    s4u::this_actor::sleep_for(active_polling_delay + (poll_count > 5 ? ((poll_count - 5) * active_polling_delay) : 0));
    auto ret = ((BxiEQ*)eq_handle)->get(event);

    if (ret == PTL_OK) {
        poll_count = 0;
    }

    return ret;
}

int BxiMainActor::PtlEQWait(ptl_handle_eq_t eq_handle, ptl_event_t* event)
{
    return ((BxiEQ*)eq_handle)->wait(event);
}

int BxiMainActor::PtlEQPoll(const ptl_handle_eq_t* eq_handles, unsigned int size, ptl_time_t timeout,
                            ptl_event_t* event, unsigned int* which)
{
    return BxiEQ::poll(eq_handles, size, timeout, event, which);
}

// ==============
// ===== MD =====
// ==============

int BxiMainActor::PtlMDBind(ptl_handle_ni_t ni_handle, const ptl_md_t* md_t, ptl_handle_md_t* md_handle)
{
    issue_portals_command();

    *md_handle = new BxiMD(ni_handle, md_t);

    return PTL_OK;
}

int BxiMainActor::PtlMDRelease(ptl_handle_md_t md_handle)
{
    issue_portals_command();

    delete (BxiMD*)md_handle;

    return PTL_OK;
}

// ===================
// ===== LE / ME =====
// ===================

int BxiMainActor::PtlLEAppend(ptl_handle_ni_t ni_handle, ptl_index_t pt_index, const ptl_le_t* le_t,
                              ptl_list_t ptl_list, void* user_ptr, ptl_handle_le_t* le_handle)
{
    return PtlMEAppend(ni_handle, pt_index, (ptl_me_t*)le_t, ptl_list, user_ptr, (ptl_handle_me_t*)le_handle);
}

int BxiMainActor::PtlLEUnlink(ptl_handle_le_t le_handle)
{
    return PtlMEUnlink((ptl_handle_me_t)le_handle);
}

int BxiMainActor::PtlMEAppend(ptl_handle_ni_t ni_handle, ptl_index_t pt_index, const ptl_me_t* me_t, ptl_list_t list,
                              void* user_ptr, ptl_handle_me_t* me_handle)
{
    issue_portals_command();

    BxiPT* pt = ((BxiNI*)ni_handle)->pt_indexes.at(pt_index);
    BxiME::append(pt, me_t, list, user_ptr, me_handle);

    return PTL_OK;
}

int BxiMainActor::PtlMEUnlink(ptl_handle_le_t me_handle)
{
    issue_portals_command();

    auto me  = (BxiME*)me_handle;
    auto uhs = me->pt->unexpected_headers;

    // This is kind of greedy, I think we should associate UHs and MEs earlier instead
    // of deciding if they match in a somewhat "lazy" way
    if (me->list == PTL_OVERFLOW_LIST)
        for (BxiMsg* uh : me->pt->unexpected_headers)
            if (me->matches_request(uh->parent_request))
                return PTL_IN_USE;

    BxiME::unlink(me_handle);

    return PTL_OK;
}

// ====================
// ===== Requests =====
// ====================

int BxiMainActor::PtlPut(ptl_handle_md_t md_handle, ptl_size_t local_offset, ptl_size_t length, ptl_ack_req_t ack_req,
                         ptl_process_t target_id, ptl_index_t pt_index, ptl_match_bits_t match_bits,
                         ptl_size_t remote_offset, void* user_ptr, ptl_hdr_data_t hdr)
{
    auto m                          = (BxiMD*)md_handle;
    bool matching                   = HAS_PTL_OPTION(m->ni, PTL_NI_MATCHING);
    const ptl_process_t target_proc = m->ni->get_physical_proc(target_id);

    auto request = new BxiPutRequest(m, length, matching, match_bits, target_proc.phys.pid, pt_index, user_ptr,
                                     service_mode, local_offset, remote_offset, ack_req, hdr);
    auto msg     = new BxiMsg(node->nid, target_proc.phys.nid, S4BXI_PTL_PUT, length, request);
    // s4bxi_fprintf(stderr, " <<< Created message %p (%s) >>>\n", msg, msg_type_c_str(msg));

    int inline_size = INLINE_SIZE(request);

    m->ni->cq->acquire();
    msg->is_PIO = is_PIO(msg);
    if (msg->is_PIO) { // Payload part of PIO command
        // Model the transfer (congestion)
        node->pci_transfer_async(request->payload_size - inline_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_PIO_PAYLOAD, true);
        // The blocking phase is very short in reality because our PCI latencies are a bit overestimated (to account for
        // many phenomenons)
        s4u::this_actor::sleep_for(200e-9);
    }
    S4BXI_STARTLOG(S4BXILOG_PCI_COMMAND, node->nid, node->nid)
    // node->resume_waiting_tx_actors(vn);
    tx_queue->put(msg, 64); // Send header in a blocking way
    // s4bxi_fprintf(stderr, "    ---- Wake up initiators for message %p (%s)\n", msg, msg_type_c_str(msg));
    S4BXI_WRITELOG()

    return PTL_OK;
}

int BxiMainActor::PtlGet(ptl_handle_md_t md_handle, ptl_size_t local_offset, ptl_size_t length, ptl_process_t target_id,
                         ptl_index_t pt_index, ptl_match_bits_t match_bits, ptl_size_t remote_offset, void* user_ptr)
{
    auto m                          = (BxiMD*)md_handle;
    bool matching                   = HAS_PTL_OPTION(m->ni, PTL_NI_MATCHING);
    const ptl_process_t target_proc = m->ni->get_physical_proc(target_id);

    auto request = new BxiGetRequest(m, length, matching, match_bits, target_proc.phys.pid, pt_index, user_ptr,
                                     service_mode, local_offset, remote_offset);
    auto msg     = new BxiMsg(node->nid, target_proc.phys.nid, S4BXI_PTL_GET, 64, request);
    //                                                                       ^^^^
    //                 I Don't know what the actual size of a get request on the network is
    // s4bxi_fprintf(stderr, " <<< Created message %p (%s) >>>\n", msg, msg_type_c_str(msg));

    m->ni->cq->acquire();
    S4BXI_STARTLOG(S4BXILOG_PCI_COMMAND, node->nid, node->nid)
    // node->resume_waiting_tx_actors(vn);
    tx_queue->put(msg, 64); // Send header in a blocking way
    // s4bxi_fprintf(stderr, "    ---- Wake up initiators for message %p (%s)\n", msg, msg_type_c_str(msg));
    S4BXI_WRITELOG()

    return PTL_OK;
}

int BxiMainActor::PtlAtomic(ptl_handle_md_t md_handle, ptl_size_t loffs, ptl_size_t length, ptl_ack_req_t ack_req,
                            ptl_process_t target_id, ptl_pt_index_t pt_index, ptl_match_bits_t match_bits,
                            ptl_size_t roffs, void* user_ptr, ptl_hdr_data_t hdr, ptl_op_t op, ptl_datatype_t datatype)
{
    auto m                          = (BxiMD*)md_handle;
    bool matching                   = HAS_PTL_OPTION(m->ni, PTL_NI_MATCHING);
    const ptl_process_t target_proc = m->ni->get_physical_proc(target_id);

    auto request = new BxiAtomicRequest(m, length, matching, match_bits, target_proc.phys.pid, pt_index, user_ptr,
                                        service_mode, loffs, roffs, ack_req, hdr, op, datatype);
    auto msg     = new BxiMsg(node->nid, target_proc.phys.nid, S4BXI_PTL_ATOMIC, length, request);
    // s4bxi_fprintf(stderr, " <<< Created message %p (%s) >>>\n", msg, msg_type_c_str(msg));

    int inline_size = INLINE_SIZE(request);

    m->ni->cq->acquire();
    msg->is_PIO = is_PIO(msg);
    if (msg->is_PIO) {
        // Payload part of PIO command
        node->pci_transfer(request->payload_size - inline_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_PIO_PAYLOAD);
    }
    S4BXI_STARTLOG(S4BXILOG_PCI_COMMAND, node->nid, node->nid)
    // node->resume_waiting_tx_actors(vn);
    tx_queue->put(msg, 64); // Send header in a blocking way
    // s4bxi_fprintf(stderr, "    ---- Wake up initiators for message %p (%s)\n", msg, msg_type_c_str(msg));
    S4BXI_WRITELOG()

    // PIO / DMA logic is handled entirely by BxiNicInitiator

    return PTL_OK;
}

int BxiMainActor::PtlFetchAtomic(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh,
                                 ptl_size_t put_loffs, ptl_size_t length, ptl_process_t target_id,
                                 ptl_pt_index_t pt_index, ptl_match_bits_t match_bits, ptl_size_t roffs, void* user_ptr,
                                 ptl_hdr_data_t hdr, ptl_op_t op, ptl_datatype_t datatype)
{
    auto m_put                      = (BxiMD*)put_mdh;
    auto m_get                      = (BxiMD*)get_mdh;
    bool matching                   = HAS_PTL_OPTION(m_put->ni, PTL_NI_MATCHING);
    const ptl_process_t target_proc = m_put->ni->get_physical_proc(target_id);

    auto request =
        new BxiFetchAtomicRequest(m_put, length, matching, match_bits, target_proc.phys.pid, pt_index, user_ptr,
                                  service_mode, put_loffs, roffs, hdr, op, datatype, m_get, get_loffs);
    auto msg = new BxiMsg(node->nid, target_proc.phys.nid, S4BXI_PTL_FETCH_ATOMIC, length, request);
    // s4bxi_fprintf(stderr, " <<< Created message %p (%s) >>>\n", msg, msg_type_c_str(msg));

    int inline_size = INLINE_SIZE(request);

    m_put->ni->cq->acquire();
    msg->is_PIO = is_PIO(msg);
    if (msg->is_PIO) {
        // Payload part of PIO command
        node->pci_transfer(request->payload_size - inline_size, PCI_CPU_TO_NIC, S4BXILOG_PCI_PIO_PAYLOAD);
    }

    S4BXI_STARTLOG(S4BXILOG_PCI_COMMAND, node->nid, node->nid)
    // node->resume_waiting_tx_actors(vn);
    tx_queue->put(msg, 64); // Send header in a blocking way
    // s4bxi_fprintf(stderr, "    ---- Wake up initiators for message %p (%s)\n", msg, msg_type_c_str(msg));
    S4BXI_WRITELOG()

    // PIO / DMA logic is handled entirely by BxiNicInitiator

    return PTL_OK;
}

int BxiMainActor::PtlSwap(ptl_handle_md_t get_mdh, ptl_size_t get_loffs, ptl_handle_md_t put_mdh, ptl_size_t put_loffs,
                          ptl_size_t length, ptl_process_t target_id, ptl_pt_index_t pt_index,
                          ptl_match_bits_t match_bits, ptl_size_t roffs, void* user_ptr, ptl_hdr_data_t hdr,
                          const void* cst, ptl_op_t op, ptl_datatype_t datatype)
{
    auto m_put                      = (BxiMD*)put_mdh;
    auto m_get                      = (BxiMD*)get_mdh;
    bool matching                   = HAS_PTL_OPTION(m_put->ni, PTL_NI_MATCHING);
    const ptl_process_t target_proc = m_put->ni->get_physical_proc(target_id);

    auto request =
        new BxiFetchAtomicRequest(m_put, length, matching, match_bits, target_proc.phys.pid, pt_index, user_ptr,
                                  service_mode, put_loffs, roffs, hdr, op, datatype, m_get, get_loffs);
    auto msg = new BxiMsg(node->nid, target_proc.phys.nid, S4BXI_PTL_FETCH_ATOMIC, length, request);
    // s4bxi_fprintf(stderr, " <<< Created message %p (%s) >>>\n", msg, msg_type_c_str(msg));

    m_put->ni->cq->acquire();
    S4BXI_STARTLOG(S4BXILOG_PCI_COMMAND, node->nid, node->nid)
    // node->resume_waiting_tx_actors(vn);
    tx_queue->put(msg, 64); // Send header in a blocking way
    // s4bxi_fprintf(stderr, "    ---- Wake up initiators for message %p (%s)\n", msg, msg_type_c_str(msg));
    S4BXI_WRITELOG()

    // PIO / DMA logic is handled entirely by BxiNicInitiator

    return PTL_OK;
}

int BxiMainActor::PtlPutNB(ptl_handle_md_t md_handle, ptl_size_t s, ptl_size_t si, ptl_ack_req_t a, ptl_process_t p,
                           ptl_index_t id, ptl_match_bits_t m, ptl_size_t siz, void* v, ptl_hdr_data_t d)
{
    double active_polling_delay = S4BXI_GLOBAL_CONFIG(active_polling_delay);
    if (is_polling == -1) {
        s4u::this_actor::sleep_for(active_polling_delay);

        if (((BxiMD*)md_handle)->ni->cq->would_block())
            return PTL_TRY_AGAIN;

        return PtlPut(md_handle, s, si, a, p, id, m, siz, v, d);
    }

    // If polling, try to do clever things to poll less
    ++poll_count;

    s4u::this_actor::sleep_for(active_polling_delay + (poll_count > 5 ? ((poll_count - 5) * active_polling_delay) : 0));

    if (((BxiMD*)md_handle)->ni->cq->would_block())
        return PTL_TRY_AGAIN;

    poll_count = 0;

    return PtlPut(md_handle, s, si, a, p, id, m, siz, v, d);
}

int BxiMainActor::PtlGetNB(ptl_handle_md_t md_handle, ptl_size_t s, ptl_size_t si, ptl_process_t p, ptl_pt_index_t i,
                           ptl_match_bits_t m, ptl_size_t siz, void* v)
{
    double active_polling_delay = S4BXI_GLOBAL_CONFIG(active_polling_delay);

    if (is_polling == -1) {
        s4u::this_actor::sleep_for(active_polling_delay);

        if (((BxiMD*)md_handle)->ni->cq->would_block())
            return PTL_TRY_AGAIN;

        return PtlGet(md_handle, s, si, p, i, m, siz, v);
    }

    // If polling, try to do clever things to poll less
    ++poll_count;

    s4u::this_actor::sleep_for(active_polling_delay + (poll_count > 5 ? ((poll_count - 5) * active_polling_delay) : 0));

    if (((BxiMD*)md_handle)->ni->cq->would_block())
        return PTL_TRY_AGAIN;

    poll_count = 0;

    return PtlGet(md_handle, s, si, p, i, m, siz, v);
}

// ==============
// ===== CT =====
// ==============

int BxiMainActor::PtlCTAlloc(ptl_handle_ni_t ni_handle, ptl_handle_ct_t* ct_handle)
{
    issue_portals_command();

    *ct_handle = new BxiCT;

    return PTL_OK;
}

int BxiMainActor::PtlCTFree(ptl_handle_ct_t ct_handle)
{
    issue_portals_command();

    delete (BxiCT*)ct_handle;

    return PTL_OK;
}

int BxiMainActor::PtlCTGet(ptl_handle_ct_t ct_handle, ptl_ct_event_t* event)
{
    *event = ((BxiCT*)ct_handle)->event;

    return PTL_OK;
}

int BxiMainActor::PtlCTWait(ptl_handle_ct_t ct_handle, ptl_size_t test, ptl_ct_event_t* event)
{
    return ((BxiCT*)ct_handle)->wait(test, event);
}

int BxiMainActor::PtlCTPoll(const ptl_handle_ct_t* ct_handles, const ptl_size_t* tests, unsigned int size,
                            ptl_time_t timeout, ptl_ct_event_t* event, unsigned int* which)
{
    return BxiCT::poll(ct_handles, tests, size, timeout, event, which);
}

int BxiMainActor::PtlCTSet(ptl_handle_ct_t ct_handle, ptl_ct_event_t new_ct)
{
    return ((BxiCT*)ct_handle)->set_value(new_ct);
}

int BxiMainActor::PtlCTInc(ptl_handle_ct_t ct_handle, ptl_ct_event_t increment)
{
    return ((BxiCT*)ct_handle)->increment(increment);
}

// ===============
// ===== L2P =====
// ===============

int BxiMainActor::PtlSetMap(ptl_handle_ni_t ni_handle, ptl_size_t size, const union ptl_process* map)
{
    ((BxiNI*)ni_handle)->l2p_map = vector<ptl_process_t>(map, map + size);

    return PTL_OK;
}

int BxiMainActor::PtlGetMap(ptl_handle_ni_t ni_handle, ptl_size_t size, ptl_process* map, ptl_size_t* actual_map_size)
{
    auto ni       = ((BxiNI*)ni_handle);
    auto map_size = ni->l2p_map.size();
    if (map_size == 0)
        return PTL_NO_SPACE;
    *actual_map_size = map_size;
    auto to_copy     = size > map_size ? map_size : size;
    memcpy(map, ni->l2p_map.data(), to_copy * sizeof(ptl_process_t));

    return PTL_OK;
}