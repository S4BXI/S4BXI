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

#ifndef S4BXI_MPI_MIDDLEWARE_H
#define S4BXI_MPI_MIDDLEWARE_H

// Inside Bull's OMPI there are already all the type definition we need, steal them in other cases only
#ifndef OMPI_MPI_H
#include <smpi/smpi.h>
#else
// Copied from SMPI, I guess they're not standard MPI types therefore user code doesn't understand them
typedef void MPI_Handler_function(MPI_Comm*, int*, ...);
typedef void MPI_Comm_errhandler_function(MPI_Comm*, int*, ...);
typedef void MPI_File_errhandler_function(MPI_File*, int*, ...);
typedef void MPI_Win_errhandler_function(MPI_Win*, int*, ...);
typedef MPI_Comm_errhandler_function MPI_Comm_errhandler_fn;
typedef MPI_File_errhandler_function MPI_File_errhandler_fn;
typedef MPI_Win_errhandler_function MPI_Win_errhandler_fn;
#endif

#ifdef __cplusplus
extern "C" {
#endif

void s4bxi_log_pending_requests();

// "Constants"
void* S4BXI_MPI_IN_PLACE();
MPI_Request S4BXI_MPI_REQUEST_NULL();

void set_mpi_middleware_ops(void* bull_libhandle, void* smpi_libhandle);

int s4bxi_is_mpi_request_null(MPI_Request r);

#define S4BXI_MPI_CALL(rtype, name, ...) rtype S4BXI_##name(const char* __file, int __line, ##__VA_ARGS__)

S4BXI_MPI_CALL(int, MPI_Init, int* argc, char*** argv);
S4BXI_MPI_CALL(int, MPI_Finalize);
S4BXI_MPI_CALL(int, MPI_Finalized, int* flag);
S4BXI_MPI_CALL(int, MPI_Init_thread, int* argc, char*** argv, int required, int* provided);
S4BXI_MPI_CALL(int, MPI_Initialized, int* flag);
S4BXI_MPI_CALL(int, MPI_Query_thread, int* provided);
S4BXI_MPI_CALL(int, MPI_Is_thread_main, int* flag);
S4BXI_MPI_CALL(int, MPI_Get_version, int* version, int* subversion);
S4BXI_MPI_CALL(int, MPI_Get_library_version, char* version, int* len);
S4BXI_MPI_CALL(int, MPI_Get_processor_name, char* name, int* resultlen);
S4BXI_MPI_CALL(int, MPI_Abort, MPI_Comm comm, int errorcode);
S4BXI_MPI_CALL(int, MPI_Alloc_mem, MPI_Aint size, MPI_Info info, void* baseptr);
S4BXI_MPI_CALL(int, MPI_Free_mem, void* base);
S4BXI_MPI_CALL(double, MPI_Wtime);
S4BXI_MPI_CALL(double, MPI_Wtick);
S4BXI_MPI_CALL(int, MPI_Buffer_attach, void* buffer, int size);
S4BXI_MPI_CALL(int, MPI_Buffer_detach, void* buffer, int* size);
S4BXI_MPI_CALL(int, MPI_Address, const void* location, MPI_Aint* address);
S4BXI_MPI_CALL(int, MPI_Get_address, const void* location, MPI_Aint* address);
S4BXI_MPI_CALL(MPI_Aint, MPI_Aint_diff, MPI_Aint base, MPI_Aint disp);
S4BXI_MPI_CALL(MPI_Aint, MPI_Aint_add, MPI_Aint base, MPI_Aint disp);
S4BXI_MPI_CALL(int, MPI_Error_class, int errorcode, int* errorclass);
S4BXI_MPI_CALL(int, MPI_Error_string, int errorcode, char* string, int* resultlen);

S4BXI_MPI_CALL(int, MPI_Attr_delete, MPI_Comm comm, int keyval);
S4BXI_MPI_CALL(int, MPI_Attr_get, MPI_Comm comm, int keyval, void* attr_value, int* flag);
S4BXI_MPI_CALL(int, MPI_Attr_put, MPI_Comm comm, int keyval, void* attr_value);
S4BXI_MPI_CALL(int, MPI_Keyval_create, MPI_Copy_function* copy_fn, MPI_Delete_function* delete_fn, int* keyval,
               void* extra_state);
S4BXI_MPI_CALL(int, MPI_Keyval_free, int* keyval);

S4BXI_MPI_CALL(int, MPI_Type_free, MPI_Datatype* datatype);
S4BXI_MPI_CALL(int, MPI_Type_size, MPI_Datatype datatype, int* size);
S4BXI_MPI_CALL(int, MPI_Type_size_x, MPI_Datatype datatype, MPI_Count* size);
S4BXI_MPI_CALL(int, MPI_Type_get_extent, MPI_Datatype datatype, MPI_Aint* lb, MPI_Aint* extent);
S4BXI_MPI_CALL(int, MPI_Type_get_extent_x, MPI_Datatype datatype, MPI_Count* lb, MPI_Count* extent);
S4BXI_MPI_CALL(int, MPI_Type_get_true_extent, MPI_Datatype datatype, MPI_Aint* lb, MPI_Aint* extent);
S4BXI_MPI_CALL(int, MPI_Type_get_true_extent_x, MPI_Datatype datatype, MPI_Count* lb, MPI_Count* extent);
S4BXI_MPI_CALL(int, MPI_Type_extent, MPI_Datatype datatype, MPI_Aint* extent);
S4BXI_MPI_CALL(int, MPI_Type_lb, MPI_Datatype datatype, MPI_Aint* disp);
S4BXI_MPI_CALL(int, MPI_Type_ub, MPI_Datatype datatype, MPI_Aint* disp);
S4BXI_MPI_CALL(int, MPI_Type_commit, MPI_Datatype* datatype);
S4BXI_MPI_CALL(int, MPI_Type_hindexed, int count, const int* blocklens, const MPI_Aint* indices, MPI_Datatype old_type,
               MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_create_hindexed, int count, const int* blocklens, const MPI_Aint* indices,
               MPI_Datatype old_type, MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_create_hindexed_block, int count, int blocklength, const MPI_Aint* indices,
               MPI_Datatype old_type, MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_hvector, int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type,
               MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_create_hvector, int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type,
               MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_indexed, int count, const int* blocklens, const int* indices, MPI_Datatype old_type,
               MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_create_indexed, int count, const int* blocklens, const int* indices, MPI_Datatype old_type,
               MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_create_indexed_block, int count, int blocklength, const int* indices,
               MPI_Datatype old_type, MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_struct, int count, const int* blocklens, const MPI_Aint* indices,
               const MPI_Datatype* old_types, MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_create_struct, int count, const int* blocklens, const MPI_Aint* indices,
               const MPI_Datatype* old_types, MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_vector, int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_contiguous, int count, MPI_Datatype old_type, MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_create_resized, MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype* newtype);
S4BXI_MPI_CALL(MPI_Datatype, MPI_Type_f2c, MPI_Fint datatype);
S4BXI_MPI_CALL(MPI_Fint, MPI_Type_c2f, MPI_Datatype datatype);
S4BXI_MPI_CALL(int, MPI_Get_count, const MPI_Status* status, MPI_Datatype datatype, int* count);
S4BXI_MPI_CALL(int, MPI_Type_get_attr, MPI_Datatype type, int type_keyval, void* attribute_val, int* flag);
S4BXI_MPI_CALL(int, MPI_Type_set_attr, MPI_Datatype type, int type_keyval, void* att);
S4BXI_MPI_CALL(int, MPI_Type_delete_attr, MPI_Datatype type, int comm_keyval);
S4BXI_MPI_CALL(int, MPI_Type_create_keyval, MPI_Type_copy_attr_function* copy_fn,
               MPI_Type_delete_attr_function* delete_fn, int* keyval, void* extra_state);
S4BXI_MPI_CALL(int, MPI_Type_free_keyval, int* keyval);
S4BXI_MPI_CALL(int, MPI_Type_dup, MPI_Datatype datatype, MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_set_name, MPI_Datatype datatype, const char* name);
S4BXI_MPI_CALL(int, MPI_Type_get_name, MPI_Datatype datatype, char* name, int* len);

S4BXI_MPI_CALL(int, MPI_Pack, const void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount,
               int* position, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Pack_size, int incount, MPI_Datatype datatype, MPI_Comm comm, int* size);
S4BXI_MPI_CALL(int, MPI_Unpack, const void* inbuf, int insize, int* position, void* outbuf, int outcount,
               MPI_Datatype type, MPI_Comm comm);

S4BXI_MPI_CALL(int, MPI_Op_create, MPI_User_function* function, int commute, MPI_Op* op);
S4BXI_MPI_CALL(int, MPI_Op_free, MPI_Op* op);
S4BXI_MPI_CALL(int, MPI_Op_commutative, MPI_Op op, int* commute);
S4BXI_MPI_CALL(MPI_Op, MPI_Op_f2c, MPI_Fint op);
S4BXI_MPI_CALL(MPI_Fint, MPI_Op_c2f, MPI_Op op);

S4BXI_MPI_CALL(int, MPI_Group_free, MPI_Group* group);
S4BXI_MPI_CALL(int, MPI_Group_size, MPI_Group group, int* size);
S4BXI_MPI_CALL(int, MPI_Group_rank, MPI_Group group, int* rank);
S4BXI_MPI_CALL(int, MPI_Group_translate_ranks, MPI_Group group1, int n, const int* ranks1, MPI_Group group2,
               int* ranks2);
S4BXI_MPI_CALL(int, MPI_Group_compare, MPI_Group group1, MPI_Group group2, int* result);
S4BXI_MPI_CALL(int, MPI_Group_union, MPI_Group group1, MPI_Group group2, MPI_Group* newgroup);
S4BXI_MPI_CALL(int, MPI_Group_intersection, MPI_Group group1, MPI_Group group2, MPI_Group* newgroup);
S4BXI_MPI_CALL(int, MPI_Group_difference, MPI_Group group1, MPI_Group group2, MPI_Group* newgroup);
S4BXI_MPI_CALL(int, MPI_Group_incl, MPI_Group group, int n, const int* ranks, MPI_Group* newgroup);
S4BXI_MPI_CALL(int, MPI_Group_excl, MPI_Group group, int n, const int* ranks, MPI_Group* newgroup);
S4BXI_MPI_CALL(int, MPI_Group_range_incl, MPI_Group group, int n, int ranges[][3], MPI_Group* newgroup);
S4BXI_MPI_CALL(int, MPI_Group_range_excl, MPI_Group group, int n, int ranges[][3], MPI_Group* newgroup);
S4BXI_MPI_CALL(MPI_Group, MPI_Group_f2c, MPI_Fint group);
S4BXI_MPI_CALL(MPI_Fint, MPI_Group_c2f, MPI_Group group);

S4BXI_MPI_CALL(int, MPI_Comm_rank, MPI_Comm comm, int* rank);
S4BXI_MPI_CALL(int, MPI_Comm_size, MPI_Comm comm, int* size);
S4BXI_MPI_CALL(int, MPI_Comm_get_name, MPI_Comm comm, char* name, int* len);
S4BXI_MPI_CALL(int, MPI_Comm_set_name, MPI_Comm comm, const char* name);
S4BXI_MPI_CALL(int, MPI_Comm_dup, MPI_Comm comm, MPI_Comm* newcomm);
S4BXI_MPI_CALL(int, MPI_Comm_dup_with_info, MPI_Comm comm, MPI_Info info, MPI_Comm* newcomm);
S4BXI_MPI_CALL(int, MPI_Comm_get_attr, MPI_Comm comm, int comm_keyval, void* attribute_val, int* flag);
S4BXI_MPI_CALL(int, MPI_Comm_set_attr, MPI_Comm comm, int comm_keyval, void* attribute_val);
S4BXI_MPI_CALL(int, MPI_Comm_delete_attr, MPI_Comm comm, int comm_keyval);
S4BXI_MPI_CALL(int, MPI_Comm_create_keyval, MPI_Comm_copy_attr_function* copy_fn,
               MPI_Comm_delete_attr_function* delete_fn, int* keyval, void* extra_state);
S4BXI_MPI_CALL(int, MPI_Comm_free_keyval, int* keyval);
S4BXI_MPI_CALL(int, MPI_Comm_group, MPI_Comm comm, MPI_Group* group);
S4BXI_MPI_CALL(int, MPI_Comm_compare, MPI_Comm comm1, MPI_Comm comm2, int* result);
S4BXI_MPI_CALL(int, MPI_Comm_create, MPI_Comm comm, MPI_Group group, MPI_Comm* newcomm);
S4BXI_MPI_CALL(int, MPI_Comm_create_group, MPI_Comm comm, MPI_Group group, int tag, MPI_Comm* newcomm);
S4BXI_MPI_CALL(int, MPI_Comm_free, MPI_Comm* comm);
S4BXI_MPI_CALL(int, MPI_Comm_disconnect, MPI_Comm* comm);
S4BXI_MPI_CALL(int, MPI_Comm_split, MPI_Comm comm, int color, int key, MPI_Comm* comm_out);
S4BXI_MPI_CALL(int, MPI_Comm_set_info, MPI_Comm comm, MPI_Info info);
S4BXI_MPI_CALL(int, MPI_Comm_get_info, MPI_Comm comm, MPI_Info* info);
S4BXI_MPI_CALL(int, MPI_Comm_split_type, MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm* newcomm);
S4BXI_MPI_CALL(MPI_Comm, MPI_Comm_f2c, MPI_Fint comm);
S4BXI_MPI_CALL(MPI_Fint, MPI_Comm_c2f, MPI_Comm comm);

S4BXI_MPI_CALL(int, MPI_Start, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Startall, int count, MPI_Request* requests);
S4BXI_MPI_CALL(int, MPI_Request_free, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Recv, void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
               MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_Recv_init, void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
               MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Irecv, void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
               MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Send, const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Send_init, const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm,
               MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Isend, const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm,
               MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Ssend, const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Ssend_init, const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
               MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Issend, const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
               MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Bsend, const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Bsend_init, const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
               MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Ibsend, const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
               MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Rsend, const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Rsend_init, const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
               MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Irsend, const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
               MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Sendrecv, const void* sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag,
               void* recvbuf, int recvcount, MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm,
               MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_Sendrecv_replace, void* buf, int count, MPI_Datatype datatype, int dst, int sendtag, int src,
               int recvtag, MPI_Comm comm, MPI_Status* status);

S4BXI_MPI_CALL(int, MPI_Test, MPI_Request* request, int* flag, MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_Testany, int count, MPI_Request requests[], int* index, int* flag, MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_Testall, int count, MPI_Request* requests, int* flag, MPI_Status* statuses);
S4BXI_MPI_CALL(int, MPI_Testsome, int incount, MPI_Request requests[], int* outcount, int* indices,
               MPI_Status status[]);
S4BXI_MPI_CALL(int, MPI_Test_cancelled, const MPI_Status* status, int* flag);
S4BXI_MPI_CALL(int, MPI_Wait, MPI_Request* request, MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_Waitany, int count, MPI_Request requests[], int* index, MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_Waitall, int count, MPI_Request requests[], MPI_Status status[]);
S4BXI_MPI_CALL(int, MPI_Waitsome, int incount, MPI_Request requests[], int* outcount, int* indices,
               MPI_Status status[]);
S4BXI_MPI_CALL(int, MPI_Iprobe, int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_Probe, int source, int tag, MPI_Comm comm, MPI_Status* status);
S4BXI_MPI_CALL(MPI_Request, MPI_Request_f2c, MPI_Fint request);
S4BXI_MPI_CALL(MPI_Fint, MPI_Request_c2f, MPI_Request request);
S4BXI_MPI_CALL(int, MPI_Cancel, MPI_Request* request);

S4BXI_MPI_CALL(int, MPI_Bcast, void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Barrier, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Ibarrier, MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Ibcast, void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm,
               MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Igather, const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
               int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Igatherv, const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
               const int* recvcounts, const int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm,
               MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Iallgather, const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
               int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Iallgatherv, const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
               const int* recvcounts, const int* displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Iscatter, const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
               int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Iscatterv, const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype,
               void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Ireduce, const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
               int root, MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Iallreduce, const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
               MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Iscan, const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
               MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Iexscan, const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
               MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Ireduce_scatter, const void* sendbuf, void* recvbuf, const int* recvcounts,
               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Ireduce_scatter_block, const void* sendbuf, void* recvbuf, int recvcount, MPI_Datatype datatype,
               MPI_Op op, MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Ialltoall, const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
               int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Ialltoallv, const void* sendbuf, const int* sendcounts, const int* senddisps,
               MPI_Datatype sendtype, void* recvbuf, const int* recvcounts, const int* recvdisps, MPI_Datatype recvtype,
               MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Ialltoallw, const void* sendbuf, const int* sendcounts, const int* senddisps,
               const MPI_Datatype* sendtypes, void* recvbuf, const int* recvcounts, const int* recvdisps,
               const MPI_Datatype* recvtypes, MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Gather, const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
               MPI_Datatype recvtype, int root, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Gatherv, const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
               const int* recvcounts, const int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Allgather, const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
               int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Allgatherv, const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
               const int* recvcounts, const int* displs, MPI_Datatype recvtype, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Scatter, const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
               int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Scatterv, const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype,
               void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Reduce, const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
               int root, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Allreduce, const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
               MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Scan, const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
               MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Exscan, const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
               MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Reduce_scatter, const void* sendbuf, void* recvbuf, const int* recvcounts,
               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Reduce_scatter_block, const void* sendbuf, void* recvbuf, int recvcount, MPI_Datatype datatype,
               MPI_Op op, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Alltoall, const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
               int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Alltoallv, const void* sendbuf, const int* sendcounts, const int* senddisps,
               MPI_Datatype sendtype, void* recvbuf, const int* recvcounts, const int* recvdisps, MPI_Datatype recvtype,
               MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Alltoallw, const void* sendbuf, const int* sendcnts, const int* sdispls,
               const MPI_Datatype* sendtypes, void* recvbuf, const int* recvcnts, const int* rdispls,
               const MPI_Datatype* recvtypes, MPI_Comm comm);
S4BXI_MPI_CALL(int, MPI_Reduce_local, const void* inbuf, void* inoutbuf, int count, MPI_Datatype datatype, MPI_Op op);

S4BXI_MPI_CALL(int, MPI_Info_create, MPI_Info* info);
S4BXI_MPI_CALL(int, MPI_Info_set, MPI_Info info, const char* key, const char* value);
S4BXI_MPI_CALL(int, MPI_Info_get, MPI_Info info, const char* key, int valuelen, char* value, int* flag);
S4BXI_MPI_CALL(int, MPI_Info_free, MPI_Info* info);
S4BXI_MPI_CALL(int, MPI_Info_delete, MPI_Info info, const char* key);
S4BXI_MPI_CALL(int, MPI_Info_dup, MPI_Info info, MPI_Info* newinfo);
S4BXI_MPI_CALL(int, MPI_Info_get_nkeys, MPI_Info info, int* nkeys);
S4BXI_MPI_CALL(int, MPI_Info_get_nthkey, MPI_Info info, int n, char* key);
S4BXI_MPI_CALL(int, MPI_Info_get_valuelen, MPI_Info info, const char* key, int* valuelen, int* flag);
S4BXI_MPI_CALL(MPI_Info, MPI_Info_f2c, MPI_Fint info);
S4BXI_MPI_CALL(MPI_Fint, MPI_Info_c2f, MPI_Info info);

S4BXI_MPI_CALL(int, MPI_Win_free, MPI_Win* win);
S4BXI_MPI_CALL(int, MPI_Win_create, void* base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
               MPI_Win* win);
S4BXI_MPI_CALL(int, MPI_Win_allocate, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void* base,
               MPI_Win* win);
S4BXI_MPI_CALL(int, MPI_Win_allocate_shared, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void* base,
               MPI_Win* win);
S4BXI_MPI_CALL(int, MPI_Win_create_dynamic, MPI_Info info, MPI_Comm comm, MPI_Win* win);
S4BXI_MPI_CALL(int, MPI_Win_attach, MPI_Win win, void* base, MPI_Aint size);
S4BXI_MPI_CALL(int, MPI_Win_detach, MPI_Win win, const void* base);
S4BXI_MPI_CALL(int, MPI_Win_set_name, MPI_Win win, const char* name);
S4BXI_MPI_CALL(int, MPI_Win_get_name, MPI_Win win, char* name, int* len);
S4BXI_MPI_CALL(int, MPI_Win_set_info, MPI_Win win, MPI_Info info);
S4BXI_MPI_CALL(int, MPI_Win_get_info, MPI_Win win, MPI_Info* info);
S4BXI_MPI_CALL(int, MPI_Win_get_group, MPI_Win win, MPI_Group* group);
S4BXI_MPI_CALL(int, MPI_Win_fence, int assert, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_get_attr, MPI_Win type, int type_keyval, void* attribute_val, int* flag);
S4BXI_MPI_CALL(int, MPI_Win_set_attr, MPI_Win type, int type_keyval, void* att);
S4BXI_MPI_CALL(int, MPI_Win_delete_attr, MPI_Win type, int comm_keyval);
S4BXI_MPI_CALL(int, MPI_Win_create_keyval, MPI_Win_copy_attr_function* copy_fn, MPI_Win_delete_attr_function* delete_fn,
               int* keyval, void* extra_state);
S4BXI_MPI_CALL(int, MPI_Win_free_keyval, int* keyval);
S4BXI_MPI_CALL(int, MPI_Win_complete, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_post, MPI_Group group, int assert, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_start, MPI_Group group, int assert, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_wait, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_lock, int lock_type, int rank, int assert, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_lock_all, int assert, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_unlock, int rank, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_unlock_all, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_flush, int rank, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_flush_local, int rank, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_flush_all, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_flush_local_all, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Win_shared_query, MPI_Win win, int rank, MPI_Aint* size, int* disp_unit, void* baseptr);
S4BXI_MPI_CALL(int, MPI_Win_sync, MPI_Win win);
S4BXI_MPI_CALL(MPI_Win, MPI_Win_f2c, MPI_Fint win);
S4BXI_MPI_CALL(MPI_Fint, MPI_Win_c2f, MPI_Win win);

S4BXI_MPI_CALL(int, MPI_Get, void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
               MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Put, const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
               MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Accumulate, const void* origin_addr, int origin_count, MPI_Datatype origin_datatype,
               int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op,
               MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Get_accumulate, const void* origin_addr, int origin_count, MPI_Datatype origin_datatype,
               void* result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
               int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Rget, void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
               MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Rput, const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
               MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Raccumulate, const void* origin_addr, int origin_count, MPI_Datatype origin_datatype,
               int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op,
               MPI_Win win, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Rget_accumulate, const void* origin_addr, int origin_count, MPI_Datatype origin_datatype,
               void* result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
               int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Fetch_and_op, const void* origin_addr, void* result_addr, MPI_Datatype datatype,
               int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win);
S4BXI_MPI_CALL(int, MPI_Compare_and_swap, const void* origin_addr, void* compare_addr, void* result_addr,
               MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Win win);

S4BXI_MPI_CALL(int, MPI_Cart_coords, MPI_Comm comm, int rank, int maxdims, int* coords);
S4BXI_MPI_CALL(int, MPI_Cart_create, MPI_Comm comm_old, int ndims, const int* dims, const int* periods, int reorder,
               MPI_Comm* comm_cart);
S4BXI_MPI_CALL(int, MPI_Cart_get, MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords);
S4BXI_MPI_CALL(int, MPI_Cart_rank, MPI_Comm comm, const int* coords, int* rank);
S4BXI_MPI_CALL(int, MPI_Cart_shift, MPI_Comm comm, int direction, int displ, int* source, int* dest);
S4BXI_MPI_CALL(int, MPI_Cart_sub, MPI_Comm comm, const int* remain_dims, MPI_Comm* comm_new);
S4BXI_MPI_CALL(int, MPI_Cartdim_get, MPI_Comm comm, int* ndims);
S4BXI_MPI_CALL(int, MPI_Dims_create, int nnodes, int ndims, int* dims);
S4BXI_MPI_CALL(int, MPI_Request_get_status, MPI_Request request, int* flag, MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_Grequest_start, MPI_Grequest_query_function* query_fn, MPI_Grequest_free_function* free_fn,
               MPI_Grequest_cancel_function* cancel_fn, void* extra_state, MPI_Request* request);
S4BXI_MPI_CALL(int, MPI_Grequest_complete, MPI_Request request);
S4BXI_MPI_CALL(int, MPI_Status_set_cancelled, MPI_Status* status, int flag);
S4BXI_MPI_CALL(int, MPI_Status_set_elements, MPI_Status* status, MPI_Datatype datatype, int count);
S4BXI_MPI_CALL(int, MPI_Status_set_elements_x, MPI_Status* status, MPI_Datatype datatype, MPI_Count count);
S4BXI_MPI_CALL(int, MPI_Type_create_subarray, int ndims, const int* array_of_sizes, const int* array_of_subsizes,
               const int* array_of_starts, int order, MPI_Datatype oldtype, MPI_Datatype* newtype);

S4BXI_MPI_CALL(int, MPI_File_open, MPI_Comm comm, const char* filename, int amode, MPI_Info info, MPI_File* fh);
S4BXI_MPI_CALL(int, MPI_File_close, MPI_File* fh);
S4BXI_MPI_CALL(int, MPI_File_delete, const char* filename, MPI_Info info);
S4BXI_MPI_CALL(int, MPI_File_get_size, MPI_File fh, MPI_Offset* size);
S4BXI_MPI_CALL(int, MPI_File_get_group, MPI_File fh, MPI_Group* group);
S4BXI_MPI_CALL(int, MPI_File_get_amode, MPI_File fh, int* amode);
S4BXI_MPI_CALL(int, MPI_File_set_info, MPI_File fh, MPI_Info info);
S4BXI_MPI_CALL(int, MPI_File_get_info, MPI_File fh, MPI_Info* info_used);
S4BXI_MPI_CALL(int, MPI_File_read_at, MPI_File fh, MPI_Offset offset, void* buf, int count, MPI_Datatype datatype,
               MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_File_read_at_all, MPI_File fh, MPI_Offset offset, void* buf, int count, MPI_Datatype datatype,
               MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_File_write_at, MPI_File fh, MPI_Offset offset, const void* buf, int count,
               MPI_Datatype datatype, MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_File_write_at_all, MPI_File fh, MPI_Offset offset, const void* buf, int count,
               MPI_Datatype datatype, MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_File_read, MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_File_read_all, MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_File_write, MPI_File fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_File_write_all, MPI_File fh, const void* buf, int count, MPI_Datatype datatype,
               MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_File_seek, MPI_File fh, MPI_Offset offset, int whenace);
S4BXI_MPI_CALL(int, MPI_File_get_position, MPI_File fh, MPI_Offset* offset);
S4BXI_MPI_CALL(int, MPI_File_read_shared, MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_File_write_shared, MPI_File fh, const void* buf, int count, MPI_Datatype datatype,
               MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_File_read_ordered, MPI_File fh, void* buf, int count, MPI_Datatype datatype,
               MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_File_write_ordered, MPI_File fh, const void* buf, int count, MPI_Datatype datatype,
               MPI_Status* status);
S4BXI_MPI_CALL(int, MPI_File_seek_shared, MPI_File fh, MPI_Offset offset, int whence);
S4BXI_MPI_CALL(int, MPI_File_get_position_shared, MPI_File fh, MPI_Offset* offset);
S4BXI_MPI_CALL(int, MPI_File_sync, MPI_File fh);
S4BXI_MPI_CALL(int, MPI_File_set_view, MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype,
               const char* datarep, MPI_Info info);
S4BXI_MPI_CALL(int, MPI_File_get_view, MPI_File fh, MPI_Offset* disp, MPI_Datatype* etype, MPI_Datatype* filetype,
               char* datarep);

S4BXI_MPI_CALL(int, MPI_Errhandler_set, MPI_Comm comm, MPI_Errhandler errhandler);
S4BXI_MPI_CALL(int, MPI_Errhandler_create, MPI_Handler_function* function, MPI_Errhandler* errhandler);
S4BXI_MPI_CALL(int, MPI_Errhandler_free, MPI_Errhandler* errhandler);
S4BXI_MPI_CALL(int, MPI_Errhandler_get, MPI_Comm comm, MPI_Errhandler* errhandler);
S4BXI_MPI_CALL(int, MPI_Comm_set_errhandler, MPI_Comm comm, MPI_Errhandler errhandler);
S4BXI_MPI_CALL(int, MPI_Comm_get_errhandler, MPI_Comm comm, MPI_Errhandler* errhandler);
S4BXI_MPI_CALL(int, MPI_Comm_create_errhandler, MPI_Comm_errhandler_fn* function, MPI_Errhandler* errhandler);
S4BXI_MPI_CALL(int, MPI_Comm_call_errhandler, MPI_Comm comm, int errorcode);
S4BXI_MPI_CALL(int, MPI_Win_set_errhandler, MPI_Win win, MPI_Errhandler errhandler);
S4BXI_MPI_CALL(int, MPI_Win_get_errhandler, MPI_Win win, MPI_Errhandler* errhandler);
S4BXI_MPI_CALL(int, MPI_Win_create_errhandler, MPI_Win_errhandler_fn* function, MPI_Errhandler* errhandler);
S4BXI_MPI_CALL(int, MPI_Win_call_errhandler, MPI_Win win, int errorcode);
S4BXI_MPI_CALL(MPI_Errhandler, MPI_Errhandler_f2c, MPI_Fint errhandler);
S4BXI_MPI_CALL(MPI_Fint, MPI_Errhandler_c2f, MPI_Errhandler errhandler);

S4BXI_MPI_CALL(int, MPI_Type_create_f90_integer, int count, MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_create_f90_real, int prec, int exp, MPI_Datatype* newtype);
S4BXI_MPI_CALL(int, MPI_Type_create_f90_complex, int prec, int exp, MPI_Datatype* newtype);

S4BXI_MPI_CALL(int, MPI_Type_get_contents, MPI_Datatype datatype, int max_integers, int max_addresses,
               int max_datatypes, int* array_of_integers, MPI_Aint* array_of_addresses,
               MPI_Datatype* array_of_datatypes);
S4BXI_MPI_CALL(int, MPI_Type_get_envelope, MPI_Datatype datatype, int* num_integers, int* num_addresses,
               int* num_datatypes, int* combiner);
S4BXI_MPI_CALL(int, MPI_File_call_errhandler, MPI_File fh, int errorcode);
S4BXI_MPI_CALL(int, MPI_File_create_errhandler, MPI_File_errhandler_function* function, MPI_Errhandler* errhandler);
S4BXI_MPI_CALL(int, MPI_File_set_errhandler, MPI_File file, MPI_Errhandler errhandler);
S4BXI_MPI_CALL(int, MPI_File_get_errhandler, MPI_File file, MPI_Errhandler* errhandler);

#undef S4BXI_MPI_CALL

#ifdef __cplusplus
}
#endif

