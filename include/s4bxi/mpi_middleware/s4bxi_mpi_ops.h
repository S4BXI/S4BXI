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

#ifndef S4BXI_MPI_OPS_H
#define S4BXI_MPI_OPS_H

// Inside Bull's OMPI there are already all the type definition we need, steal them in other cases only
#ifndef OMPI_MPI_H
#include <smpi/smpi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILD_MPI_MIDDLEWARE

struct s4bxi_mpi_ops {
    void* Init;
    void* Finalize;
    void* Finalized;
    void* Init_thread;
    void* Initialized;
    void* Query_thread;
    void* Is_thread_main;
    void* Get_version;
    void* Get_library_version;
    void* Get_processor_name;
    void* Abort;
    void* Alloc_mem;
    void* Free_mem;
    void* Wtime;
    void* Wtick;
    void* Buffer_attach;
    void* Buffer_detach;
    void* Address;
    void* Get_address;
    void* Aint_diff;
    void* Aint_add;
    void* Error_class;
    void* Error_string;
    void* Attr_delete;
    void* Attr_get;
    void* Attr_put;
    void* Keyval_create;
    void* Keyval_free;
    void* Type_free;
    void* Type_size;
    void* Type_size_x;
    void* Type_get_extent;
    void* Type_get_extent_x;
    void* Type_get_true_extent;
    void* Type_get_true_extent_x;
    void* Type_extent;
    void* Type_lb;
    void* Type_ub;
    void* Type_commit;
    void* Type_hindexed;
    void* Type_create_hindexed;
    void* Type_create_hindexed_block;
    void* Type_hvector;
    void* Type_create_hvector;
    void* Type_indexed;
    void* Type_create_indexed;
    void* Type_create_indexed_block;
    void* Type_struct;
    void* Type_create_struct;
    void* Type_vector;
    void* Type_contiguous;
    void* Type_create_resized;
    void* Type_f2c;
    void* Type_c2f;
    void* Get_count;
    void* Type_get_attr;
    void* Type_set_attr;
    void* Type_delete_attr;
    void* Type_create_keyval;
    void* Type_free_keyval;
    void* Type_dup;
    void* Type_set_name;
    void* Type_get_name;
    void* Pack;
    void* Pack_size;
    void* Unpack;
    void* Op_create;
    void* Op_free;
    void* Op_commutative;
    void* Op_f2c;
    void* Op_c2f;
    void* Group_free;
    void* Group_size;
    void* Group_rank;
    void* Group_translate_ranks;
    void* Group_compare;
    void* Group_union;
    void* Group_intersection;
    void* Group_difference;
    void* Group_incl;
    void* Group_excl;
    void* Group_range_incl;
    void* Group_range_excl;
    void* Group_f2c;
    void* Group_c2f;
    void* Comm_rank;
    void* Comm_size;
    void* Comm_get_name;
    void* Comm_set_name;
    void* Comm_dup;
    void* Comm_dup_with_info;
    void* Comm_get_attr;
    void* Comm_set_attr;
    void* Comm_delete_attr;
    void* Comm_create_keyval;
    void* Comm_free_keyval;
    void* Comm_group;
    void* Comm_compare;
    void* Comm_create;
    void* Comm_create_group;
    void* Comm_free;
    void* Comm_disconnect;
    void* Comm_split;
    void* Comm_set_info;
    void* Comm_get_info;
    void* Comm_split_type;
    void* Comm_f2c;
    void* Comm_c2f;
    void* Start;
    void* Startall;
    void* Request_free;
    void* Recv;
    void* Recv_init;
    void* Irecv;
    void* Send;
    void* Send_init;
    void* Isend;
    void* Ssend;
    void* Ssend_init;
    void* Issend;
    void* Bsend;
    void* Bsend_init;
    void* Ibsend;
    void* Rsend;
    void* Rsend_init;
    void* Irsend;
    void* Sendrecv;
    void* Sendrecv_replace;
    void* Test;
    void* Testany;
    void* Testall;
    void* Testsome;
    void* Test_cancelled;
    void* Wait;
    void* Waitany;
    void* Waitall;
    void* Waitsome;
    void* Iprobe;
    void* Probe;
    void* Request_f2c;
    void* Request_c2f;
    void* Cancel;
    void* Bcast;
    void* Barrier;
    void* Ibarrier;
    void* Ibcast;
    void* Igather;
    void* Igatherv;
    void* Iallgather;
    void* Iallgatherv;
    void* Iscatter;
    void* Iscatterv;
    void* Ireduce;
    void* Iallreduce;
    void* Iscan;
    void* Iexscan;
    void* Ireduce_scatter;
    void* Ireduce_scatter_block;
    void* Ialltoall;
    void* Ialltoallv;
    void* Ialltoallw;
    void* Gather;
    void* Gatherv;
    void* Allgather;
    void* Allgatherv;
    void* Scatter;
    void* Scatterv;
    void* Reduce;
    void* Allreduce;
    void* Scan;
    void* Exscan;
    void* Reduce_scatter;
    void* Reduce_scatter_block;
    void* Alltoall;
    void* Alltoallv;
    void* Alltoallw;
    void* Reduce_local;
    void* Info_create;
    void* Info_set;
    void* Info_get;
    void* Info_free;
    void* Info_delete;
    void* Info_dup;
    void* Info_get_nkeys;
    void* Info_get_nthkey;
    void* Info_get_valuelen;
    void* Info_f2c;
    void* Info_c2f;
    void* Win_free;
    void* Win_create;
    void* Win_allocate;
    void* Win_allocate_shared;
    void* Win_create_dynamic;
    void* Win_attach;
    void* Win_detach;
    void* Win_set_name;
    void* Win_get_name;
    void* Win_set_info;
    void* Win_get_info;
    void* Win_get_group;
    void* Win_fence;
    void* Win_get_attr;
    void* Win_set_attr;
    void* Win_delete_attr;
    void* Win_create_keyval;
    void* Win_free_keyval;
    void* Win_complete;
    void* Win_post;
    void* Win_start;
    void* Win_wait;
    void* Win_lock;
    void* Win_lock_all;
    void* Win_unlock;
    void* Win_unlock_all;
    void* Win_flush;
    void* Win_flush_local;
    void* Win_flush_all;
    void* Win_flush_local_all;
    void* Win_shared_query;
    void* Win_sync;
    void* Win_f2c;
    void* Win_c2f;
    void* Get;
    void* Put;
    void* Accumulate;
    void* Get_accumulate;
    void* Rget;
    void* Rput;
    void* Raccumulate;
    void* Rget_accumulate;
    void* Fetch_and_op;
    void* Compare_and_swap;
    void* Cart_coords;
    void* Cart_create;
    void* Cart_get;
    void* Cart_rank;
    void* Cart_shift;
    void* Cart_sub;
    void* Cartdim_get;
    void* Dims_create;
    void* Request_get_status;
    void* Grequest_start;
    void* Grequest_complete;
    void* Status_set_cancelled;
    void* Status_set_elements;
    void* Status_set_elements_x;
    void* Type_create_subarray;
    void* File_open;
    void* File_close;
    void* File_delete;
    void* File_get_size;
    void* File_get_group;
    void* File_get_amode;
    void* File_set_info;
    void* File_get_info;
    void* File_read_at;
    void* File_read_at_all;
    void* File_write_at;
    void* File_write_at_all;
    void* File_read;
    void* File_read_all;
    void* File_write;
    void* File_write_all;
    void* File_seek;
    void* File_get_position;
    void* File_read_shared;
    void* File_write_shared;
    void* File_read_ordered;
    void* File_write_ordered;
    void* File_seek_shared;
    void* File_get_position_shared;
    void* File_sync;
    void* File_set_view;
    void* File_get_view;
    void* Errhandler_set;
    void* Errhandler_create;
    void* Errhandler_free;
    void* Errhandler_get;
    void* Comm_set_errhandler;
    void* Comm_get_errhandler;
    void* Comm_create_errhandler;
    void* Comm_call_errhandler;
    void* Win_set_errhandler;
    void* Win_get_errhandler;
    void* Win_create_errhandler;
    void* Win_call_errhandler;
    void* Errhandler_f2c;
    void* Errhandler_c2f;
    void* Type_create_f90_integer;
    void* Type_create_f90_real;
    void* Type_create_f90_complex;
    void* Type_get_contents;
    void* Type_get_envelope;
    void* File_call_errhandler;
    void* File_create_errhandler;
    void* File_set_errhandler;
    void* File_get_errhandler;