#ifndef COMPILING_SIMULATOR

#ifdef MPI_IN_PLACE
// The only reason for this undef is to avoid stupid compiler warnings
#undef MPI_IN_PLACE
#endif
#define MPI_IN_PLACE (S4BXI_MPI_IN_PLACE())

#ifdef MPI_REQUEST_NULL
#undef MPI_REQUEST_NULL
#endif
#define MPI_REQUEST_NULL (S4BXI_MPI_REQUEST_NULL())

#define MPI_Init(...)                       S4BXI_MPI_Init(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Finalize()                      S4BXI_MPI_Finalize(__FILE__, __LINE__)
#define MPI_Finalized(...)                  S4BXI_MPI_Finalized(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Init_thread(...)                S4BXI_MPI_Init_thread(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Initialized(...)                S4BXI_MPI_Initialized(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Query_thread(...)               S4BXI_MPI_Query_thread(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Is_thread_main(...)             S4BXI_MPI_Is_thread_main(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Get_version(...)                S4BXI_MPI_Get_version(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Get_library_version(...)        S4BXI_MPI_Get_library_version(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Get_processor_name(...)         S4BXI_MPI_Get_processor_name(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Abort(...)                      S4BXI_MPI_Abort(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Alloc_mem(...)                  S4BXI_MPI_Alloc_mem(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Free_mem(...)                   S4BXI_MPI_Free_mem(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Wtime()                         S4BXI_MPI_Wtime(__FILE__, __LINE__)
#define MPI_Wtick(...)                      S4BXI_MPI_Wtick(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Buffer_attach(...)              S4BXI_MPI_Buffer_attach(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Buffer_detach(...)              S4BXI_MPI_Buffer_detach(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Address(...)                    S4BXI_MPI_Address(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Get_address(...)                S4BXI_MPI_Get_address(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Aint_diff(...)                  S4BXI_MPI_Aint_diff(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Aint_add(...)                   S4BXI_MPI_Aint_add(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Error_class(...)                S4BXI_MPI_Error_class(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Error_string(...)               S4BXI_MPI_Error_string(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Attr_delete(...)                S4BXI_MPI_Attr_delete(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Attr_get(...)                   S4BXI_MPI_Attr_get(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Attr_put(...)                   S4BXI_MPI_Attr_put(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Keyval_create(...)              S4BXI_MPI_Keyval_create(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Keyval_free(...)                S4BXI_MPI_Keyval_free(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_free(...)                  S4BXI_MPI_Type_free(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_size(...)                  S4BXI_MPI_Type_size(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_size_x(...)                S4BXI_MPI_Type_size_x(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_get_extent(...)            S4BXI_MPI_Type_get_extent(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_get_extent_x(...)          S4BXI_MPI_Type_get_extent_x(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_get_true_extent(...)       S4BXI_MPI_Type_get_true_extent(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_get_true_extent_x(...)     S4BXI_MPI_Type_get_true_extent_x(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_extent(...)                S4BXI_MPI_Type_extent(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_lb(...)                    S4BXI_MPI_Type_lb(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_ub(...)                    S4BXI_MPI_Type_ub(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_commit(...)                S4BXI_MPI_Type_commit(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_hindexed(...)              S4BXI_MPI_Type_hindexed(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_create_hindexed(...)       S4BXI_MPI_Type_create_hindexed(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_create_hindexed_block(...) S4BXI_MPI_Type_create_hindexed_block(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_hvector(...)               S4BXI_MPI_Type_hvector(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_create_hvector(...)        S4BXI_MPI_Type_create_hvector(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_indexed(...)               S4BXI_MPI_Type_indexed(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_create_indexed(...)        S4BXI_MPI_Type_create_indexed(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_create_indexed_block(...)  S4BXI_MPI_Type_create_indexed_block(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_struct(...)                S4BXI_MPI_Type_struct(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_create_struct(...)         S4BXI_MPI_Type_create_struct(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_vector(...)                S4BXI_MPI_Type_vector(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_contiguous(...)            S4BXI_MPI_Type_contiguous(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_create_resized(...)        S4BXI_MPI_Type_create_resized(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_f2c(...)                   S4BXI_MPI_Type_f2c(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_c2f(...)                   S4BXI_MPI_Type_c2f(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Get_count(...)                  S4BXI_MPI_Get_count(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_get_attr(...)              S4BXI_MPI_Type_get_attr(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_set_attr(...)              S4BXI_MPI_Type_set_attr(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_delete_attr(...)           S4BXI_MPI_Type_delete_attr(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_create_keyval(...)         S4BXI_MPI_Type_create_keyval(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_free_keyval(...)           S4BXI_MPI_Type_free_keyval(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_dup(...)                   S4BXI_MPI_Type_dup(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_set_name(...)              S4BXI_MPI_Type_set_name(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_get_name(...)              S4BXI_MPI_Type_get_name(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Pack(...)                       S4BXI_MPI_Pack(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Pack_size(...)                  S4BXI_MPI_Pack_size(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Unpack(...)                     S4BXI_MPI_Unpack(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Op_create(...)                  S4BXI_MPI_Op_create(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Op_free(...)                    S4BXI_MPI_Op_free(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Op_commutative(...)             S4BXI_MPI_Op_commutative(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Op_f2c(...)                     S4BXI_MPI_Op_f2c(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Op_c2f(...)                     S4BXI_MPI_Op_c2f(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_free(...)                 S4BXI_MPI_Group_free(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_size(...)                 S4BXI_MPI_Group_size(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_rank(...)                 S4BXI_MPI_Group_rank(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_translate_ranks(...)      S4BXI_MPI_Group_translate_ranks(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_compare(...)              S4BXI_MPI_Group_compare(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_union(...)                S4BXI_MPI_Group_union(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_intersection(...)         S4BXI_MPI_Group_intersection(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_difference(...)           S4BXI_MPI_Group_difference(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_incl(...)                 S4BXI_MPI_Group_incl(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_excl(...)                 S4BXI_MPI_Group_excl(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_range_incl(...)           S4BXI_MPI_Group_range_incl(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_range_excl(...)           S4BXI_MPI_Group_range_excl(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_f2c(...)                  S4BXI_MPI_Group_f2c(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Group_c2f(...)                  S4BXI_MPI_Group_c2f(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_rank(...)                  S4BXI_MPI_Comm_rank(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_size(...)                  S4BXI_MPI_Comm_size(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_get_name(...)              S4BXI_MPI_Comm_get_name(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_set_name(...)              S4BXI_MPI_Comm_set_name(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_dup(...)                   S4BXI_MPI_Comm_dup(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_dup_with_info(...)         S4BXI_MPI_Comm_dup_with_info(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_get_attr(...)              S4BXI_MPI_Comm_get_attr(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_set_attr(...)              S4BXI_MPI_Comm_set_attr(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_delete_attr(...)           S4BXI_MPI_Comm_delete_attr(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_create_keyval(...)         S4BXI_MPI_Comm_create_keyval(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_free_keyval(...)           S4BXI_MPI_Comm_free_keyval(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_group(...)                 S4BXI_MPI_Comm_group(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_compare(...)               S4BXI_MPI_Comm_compare(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_create(...)                S4BXI_MPI_Comm_create(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_create_group(...)          S4BXI_MPI_Comm_create_group(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_free(...)                  S4BXI_MPI_Comm_free(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_disconnect(...)            S4BXI_MPI_Comm_disconnect(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_split(...)                 S4BXI_MPI_Comm_split(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_set_info(...)              S4BXI_MPI_Comm_set_info(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_get_info(...)              S4BXI_MPI_Comm_get_info(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_split_type(...)            S4BXI_MPI_Comm_split_type(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_f2c(...)                   S4BXI_MPI_Comm_f2c(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_c2f(...)                   S4BXI_MPI_Comm_c2f(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Start(...)                      S4BXI_MPI_Start(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Startall(...)                   S4BXI_MPI_Startall(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Request_free(...)               S4BXI_MPI_Request_free(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Recv(...)                       S4BXI_MPI_Recv(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Recv_init(...)                  S4BXI_MPI_Recv_init(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Irecv(...)                      S4BXI_MPI_Irecv(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Send(...)                       S4BXI_MPI_Send(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Send_init(...)                  S4BXI_MPI_Send_init(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Isend(...)                      S4BXI_MPI_Isend(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Ssend(...)                      S4BXI_MPI_Ssend(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Ssend_init(...)                 S4BXI_MPI_Ssend_init(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Issend(...)                     S4BXI_MPI_Issend(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Bsend(...)                      S4BXI_MPI_Bsend(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Bsend_init(...)                 S4BXI_MPI_Bsend_init(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Ibsend(...)                     S4BXI_MPI_Ibsend(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Rsend(...)                      S4BXI_MPI_Rsend(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Rsend_init(...)                 S4BXI_MPI_Rsend_init(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Irsend(...)                     S4BXI_MPI_Irsend(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Sendrecv(...)                   S4BXI_MPI_Sendrecv(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Sendrecv_replace(...)           S4BXI_MPI_Sendrecv_replace(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Test(...)                       S4BXI_MPI_Test(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Testany(...)                    S4BXI_MPI_Testany(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Testall(...)                    S4BXI_MPI_Testall(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Testsome(...)                   S4BXI_MPI_Testsome(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Test_cancelled(...)             S4BXI_MPI_Test_cancelled(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Wait(...)                       S4BXI_MPI_Wait(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Waitany(...)                    S4BXI_MPI_Waitany(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Waitall(...)                    S4BXI_MPI_Waitall(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Waitsome(...)                   S4BXI_MPI_Waitsome(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Iprobe(...)                     S4BXI_MPI_Iprobe(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Probe(...)                      S4BXI_MPI_Probe(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Request_f2c(...)                S4BXI_MPI_Request_f2c(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Request_c2f(...)                S4BXI_MPI_Request_c2f(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Cancel(...)                     S4BXI_MPI_Cancel(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Bcast(...)                      S4BXI_MPI_Bcast(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Barrier(...)                    S4BXI_MPI_Barrier(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Ibarrier(...)                   S4BXI_MPI_Ibarrier(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Ibcast(...)                     S4BXI_MPI_Ibcast(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Igather(...)                    S4BXI_MPI_Igather(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Igatherv(...)                   S4BXI_MPI_Igatherv(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Iallgather(...)                 S4BXI_MPI_Iallgather(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Iallgatherv(...)                S4BXI_MPI_Iallgatherv(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Iscatter(...)                   S4BXI_MPI_Iscatter(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Iscatterv(...)                  S4BXI_MPI_Iscatterv(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Ireduce(...)                    S4BXI_MPI_Ireduce(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Iallreduce(...)                 S4BXI_MPI_Iallreduce(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Iscan(...)                      S4BXI_MPI_Iscan(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Iexscan(...)                    S4BXI_MPI_Iexscan(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Ireduce_scatter(...)            S4BXI_MPI_Ireduce_scatter(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Ireduce_scatter_block(...)      S4BXI_MPI_Ireduce_scatter_block(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Ialltoall(...)                  S4BXI_MPI_Ialltoall(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Ialltoallv(...)                 S4BXI_MPI_Ialltoallv(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Ialltoallw(...)                 S4BXI_MPI_Ialltoallw(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Gather(...)                     S4BXI_MPI_Gather(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Gatherv(...)                    S4BXI_MPI_Gatherv(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Allgather(...)                  S4BXI_MPI_Allgather(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Allgatherv(...)                 S4BXI_MPI_Allgatherv(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Scatter(...)                    S4BXI_MPI_Scatter(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Scatterv(...)                   S4BXI_MPI_Scatterv(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Reduce(...)                     S4BXI_MPI_Reduce(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Allreduce(...)                  S4BXI_MPI_Allreduce(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Scan(...)                       S4BXI_MPI_Scan(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Exscan(...)                     S4BXI_MPI_Exscan(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Reduce_scatter(...)             S4BXI_MPI_Reduce_scatter(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Reduce_scatter_block(...)       S4BXI_MPI_Reduce_scatter_block(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Alltoall(...)                   S4BXI_MPI_Alltoall(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Alltoallv(...)                  S4BXI_MPI_Alltoallv(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Alltoallw(...)                  S4BXI_MPI_Alltoallw(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Reduce_local(...)               S4BXI_MPI_Reduce_local(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Info_create(...)                S4BXI_MPI_Info_create(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Info_set(...)                   S4BXI_MPI_Info_set(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Info_get(...)                   S4BXI_MPI_Info_get(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Info_free(...)                  S4BXI_MPI_Info_free(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Info_delete(...)                S4BXI_MPI_Info_delete(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Info_dup(...)                   S4BXI_MPI_Info_dup(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Info_get_nkeys(...)             S4BXI_MPI_Info_get_nkeys(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Info_get_nthkey(...)            S4BXI_MPI_Info_get_nthkey(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Info_get_valuelen(...)          S4BXI_MPI_Info_get_valuelen(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Info_f2c(...)                   S4BXI_MPI_Info_f2c(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Info_c2f(...)                   S4BXI_MPI_Info_c2f(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_free(...)                   S4BXI_MPI_Win_free(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_create(...)                 S4BXI_MPI_Win_create(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_allocate(...)               S4BXI_MPI_Win_allocate(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_allocate_shared(...)        S4BXI_MPI_Win_allocate_shared(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_create_dynamic(...)         S4BXI_MPI_Win_create_dynamic(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_attach(...)                 S4BXI_MPI_Win_attach(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_detach(...)                 S4BXI_MPI_Win_detach(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_set_name(...)               S4BXI_MPI_Win_set_name(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_get_name(...)               S4BXI_MPI_Win_get_name(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_set_info(...)               S4BXI_MPI_Win_set_info(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_get_info(...)               S4BXI_MPI_Win_get_info(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_get_group(...)              S4BXI_MPI_Win_get_group(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_fence(...)                  S4BXI_MPI_Win_fence(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_get_attr(...)               S4BXI_MPI_Win_get_attr(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_set_attr(...)               S4BXI_MPI_Win_set_attr(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_delete_attr(...)            S4BXI_MPI_Win_delete_attr(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_create_keyval(...)          S4BXI_MPI_Win_create_keyval(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_free_keyval(...)            S4BXI_MPI_Win_free_keyval(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_complete(...)               S4BXI_MPI_Win_complete(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_post(...)                   S4BXI_MPI_Win_post(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_start(...)                  S4BXI_MPI_Win_start(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_wait(...)                   S4BXI_MPI_Win_wait(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_lock(...)                   S4BXI_MPI_Win_lock(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_lock_all(...)               S4BXI_MPI_Win_lock_all(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_unlock(...)                 S4BXI_MPI_Win_unlock(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_unlock_all(...)             S4BXI_MPI_Win_unlock_all(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_flush(...)                  S4BXI_MPI_Win_flush(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_flush_local(...)            S4BXI_MPI_Win_flush_local(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_flush_all(...)              S4BXI_MPI_Win_flush_all(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_flush_local_all(...)        S4BXI_MPI_Win_flush_local_all(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_shared_query(...)           S4BXI_MPI_Win_shared_query(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_sync(...)                   S4BXI_MPI_Win_sync(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_f2c(...)                    S4BXI_MPI_Win_f2c(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_c2f(...)                    S4BXI_MPI_Win_c2f(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Get(...)                        S4BXI_MPI_Get(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Put(...)                        S4BXI_MPI_Put(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Accumulate(...)                 S4BXI_MPI_Accumulate(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Get_accumulate(...)             S4BXI_MPI_Get_accumulate(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Rget(...)                       S4BXI_MPI_Rget(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Rput(...)                       S4BXI_MPI_Rput(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Raccumulate(...)                S4BXI_MPI_Raccumulate(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Rget_accumulate(...)            S4BXI_MPI_Rget_accumulate(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Fetch_and_op(...)               S4BXI_MPI_Fetch_and_op(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Compare_and_swap(...)           S4BXI_MPI_Compare_and_swap(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Cart_coords(...)                S4BXI_MPI_Cart_coords(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Cart_create(...)                S4BXI_MPI_Cart_create(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Cart_get(...)                   S4BXI_MPI_Cart_get(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Cart_rank(...)                  S4BXI_MPI_Cart_rank(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Cart_shift(...)                 S4BXI_MPI_Cart_shift(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Cart_sub(...)                   S4BXI_MPI_Cart_sub(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Cartdim_get(...)                S4BXI_MPI_Cartdim_get(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Dims_create(...)                S4BXI_MPI_Dims_create(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Request_get_status(...)         S4BXI_MPI_Request_get_status(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Grequest_start(...)             S4BXI_MPI_Grequest_start(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Grequest_complete(...)          S4BXI_MPI_Grequest_complete(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Status_set_cancelled(...)       S4BXI_MPI_Status_set_cancelled(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Status_set_elements(...)        S4BXI_MPI_Status_set_elements(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Status_set_elements_x(...)      S4BXI_MPI_Status_set_elements_x(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_create_subarray(...)       S4BXI_MPI_Type_create_subarray(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_open(...)                  S4BXI_MPI_File_open(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_close(...)                 S4BXI_MPI_File_close(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_delete(...)                S4BXI_MPI_File_delete(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_get_size(...)              S4BXI_MPI_File_get_size(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_get_group(...)             S4BXI_MPI_File_get_group(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_get_amode(...)             S4BXI_MPI_File_get_amode(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_set_info(...)              S4BXI_MPI_File_set_info(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_get_info(...)              S4BXI_MPI_File_get_info(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_read_at(...)               S4BXI_MPI_File_read_at(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_read_at_all(...)           S4BXI_MPI_File_read_at_all(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_write_at(...)              S4BXI_MPI_File_write_at(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_write_at_all(...)          S4BXI_MPI_File_write_at_all(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_read(...)                  S4BXI_MPI_File_read(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_read_all(...)              S4BXI_MPI_File_read_all(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_write(...)                 S4BXI_MPI_File_write(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_write_all(...)             S4BXI_MPI_File_write_all(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_seek(...)                  S4BXI_MPI_File_seek(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_get_position(...)          S4BXI_MPI_File_get_position(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_read_shared(...)           S4BXI_MPI_File_read_shared(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_write_shared(...)          S4BXI_MPI_File_write_shared(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_read_ordered(...)          S4BXI_MPI_File_read_ordered(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_write_ordered(...)         S4BXI_MPI_File_write_ordered(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_seek_shared(...)           S4BXI_MPI_File_seek_shared(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_get_position_shared(...)   S4BXI_MPI_File_get_position_shared(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_sync(...)                  S4BXI_MPI_File_sync(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_set_view(...)              S4BXI_MPI_File_set_view(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_get_view(...)              S4BXI_MPI_File_get_view(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Errhandler_set(...)             S4BXI_MPI_Errhandler_set(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Errhandler_create(...)          S4BXI_MPI_Errhandler_create(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Errhandler_free(...)            S4BXI_MPI_Errhandler_free(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Errhandler_get(...)             S4BXI_MPI_Errhandler_get(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_set_errhandler(...)        S4BXI_MPI_Comm_set_errhandler(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_get_errhandler(...)        S4BXI_MPI_Comm_get_errhandler(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_create_errhandler(...)     S4BXI_MPI_Comm_create_errhandler(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Comm_call_errhandler(...)       S4BXI_MPI_Comm_call_errhandler(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_set_errhandler(...)         S4BXI_MPI_Win_set_errhandler(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_get_errhandler(...)         S4BXI_MPI_Win_get_errhandler(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_create_errhandler(...)      S4BXI_MPI_Win_create_errhandler(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Win_call_errhandler(...)        S4BXI_MPI_Win_call_errhandler(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Errhandler_f2c(...)             S4BXI_MPI_Errhandler_f2c(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Errhandler_c2f(...)             S4BXI_MPI_Errhandler_c2f(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_create_f90_integer(...)    S4BXI_MPI_Type_create_f90_integer(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_create_f90_real(...)       S4BXI_MPI_Type_create_f90_real(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_create_f90_complex(...)    S4BXI_MPI_Type_create_f90_complex(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_get_contents(...)          S4BXI_MPI_Type_get_contents(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_Type_get_envelope(...)          S4BXI_MPI_Type_get_envelope(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_call_errhandler(...)       S4BXI_MPI_File_call_errhandler(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_create_errhandler(...)     S4BXI_MPI_File_create_errhandler(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_set_errhandler(...)        S4BXI_MPI_File_set_errhandler(__FILE__, __LINE__, ##__VA_ARGS__)
#define MPI_File_get_errhandler(...)        S4BXI_MPI_File_get_errhandler(__FILE__, __LINE__, ##__VA_ARGS__)

// ================================================================================================================
// SMPI stuff, copy/pasted because all of this is defined in smpi.h and not in a specific file, and I don't want to
// include it all the time
// ================================================================================================================

/**
 * Functions for call location tracing. These functions will be
 * called from the user's application! (With the __FILE__ and __LINE__ values
 * passed as parameters.)
 */
void smpi_trace_set_call_location(const char* file, int line);
/** Fortran binding **/
void smpi_trace_set_call_location_(const char* file, const int* line);
/** Fortran binding + -fsecond-underscore **/
void smpi_trace_set_call_location__(const char* file, const int* line);

#define SMPI_ITER_NAME1(line) _XBT_CONCAT(iter_count, line)
#define SMPI_ITER_NAME(line)  SMPI_ITER_NAME1(line)
#define SMPI_CTAG_NAME1(line) _XBT_CONCAT(ctag, line)
#define SMPI_CTAG_NAME(line)  SMPI_CTAG_NAME1(line)

#define SMPI_SAMPLE_LOOP(loop_init, loop_end, loop_iter, global, iters, thres, tag)                                    \
    char SMPI_CTAG_NAME(__LINE__)[132];                                                                                \
    snprintf(SMPI_CTAG_NAME(__LINE__), 132, "%s%d", tag, __LINE__);                                                    \
    int SMPI_ITER_NAME(__LINE__) = 0;                                                                                  \
    {                                                                                                                  \
        loop_init;                                                                                                     \
        while (loop_end) {                                                                                             \
            SMPI_ITER_NAME(__LINE__)++;                                                                                \
            (loop_iter);                                                                                               \
        }                                                                                                              \
    }                                                                                                                  \
    for (loop_init;                                                                                                    \
         (loop_end) ? (smpi_sample_1((global), __FILE__, SMPI_CTAG_NAME(__LINE__), (iters), (thres)),                  \
                       (smpi_sample_2((global), __FILE__, SMPI_CTAG_NAME(__LINE__), SMPI_ITER_NAME(__LINE__))))        \
                    : smpi_sample_exit((global), __FILE__, SMPI_CTAG_NAME(__LINE__), SMPI_ITER_NAME(__LINE__));        \
         smpi_sample_3((global), __FILE__, SMPI_CTAG_NAME(__LINE__)), (loop_iter))