    // Special communicators
    MPI_Comm COMM_WORLD;
    MPI_Comm COMM_SELF;

    // Datatypes
    MPI_Datatype TYPE_DATATYPE_NULL;
    MPI_Datatype TYPE_CHAR;
    MPI_Datatype TYPE_SHORT;
    MPI_Datatype TYPE_INT;
    MPI_Datatype TYPE_LONG;
    MPI_Datatype TYPE_LONG_LONG;
    MPI_Datatype TYPE_SIGNED_CHAR;
    MPI_Datatype TYPE_UNSIGNED_CHAR;
    MPI_Datatype TYPE_UNSIGNED_SHORT;
    MPI_Datatype TYPE_UNSIGNED;
    MPI_Datatype TYPE_UNSIGNED_LONG;
    MPI_Datatype TYPE_UNSIGNED_LONG_LONG;
    MPI_Datatype TYPE_FLOAT;
    MPI_Datatype TYPE_DOUBLE;
    MPI_Datatype TYPE_LONG_DOUBLE;
    MPI_Datatype TYPE_WCHAR;
    MPI_Datatype TYPE_C_BOOL;
    MPI_Datatype TYPE_INT8_T;
    MPI_Datatype TYPE_INT16_T;
    MPI_Datatype TYPE_INT32_T;
    MPI_Datatype TYPE_INT64_T;
    MPI_Datatype TYPE_UINT8_T;
    MPI_Datatype TYPE_BYTE;
    MPI_Datatype TYPE_UINT16_T;
    MPI_Datatype TYPE_UINT32_T;
    MPI_Datatype TYPE_UINT64_T;
    MPI_Datatype TYPE_C_FLOAT_COMPLEX;
    MPI_Datatype TYPE_C_DOUBLE_COMPLEX;
    MPI_Datatype TYPE_C_LONG_DOUBLE_COMPLEX;
    MPI_Datatype TYPE_AINT;
    MPI_Datatype TYPE_OFFSET;
    MPI_Datatype TYPE_LB;
    MPI_Datatype TYPE_UB;
    // The following are datatypes for the MPI functions MPI_MAXLOC  and MPI_MINLOC.
    MPI_Datatype TYPE_FLOAT_INT;
    MPI_Datatype TYPE_LONG_INT;
    MPI_Datatype TYPE_DOUBLE_INT;
    MPI_Datatype TYPE_SHORT_INT;
    MPI_Datatype TYPE_2INT;
    MPI_Datatype TYPE_LONG_DOUBLE_INT;
    MPI_Datatype TYPE_2FLOAT;
    MPI_Datatype TYPE_2DOUBLE;
    MPI_Datatype TYPE_2LONG;
    // only for compatibility with Fortran
    MPI_Datatype TYPE_REAL;
    MPI_Datatype TYPE_REAL4;
    MPI_Datatype TYPE_REAL8;
    MPI_Datatype TYPE_REAL16;
    MPI_Datatype TYPE_COMPLEX8;
    MPI_Datatype TYPE_COMPLEX16;
    MPI_Datatype TYPE_COMPLEX32;
    MPI_Datatype TYPE_INTEGER1;
    MPI_Datatype TYPE_INTEGER2;
    MPI_Datatype TYPE_INTEGER4;
    MPI_Datatype TYPE_INTEGER8;
    MPI_Datatype TYPE_INTEGER16;
    MPI_Datatype TYPE_COUNT;

    // Operators
    MPI_Op OP_MAX;
    MPI_Op OP_MIN;
    MPI_Op OP_MAXLOC;
    MPI_Op OP_MINLOC;
    MPI_Op OP_SUM;
    MPI_Op OP_PROD;
    MPI_Op OP_LAND;
    MPI_Op OP_LOR;
    MPI_Op OP_LXOR;
    MPI_Op OP_BAND;
    MPI_Op OP_BOR;
    MPI_Op OP_BXOR;
    MPI_Op OP_REPLACE;
    MPI_Op OP_NO_OP;

    // Request
    MPI_Request REQUEST_NULL;
};

#else // BUILD_MPI_MIDDLEWARE

struct s4bxi_mpi_ops {}; // Needs to be defined for MainActors to compile

#endif

#ifdef __cplusplus
}
#endif

#endif // S4BXI_MPI_OPS_H