#define SMPI_SAMPLE_LOCAL(loop_init, loop_end, loop_iter, iters, thres)                                                \
    SMPI_SAMPLE_LOOP(loop_init, (loop_end), (loop_iter), 0, (iters), (thres), "")
#define SMPI_SAMPLE_LOCAL_TAG(loop_init, loop_end, loop_iter, iters, thres, tag)                                       \
    SMPI_SAMPLE_LOOP(loop_init, (loop_end), (loop_iter), 0, (iters), (thres), tag)
#define SMPI_SAMPLE_GLOBAL(loop_init, loop_end, loop_iter, iters, thres)                                               \
    SMPI_SAMPLE_LOOP(loop_init, (loop_end), (loop_iter), 1, (iters), (thres), "")
#define SMPI_SAMPLE_GLOBAL_TAG(loop_init, loop_end, loop_iter, iters, thres, tag)                                      \
    SMPI_SAMPLE_LOOP(loop_init, (loop_end), (loop_iter), 1, (iters), (thres), tag)
#define SMPI_SAMPLE_DELAY(duration) for (smpi_execute(duration); 0;)
#define SMPI_SAMPLE_FLOPS(flops)    for (smpi_execute_flops(flops); 0;)
void* smpi_shared_malloc(size_t size, const char* file, int line);
#define SMPI_SHARED_MALLOC(size) smpi_shared_malloc((size), __FILE__, __LINE__)
void* smpi_shared_malloc_partial(size_t size, const size_t* shared_block_offsets, int nb_shared_blocks);
#define SMPI_PARTIAL_SHARED_MALLOC(size, shared_block_offsets, nb_shared_blocks)                                       \
    smpi_shared_malloc_partial((size), (shared_block_offsets), (nb_shared_blocks))

#define SMPI_SHARED_FREE(data) smpi_shared_free(data)

int smpi_shared_known_call(const char* func, const char* input);
void* smpi_shared_get_call(const char* func, const char* input);
void* smpi_shared_set_call(const char* func, const char* input, void* data);
#define SMPI_SHARED_CALL(func, input, ...)                                                                             \
    (smpi_shared_known_call(_XBT_STRINGIFY(func), (input))                                                             \
         ? smpi_shared_get_call(_XBT_STRINGIFY(func), (input))                                                         \
         : smpi_shared_set_call(_XBT_STRINGIFY(func), (input), ((func)(__VA_ARGS__))))

#endif

#endif // S4BXI_MPI_MIDDLEWARE_H