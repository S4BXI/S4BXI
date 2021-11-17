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

#include <dlfcn.h>
#include <fcntl.h>

#include "s4bxi/mpi_middleware/s4bxi_mpi_middleware.h"
#include "s4bxi/s4bxi_xbt_log.h"
#include "s4bxi/actors/BxiMainActor.hpp"
#include "s4bxi/s4bxi_util.hpp"
#include "s4bxi/mpi_middleware/BxiMpiComm.hpp"
#include <smpi_actor.hpp>

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_mpi_middlware, "Messages generated in MPI middleware");

using namespace std;

MPI_Datatype* type_array;

std::unique_ptr<struct s4bxi_mpi_ops> smpi_mpi_ops = nullptr;

// "Constants"
void* S4BXI_MPI_IN_PLACE()
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    return (main_actor->use_smpi_implem ? ((void*)-222) : ((void*)1));
}

MPI_Request S4BXI_MPI_REQUEST_NULL()
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    return (main_actor->use_smpi_implem ? NULL : main_actor->bull_mpi_ops->REQUEST_NULL);
}

#define SETUP_SYMBOLS_IN_IMPLEMS(symbol)                                                                               \
    main_actor->bull_mpi_ops->symbol = dlsym(bull_libhandle, "PMPI_" #symbol);                                         \
    assert(main_actor->bull_mpi_ops->symbol != nullptr);                                                               \
    if (setup_smpi) {                                                                                                  \
        smpi_mpi_ops->symbol = dlsym(smpi_libhandle, "PMPI_" #symbol);                                                 \
        assert(smpi_mpi_ops->symbol != nullptr);                                                                       \
    }

#define SETUP_COMMS_IN_IMPLEMS(lowercase, uppercase)                                                                   \
    main_actor->bull_mpi_ops->COMM_##uppercase = (MPI_Comm)dlsym(bull_libhandle, "ompi_mpi_comm_" #lowercase);         \
    assert(main_actor->bull_mpi_ops->COMM_##uppercase != nullptr);                                                     \
    if (setup_smpi)                                                                                                    \
        smpi_mpi_ops->COMM_##uppercase = nullptr;

#define SETUP_DATATYPES_IN_IMPLEMS(lowercase, uppercase)                                                               \
    main_actor->bull_mpi_ops->TYPE_##uppercase = (MPI_Datatype)dlsym(bull_libhandle, "ompi_mpi_" #lowercase);          \
    assert(main_actor->bull_mpi_ops->TYPE_##uppercase != nullptr);                                                     \
    if (setup_smpi) {                                                                                                  \
        smpi_mpi_ops->TYPE_##uppercase = (MPI_Datatype)dlsym(smpi_libhandle, "smpi_MPI_" #uppercase);                  \
        assert(smpi_mpi_ops->TYPE_##uppercase != nullptr);                                                             \
    }

#define SETUP_OPS_IN_IMPLEMS(lowercase, uppercase)                                                                     \
    main_actor->bull_mpi_ops->OP_##uppercase = (MPI_Op)dlsym(bull_libhandle, "ompi_mpi_op_" #lowercase);               \
    assert(main_actor->bull_mpi_ops->OP_##uppercase != nullptr);                                                       \
    if (setup_smpi) {                                                                                                  \
        smpi_mpi_ops->OP_##uppercase = (MPI_Op)dlsym(smpi_libhandle, "smpi_MPI_" #uppercase);                          \
        assert(smpi_mpi_ops->OP_##uppercase != nullptr);                                                               \
    }

MPI_Datatype implem_datatype(MPI_Datatype original)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

#define MPI_TYPE_TRANSLATION(type)                                                                                     \
    if (original == MPI_##type || original == main_actor->bull_mpi_ops->TYPE_##type ||                                 \
        original == smpi_mpi_ops->TYPE_##type)                                                                         \
        return (main_actor->use_smpi_implem ? smpi_mpi_ops : main_actor->bull_mpi_ops)->TYPE_##type;

    MPI_TYPE_TRANSLATION(CHAR)
    MPI_TYPE_TRANSLATION(DATATYPE_NULL)
    MPI_TYPE_TRANSLATION(CHAR)
    MPI_TYPE_TRANSLATION(SHORT)
    MPI_TYPE_TRANSLATION(INT)
    MPI_TYPE_TRANSLATION(LONG)
    MPI_TYPE_TRANSLATION(LONG_LONG)
    MPI_TYPE_TRANSLATION(SIGNED_CHAR)
    MPI_TYPE_TRANSLATION(UNSIGNED_CHAR)
    MPI_TYPE_TRANSLATION(UNSIGNED_SHORT)
    MPI_TYPE_TRANSLATION(UNSIGNED)
    MPI_TYPE_TRANSLATION(UNSIGNED_LONG)
    MPI_TYPE_TRANSLATION(UNSIGNED_LONG_LONG)
    MPI_TYPE_TRANSLATION(FLOAT)
    MPI_TYPE_TRANSLATION(DOUBLE)
    MPI_TYPE_TRANSLATION(LONG_DOUBLE)
    MPI_TYPE_TRANSLATION(WCHAR)
    MPI_TYPE_TRANSLATION(C_BOOL)
    MPI_TYPE_TRANSLATION(INT8_T)
    MPI_TYPE_TRANSLATION(INT16_T)
    MPI_TYPE_TRANSLATION(INT32_T)
    MPI_TYPE_TRANSLATION(INT64_T)
    MPI_TYPE_TRANSLATION(UINT8_T)
    MPI_TYPE_TRANSLATION(BYTE)
    MPI_TYPE_TRANSLATION(UINT16_T)
    MPI_TYPE_TRANSLATION(UINT32_T)
    MPI_TYPE_TRANSLATION(UINT64_T)
    MPI_TYPE_TRANSLATION(C_FLOAT_COMPLEX)
    MPI_TYPE_TRANSLATION(C_DOUBLE_COMPLEX)
    MPI_TYPE_TRANSLATION(C_LONG_DOUBLE_COMPLEX)
    MPI_TYPE_TRANSLATION(AINT)
    MPI_TYPE_TRANSLATION(OFFSET)
    MPI_TYPE_TRANSLATION(LB)
    MPI_TYPE_TRANSLATION(UB)
    MPI_TYPE_TRANSLATION(FLOAT_INT)
    MPI_TYPE_TRANSLATION(LONG_INT)
    MPI_TYPE_TRANSLATION(DOUBLE_INT)
    MPI_TYPE_TRANSLATION(SHORT_INT)
    MPI_TYPE_TRANSLATION(2INT)
    MPI_TYPE_TRANSLATION(LONG_DOUBLE_INT)
    MPI_TYPE_TRANSLATION(2FLOAT)
    MPI_TYPE_TRANSLATION(2DOUBLE)
    MPI_TYPE_TRANSLATION(2LONG)
    MPI_TYPE_TRANSLATION(REAL)
    MPI_TYPE_TRANSLATION(REAL4)
    MPI_TYPE_TRANSLATION(REAL8)
    MPI_TYPE_TRANSLATION(REAL16)
    MPI_TYPE_TRANSLATION(COMPLEX8)
    MPI_TYPE_TRANSLATION(COMPLEX16)
    MPI_TYPE_TRANSLATION(COMPLEX32)
    MPI_TYPE_TRANSLATION(INTEGER1)
    MPI_TYPE_TRANSLATION(INTEGER2)
    MPI_TYPE_TRANSLATION(INTEGER4)
    MPI_TYPE_TRANSLATION(INTEGER8)
    MPI_TYPE_TRANSLATION(INTEGER16)
    MPI_TYPE_TRANSLATION(COUNT)

#undef MPI_TYPE_TRANSLATION

    return original;
}

MPI_Datatype* implem_datatypes(const MPI_Datatype* original, int size, MPI_Datatype* out)
{
    for (int i = 0; i < size; ++i)
        out[i] = implem_datatype(original[i]);

    return out;
}

MPI_Op implem_op(MPI_Op original)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

#define MPI_OP_TRANSLATION(op)                                                                                         \
    if (original == MPI_##op || original == main_actor->bull_mpi_ops->OP_##op || original == smpi_mpi_ops->OP_##op)    \
        return (main_actor->use_smpi_implem ? smpi_mpi_ops : main_actor->bull_mpi_ops)->OP_##op;

    MPI_OP_TRANSLATION(MAX)
    MPI_OP_TRANSLATION(MIN)
    MPI_OP_TRANSLATION(MAXLOC)
    MPI_OP_TRANSLATION(MINLOC)
    MPI_OP_TRANSLATION(SUM)
    MPI_OP_TRANSLATION(PROD)
    MPI_OP_TRANSLATION(LAND)
    MPI_OP_TRANSLATION(LOR)
    MPI_OP_TRANSLATION(LXOR)
    MPI_OP_TRANSLATION(BAND)
    MPI_OP_TRANSLATION(BOR)
    MPI_OP_TRANSLATION(BXOR)
    MPI_OP_TRANSLATION(REPLACE)
    MPI_OP_TRANSLATION(NO_OP)

#undef MPI_OP_TRANSLATION

    return original;
}

#define S4BXI_MPI_ONE_IMPLEM(rtype, name, args, argsval)                                                               \
    typedef rtype(*name##_func) args;                                                                                  \
    rtype S4BXI_MPI_##name args                                                                                        \
    {                                                                                                                  \
        BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;                                                             \
        return ((name##_func)(main_actor->use_smpi_implem ? smpi_mpi_ops : main_actor->bull_mpi_ops)->name)argsval;    \
    }

// This one is kind of specific, it's only used for [I]alltoallw because of the datatype arrays
#define S4BXI_MPI_W_COLLECTIVE(rtype, name, args, argsval)                                                             \
    typedef rtype(*name##_func) args;                                                                                  \
    rtype S4BXI_MPI_##name args                                                                                        \
    {                                                                                                                  \
        BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;                                                             \
        int size;                                                                                                      \
        S4BXI_MPI_Comm_size(comm, &size);                                                                              \
        MPI_Datatype sendtypes_arr[size];                                                                              \
        MPI_Datatype recvtypes_arr[size];                                                                              \
        return ((name##_func)(main_actor->use_smpi_implem ? smpi_mpi_ops : main_actor->bull_mpi_ops)->name)argsval;    \
    }

typedef int (*Init_func)(int* argc, char*** argv);
int S4BXI_MPI_Init(int* argc, char*** argv)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    XBT_INFO("Init SMPI");
    int smpi = ((Init_func)smpi_mpi_ops->Init)(argc, argv);
    XBT_INFO("Init Bull MPI");
    int bull = ((Init_func)main_actor->bull_mpi_ops->Init)(argc, argv);

    return bull > smpi ? bull : smpi;
}

typedef int (*Finalize_func)(void);
int S4BXI_MPI_Finalize(void)
{
    s4bxi_barrier();

    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    XBT_INFO("Finalize SMPI");
    int smpi = ((Finalize_func)smpi_mpi_ops->Finalize)();
    XBT_INFO("Finalize Bull MPI");
    int bull = ((Finalize_func)main_actor->bull_mpi_ops->Finalize)();

    return bull > smpi ? bull : smpi;
}

// S4BXI_MPI_ONE_IMPLEM(int, Finalized, (int* flag));
// S4BXI_MPI_ONE_IMPLEM(int, Init_thread, (int* argc, char*** argv, int required, int* provided));
// S4BXI_MPI_ONE_IMPLEM(int, Initialized, (int* flag));
// S4BXI_MPI_ONE_IMPLEM(int, Query_thread, (int* provided));
// S4BXI_MPI_ONE_IMPLEM(int, Is_thread_main, (int* flag));
// S4BXI_MPI_ONE_IMPLEM(int, Get_version, (int* version, int* subversion));
// S4BXI_MPI_ONE_IMPLEM(int, Get_library_version, (char* version, int* len));
// S4BXI_MPI_ONE_IMPLEM(int, Get_processor_name, (char* name, int* resultlen));
S4BXI_MPI_ONE_IMPLEM(int, Abort, (MPI_Comm comm, int errorcode), (BxiMpiComm::implem_comm(comm), errorcode));
// S4BXI_MPI_ONE_IMPLEM(int, Alloc_mem, (MPI_Aint size, MPI_Info info, void* baseptr));
// S4BXI_MPI_ONE_IMPLEM(int, Free_mem, (void* base));
S4BXI_MPI_ONE_IMPLEM(double, Wtime, (void), ());
S4BXI_MPI_ONE_IMPLEM(double, Wtick, (void), ());
// S4BXI_MPI_ONE_IMPLEM(int, Buffer_attach, (void* buffer, int size));
// S4BXI_MPI_ONE_IMPLEM(int, Buffer_detach, (void* buffer, int* size));
// S4BXI_MPI_ONE_IMPLEM(int, Address, (const void* location, MPI_Aint* address));
// S4BXI_MPI_ONE_IMPLEM(int, Get_address, (const void* location, MPI_Aint* address));
// S4BXI_MPI_ONE_IMPLEM(MPI_Aint, Aint_diff, (MPI_Aint base, MPI_Aint disp));
// S4BXI_MPI_ONE_IMPLEM(MPI_Aint, Aint_add, (MPI_Aint base, MPI_Aint disp));
// S4BXI_MPI_ONE_IMPLEM(int, Error_class, (int errorcode, int* errorclass));
// S4BXI_MPI_ONE_IMPLEM(int, Error_string, (int errorcode, char* string, int* resultlen));

// S4BXI_MPI_ONE_IMPLEM(int, Attr_delete, (MPI_Comm comm, int keyval));
// S4BXI_MPI_ONE_IMPLEM(int, Attr_get, (MPI_Comm comm, int keyval, void* attr_value, int* flag));
// S4BXI_MPI_ONE_IMPLEM(int, Attr_put, (MPI_Comm comm, int keyval, void* attr_value));
// S4BXI_MPI_ONE_IMPLEM(int, Keyval_create,
//                      (MPI_Copy_function * copy_fn, MPI_Delete_function* delete_fn, int* keyval, void* extra_state));
// S4BXI_MPI_ONE_IMPLEM(int, Keyval_free, (int* keyval));

// S4BXI_MPI_ONE_IMPLEM(int, Type_free, (MPI_Datatype * datatype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_size, (MPI_Datatype datatype, int* size));
// S4BXI_MPI_ONE_IMPLEM(int, Type_size_x, (MPI_Datatype datatype, MPI_Count* size));
// S4BXI_MPI_ONE_IMPLEM(int, Type_get_extent, (MPI_Datatype datatype, MPI_Aint* lb, MPI_Aint* extent));
// S4BXI_MPI_ONE_IMPLEM(int, Type_get_extent_x, (MPI_Datatype datatype, MPI_Count* lb, MPI_Count* extent));
// S4BXI_MPI_ONE_IMPLEM(int, Type_get_true_extent, (MPI_Datatype datatype, MPI_Aint* lb, MPI_Aint* extent));
// S4BXI_MPI_ONE_IMPLEM(int, Type_get_true_extent_x, (MPI_Datatype datatype, MPI_Count* lb, MPI_Count* extent));
// S4BXI_MPI_ONE_IMPLEM(int, Type_extent, (MPI_Datatype datatype, MPI_Aint* extent));
// S4BXI_MPI_ONE_IMPLEM(int, Type_lb, (MPI_Datatype datatype, MPI_Aint* disp));
// S4BXI_MPI_ONE_IMPLEM(int, Type_ub, (MPI_Datatype datatype, MPI_Aint* disp));
// S4BXI_MPI_ONE_IMPLEM(int, Type_commit, (MPI_Datatype * datatype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_hindexed,
//                      (int count, const int* blocklens, const MPI_Aint* indices, MPI_Datatype old_type,
//                       MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_create_hindexed,
//                      (int count, const int* blocklens, const MPI_Aint* indices, MPI_Datatype old_type,
//                       MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_create_hindexed_block,
//                      (int count, int blocklength, const MPI_Aint* indices, MPI_Datatype old_type,
//                       MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_hvector,
//                      (int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_create_hvector,
//                      (int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_indexed,
//                      (int count, const int* blocklens, const int* indices, MPI_Datatype old_type,
//                       MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_create_indexed,
//                      (int count, const int* blocklens, const int* indices, MPI_Datatype old_type,
//                       MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_create_indexed_block,
//                      (int count, int blocklength, const int* indices, MPI_Datatype old_type, MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_struct,
//                      (int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* old_types,
//                       MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_create_struct,
//                      (int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* old_types,
//                       MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_vector,
//                      (int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_contiguous, (int count, MPI_Datatype old_type, MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_create_resized,
//                      (MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(MPI_Datatype, Type_f2c, (MPI_Fint datatype));
// S4BXI_MPI_ONE_IMPLEM(MPI_Fint, Type_c2f, (MPI_Datatype datatype));
// S4BXI_MPI_ONE_IMPLEM(int, Get_count, (const MPI_Status* status, MPI_Datatype datatype, int* count));
// S4BXI_MPI_ONE_IMPLEM(int, Type_get_attr, (MPI_Datatype type, int type_keyval, void* attribute_val, int* flag));
// S4BXI_MPI_ONE_IMPLEM(int, Type_set_attr, (MPI_Datatype type, int type_keyval, void* att));
// S4BXI_MPI_ONE_IMPLEM(int, Type_delete_attr, (MPI_Datatype type, int comm_keyval));
// S4BXI_MPI_ONE_IMPLEM(int, Type_create_keyval,
//                      (MPI_Type_copy_attr_function * copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval,
//                       void* extra_state));
// S4BXI_MPI_ONE_IMPLEM(int, Type_free_keyval, (int* keyval));
// S4BXI_MPI_ONE_IMPLEM(int, Type_dup, (MPI_Datatype datatype, MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_set_name, (MPI_Datatype datatype, const char* name));
// S4BXI_MPI_ONE_IMPLEM(int, Type_get_name, (MPI_Datatype datatype, char* name, int* len));

// S4BXI_MPI_ONE_IMPLEM(int, Pack,
//                      (const void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position,
//                       MPI_Comm comm));
// S4BXI_MPI_ONE_IMPLEM(int, Pack_size, (int incount, MPI_Datatype datatype, MPI_Comm comm, int* size));
// S4BXI_MPI_ONE_IMPLEM(int, Unpack,
//                      (const void* inbuf, int insize, int* position, void* outbuf, int outcount, MPI_Datatype type,
//                       MPI_Comm comm));

// S4BXI_MPI_ONE_IMPLEM(int, Op_create, (MPI_User_function * function, int commute, MPI_Op* op));
// S4BXI_MPI_ONE_IMPLEM(int, Op_free, (SETUP_OPS_IN_IMPLEMS(* op)))
// S4BXI_MPI_ONE_IMPLEM(int, Op_commutative, (SETUP_OPS_IN_IMPLEMS(op, int* commute)))
// S4BXI_MPI_ONE_IMPLEM(MPI_Op, Op_f2c, (MPI_Fint op));
// S4BXI_MPI_ONE_IMPLEM(MPI_Fint, Op_c2f, (SETUP_OPS_IN_IMPLEMS(op)))

// S4BXI_MPI_ONE_IMPLEM(int, Group_free, (MPI_Group * group));
// S4BXI_MPI_ONE_IMPLEM(int, Group_size, (MPI_Group group, int* size));
// S4BXI_MPI_ONE_IMPLEM(int, Group_rank, (MPI_Group group, int* rank));
// S4BXI_MPI_ONE_IMPLEM(int, Group_translate_ranks,
//                      (MPI_Group group1, int n, const int* ranks1, MPI_Group group2, int* ranks2));
// S4BXI_MPI_ONE_IMPLEM(int, Group_compare, (MPI_Group group1, MPI_Group group2, int* result));
// S4BXI_MPI_ONE_IMPLEM(int, Group_union, (MPI_Group group1, MPI_Group group2, MPI_Group* newgroup));
// S4BXI_MPI_ONE_IMPLEM(int, Group_intersection, (MPI_Group group1, MPI_Group group2, MPI_Group* newgroup));
// S4BXI_MPI_ONE_IMPLEM(int, Group_difference, (MPI_Group group1, MPI_Group group2, MPI_Group* newgroup));
// S4BXI_MPI_ONE_IMPLEM(int, Group_incl, (MPI_Group group, int n, const int* ranks, MPI_Group* newgroup));
// S4BXI_MPI_ONE_IMPLEM(int, Group_excl, (MPI_Group group, int n, const int* ranks, MPI_Group* newgroup));
// S4BXI_MPI_ONE_IMPLEM(int, Group_range_incl, (MPI_Group group, int n, int ranges[][3], MPI_Group* newgroup));
// S4BXI_MPI_ONE_IMPLEM(int, Group_range_excl, (MPI_Group group, int n, int ranges[][3], MPI_Group* newgroup));
// S4BXI_MPI_ONE_IMPLEM(MPI_Group, Group_f2c, (MPI_Fint group));
// S4BXI_MPI_ONE_IMPLEM(MPI_Fint, Group_c2f, (MPI_Group group));

S4BXI_MPI_ONE_IMPLEM(int, Comm_rank, (MPI_Comm comm, int* rank), (BxiMpiComm::implem_comm(comm), rank));
S4BXI_MPI_ONE_IMPLEM(int, Comm_size, (MPI_Comm comm, int* size), (BxiMpiComm::implem_comm(comm), size));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_get_name, (MPI_Comm comm, char* name, int* len));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_set_name, (MPI_Comm comm, const char* name));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_dup, (MPI_Comm comm, MPI_Comm* newcomm));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_dup_with_info, (MPI_Comm comm, MPI_Info info, MPI_Comm* newcomm));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_get_attr, (MPI_Comm comm, int comm_keyval, void* attribute_val, int* flag));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_set_attr, (MPI_Comm comm, int comm_keyval, void* attribute_val));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_delete_attr, (MPI_Comm comm, int comm_keyval));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_create_keyval,
//                      (MPI_Comm_copy_attr_function * copy_fn, MPI_Comm_delete_attr_function* delete_fn, int* keyval,
//                       void* extra_state));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_free_keyval, (int* keyval));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_group, (MPI_Comm comm, MPI_Group* group));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_compare, (MPI_Comm comm1, MPI_Comm comm2, int* result));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_create, (MPI_Comm comm, MPI_Group group, MPI_Comm* newcomm));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_create_group, (MPI_Comm comm, MPI_Group group, int tag, MPI_Comm* newcomm));
typedef int (*Comm_free_func)(MPI_Comm* comm);
int S4BXI_MPI_Comm_free(MPI_Comm* comm)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    // We should probably check that the comm is not WORLD or SELF, but I feel that it's the user app problem if someone
    // tried to free WORLD or SELF...
    auto s4bxi_comm = (BxiMpiComm*)(*comm);
    int bull        = ((Comm_free_func)(main_actor->bull_mpi_ops->Comm_free))(&s4bxi_comm->bull);
    int smpi        = ((Comm_free_func)(smpi_mpi_ops->Comm_free))(&s4bxi_comm->smpi);
    delete s4bxi_comm;

    return bull > smpi ? bull : smpi;
}
// S4BXI_MPI_ONE_IMPLEM(int, Comm_disconnect, (MPI_Comm * comm));
typedef int (*Comm_split_func)(MPI_Comm comm, int color, int key, MPI_Comm* comm_out);
int S4BXI_MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm* comm_out)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    MPI_Comm comm_in_bull    = BxiMpiComm::bull_comm(comm);
    MPI_Comm comm_in_smpi    = BxiMpiComm::smpi_comm(comm);
    MPI_Comm comm_out_bull;
    MPI_Comm comm_out_smpi;

    int bull = ((Comm_split_func)(main_actor->bull_mpi_ops->Comm_split))(comm_in_bull, color, key, &comm_out_bull);
    int smpi = ((Comm_split_func)(smpi_mpi_ops->Comm_split))(comm_in_smpi, color, key, &comm_out_smpi);

    auto s4bxi_comm_out  = new BxiMpiComm;
    s4bxi_comm_out->bull = comm_out_bull;
    s4bxi_comm_out->smpi = comm_out_smpi;
    *comm_out            = (MPI_Comm)s4bxi_comm_out;

    return bull > smpi ? bull : smpi;
}
// S4BXI_MPI_ONE_IMPLEM(int, Comm_set_info, (MPI_Comm comm, MPI_Info info));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_get_info, (MPI_Comm comm, MPI_Info* info));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_split_type, (MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm*
// newcomm));
// S4BXI_MPI_ONE_IMPLEM(MPI_Comm, Comm_f2c, (MPI_Fint comm));
// S4BXI_MPI_ONE_IMPLEM(MPI_Fint, Comm_c2f, (MPI_Comm comm));

S4BXI_MPI_ONE_IMPLEM(int, Start, (MPI_Request * request), (request));
S4BXI_MPI_ONE_IMPLEM(int, Startall, (int count, MPI_Request* requests), (count, requests));
S4BXI_MPI_ONE_IMPLEM(int, Request_free, (MPI_Request * request), (request));
S4BXI_MPI_ONE_IMPLEM(int, Recv,
                     (void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status* status),
                     (buf, count, implem_datatype(datatype), src, tag, BxiMpiComm::implem_comm(comm), status));
S4BXI_MPI_ONE_IMPLEM(int, Recv_init,
                     (void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
                      MPI_Request* request),
                     (buf, count, implem_datatype(datatype), src, tag, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Irecv,
                     (void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
                      MPI_Request* request),
                     (buf, count, implem_datatype(datatype), src, tag, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Send, (const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm),
                     (buf, count, implem_datatype(datatype), dst, tag, BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Send_init,
                     (const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm,
                      MPI_Request* request),
                     (buf, count, implem_datatype(datatype), dst, tag, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Isend,
                     (const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm,
                      MPI_Request* request),
                     (buf, count, implem_datatype(datatype), dst, tag, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Ssend, (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm),
                     (buf, count, implem_datatype(datatype), dest, tag, BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Ssend_init,
                     (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                      MPI_Request* request),
                     (buf, count, implem_datatype(datatype), dest, tag, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Issend,
                     (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                      MPI_Request* request),
                     (buf, count, implem_datatype(datatype), dest, tag, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Bsend, (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm),
                     (buf, count, implem_datatype(datatype), dest, tag, BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Bsend_init,
                     (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                      MPI_Request* request),
                     (buf, count, implem_datatype(datatype), dest, tag, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Ibsend,
                     (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                      MPI_Request* request),
                     (buf, count, implem_datatype(datatype), dest, tag, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Rsend, (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm),
                     (buf, count, implem_datatype(datatype), dest, tag, BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Rsend_init,
                     (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                      MPI_Request* request),
                     (buf, count, implem_datatype(datatype), dest, tag, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Irsend,
                     (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                      MPI_Request* request),
                     (buf, count, implem_datatype(datatype), dest, tag, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Sendrecv,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag, void* recvbuf,
                      int recvcount, MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status* status),
                     (sendbuf, sendcount, implem_datatype(sendtype), dst, sendtag, recvbuf, recvcount,
                      implem_datatype(recvtype), src, recvtag, BxiMpiComm::implem_comm(comm), status));
S4BXI_MPI_ONE_IMPLEM(int, Sendrecv_replace,
                     (void* buf, int count, MPI_Datatype datatype, int dst, int sendtag, int src, int recvtag,
                      MPI_Comm comm, MPI_Status* status),
                     (buf, count, implem_datatype(datatype), dst, sendtag, src, recvtag, BxiMpiComm::implem_comm(comm),
                      status));

S4BXI_MPI_ONE_IMPLEM(int, Test, (MPI_Request * request, int* flag, MPI_Status* status), (request, flag, status));
S4BXI_MPI_ONE_IMPLEM(int, Testany, (int count, MPI_Request requests[], int* index, int* flag, MPI_Status* status),
                     (count, requests, index, flag, status));
S4BXI_MPI_ONE_IMPLEM(int, Testall, (int count, MPI_Request* requests, int* flag, MPI_Status* statuses),
                     (count, requests, flag, statuses));
S4BXI_MPI_ONE_IMPLEM(int, Testsome,
                     (int incount, MPI_Request requests[], int* outcount, int* indices, MPI_Status status[]),
                     (incount, requests, outcount, indices, status));
S4BXI_MPI_ONE_IMPLEM(int, Test_cancelled, (const MPI_Status* status, int* flag), (status, flag));
S4BXI_MPI_ONE_IMPLEM(int, Wait, (MPI_Request * request, MPI_Status* status), (request, status));
S4BXI_MPI_ONE_IMPLEM(int, Waitany, (int count, MPI_Request requests[], int* index, MPI_Status* status),
                     (count, requests, index, status));
S4BXI_MPI_ONE_IMPLEM(int, Waitall, (int count, MPI_Request requests[], MPI_Status status[]), (count, requests, status));
S4BXI_MPI_ONE_IMPLEM(int, Waitsome,
                     (int incount, MPI_Request requests[], int* outcount, int* indices, MPI_Status status[]),
                     (incount, requests, outcount, indices, status));
S4BXI_MPI_ONE_IMPLEM(int, Iprobe, (int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status),
                     (source, tag, BxiMpiComm::implem_comm(comm), flag, status));
S4BXI_MPI_ONE_IMPLEM(int, Probe, (int source, int tag, MPI_Comm comm, MPI_Status* status),
                     (source, tag, BxiMpiComm::implem_comm(comm), status));
// S4BXI_MPI_ONE_IMPLEM(MPI_Request, Request_f2c, (MPI_Fint request), (request));
// S4BXI_MPI_ONE_IMPLEM(MPI_Fint, Request_c2f, (MPI_Request request), (request));
S4BXI_MPI_ONE_IMPLEM(int, Cancel, (MPI_Request * request), (request));

S4BXI_MPI_ONE_IMPLEM(int, Bcast, (void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm),
                     (buf, count, implem_datatype(datatype), root, BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Barrier, (MPI_Comm comm), (BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Ibarrier, (MPI_Comm comm, MPI_Request* request), (BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Ibcast,
                     (void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request* request),
                     (buf, count, implem_datatype(datatype), root, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Igather,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                      MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request),
                     (sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcount, implem_datatype(recvtype),
                      root, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Igatherv,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
                      const int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request),
                     (sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcounts, displs,
                      implem_datatype(recvtype), root, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Iallgather,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                      MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request),
                     (sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcount, implem_datatype(recvtype),
                      BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Iallgatherv,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
                      const int* displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request),
                     (sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcounts, displs,
                      implem_datatype(recvtype), BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Iscatter,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                      MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request),
                     (sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcount, implem_datatype(recvtype),
                      root, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Iscatterv,
                     (const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype,
                      void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                      MPI_Request* request),
                     (sendbuf, sendcounts, displs, implem_datatype(sendtype), recvbuf, recvcount,
                      implem_datatype(recvtype), root, BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Ireduce,
                     (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                      MPI_Comm comm, MPI_Request* request),
                     (sendbuf, recvbuf, count, implem_datatype(datatype), implem_op(op), root,
                      BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Iallreduce,
                     (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                      MPI_Request* request),
                     (sendbuf, recvbuf, count, implem_datatype(datatype), implem_op(op), BxiMpiComm::implem_comm(comm),
                      request));
S4BXI_MPI_ONE_IMPLEM(int, Iscan,
                     (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                      MPI_Request* request),
                     (sendbuf, recvbuf, count, implem_datatype(datatype), implem_op(op), BxiMpiComm::implem_comm(comm),
                      request));
S4BXI_MPI_ONE_IMPLEM(int, Iexscan,
                     (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                      MPI_Request* request),
                     (sendbuf, recvbuf, count, implem_datatype(datatype), implem_op(op), BxiMpiComm::implem_comm(comm),
                      request));
S4BXI_MPI_ONE_IMPLEM(int, Ireduce_scatter,
                     (const void* sendbuf, void* recvbuf, const int* recvcounts, MPI_Datatype datatype, MPI_Op op,
                      MPI_Comm comm, MPI_Request* request),
                     (sendbuf, recvbuf, recvcounts, implem_datatype(datatype), implem_op(op),
                      BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Ireduce_scatter_block,
                     (const void* sendbuf, void* recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op,
                      MPI_Comm comm, MPI_Request* request),
                     (sendbuf, recvbuf, recvcount, implem_datatype(datatype), implem_op(op),
                      BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Ialltoall,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                      MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request),
                     (sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcount, implem_datatype(recvtype),
                      BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_ONE_IMPLEM(int, Ialltoallv,
                     (const void* sendbuf, const int* sendcounts, const int* senddisps, MPI_Datatype sendtype,
                      void* recvbuf, const int* recvcounts, const int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm,
                      MPI_Request* request),
                     (sendbuf, sendcounts, senddisps, implem_datatype(sendtype), recvbuf, recvcounts, recvdisps,
                      implem_datatype(recvtype), BxiMpiComm::implem_comm(comm), request));
S4BXI_MPI_W_COLLECTIVE(int, Ialltoallw,
                       (const void* sendbuf, const int* sendcounts, const int* senddisps, const MPI_Datatype* sendtypes,
                        void* recvbuf, const int* recvcounts, const int* recvdisps, const MPI_Datatype* recvtypes,
                        MPI_Comm comm, MPI_Request* request),
                       (sendbuf, sendcounts, senddisps, implem_datatypes(sendtypes, size, sendtypes_arr), recvbuf,
                        recvcounts, recvdisps, implem_datatypes(recvtypes, size, recvtypes_arr),
                        BxiMpiComm::implem_comm(comm), request))
S4BXI_MPI_ONE_IMPLEM(int, Gather,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                      MPI_Datatype recvtype, int root, MPI_Comm comm),
                     (sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcount, implem_datatype(recvtype),
                      root, BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Gatherv,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
                      const int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm),
                     (sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcounts, displs,
                      implem_datatype(recvtype), root, BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Allgather,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                      MPI_Datatype recvtype, MPI_Comm comm),
                     (sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcount, implem_datatype(recvtype),
                      BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Allgatherv,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
                      const int* displs, MPI_Datatype recvtype, MPI_Comm comm),
                     (sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcounts, displs,
                      implem_datatype(recvtype), BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Scatter,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                      MPI_Datatype recvtype, int root, MPI_Comm comm),
                     (sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcount, implem_datatype(recvtype),
                      root, BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Scatterv,
                     (const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype,
                      void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm),
                     (sendbuf, sendcounts, displs, implem_datatype(sendtype), recvbuf, recvcount,
                      implem_datatype(recvtype), root, BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Reduce,
                     (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                      MPI_Comm comm),
                     (sendbuf, recvbuf, count, implem_datatype(datatype), implem_op(op), root,
                      BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Allreduce,
                     (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm),
                     (sendbuf, recvbuf, count, implem_datatype(datatype), implem_op(op),
                      BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Scan,
                     (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm),
                     (sendbuf, recvbuf, count, implem_datatype(datatype), implem_op(op),
                      BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Exscan,
                     (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm),
                     (sendbuf, recvbuf, count, implem_datatype(datatype), implem_op(op),
                      BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Reduce_scatter,
                     (const void* sendbuf, void* recvbuf, const int* recvcounts, MPI_Datatype datatype, MPI_Op op,
                      MPI_Comm comm),
                     (sendbuf, recvbuf, recvcounts, implem_datatype(datatype), implem_op(op),
                      BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Reduce_scatter_block,
                     (const void* sendbuf, void* recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op,
                      MPI_Comm comm),
                     (sendbuf, recvbuf, recvcount, implem_datatype(datatype), implem_op(op),
                      BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Alltoall,
                     (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                      MPI_Datatype recvtype, MPI_Comm comm),
                     (sendbuf, sendcount, implem_datatype(sendtype), recvbuf, recvcount, implem_datatype(recvtype),
                      BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_ONE_IMPLEM(int, Alltoallv,
                     (const void* sendbuf, const int* sendcounts, const int* senddisps, MPI_Datatype sendtype,
                      void* recvbuf, const int* recvcounts, const int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm),
                     (sendbuf, sendcounts, senddisps, implem_datatype(sendtype), recvbuf, recvcounts, recvdisps,
                      implem_datatype(recvtype), BxiMpiComm::implem_comm(comm)));
S4BXI_MPI_W_COLLECTIVE(int, Alltoallw,
                       (const void* sendbuf, const int* sendcnts, const int* sdispls, const MPI_Datatype* sendtypes,
                        void* recvbuf, const int* recvcnts, const int* rdispls, const MPI_Datatype* recvtypes,
                        MPI_Comm comm),
                       (sendbuf, sendcnts, sdispls, implem_datatypes(sendtypes, size, sendtypes_arr), recvbuf, recvcnts,
                        rdispls, implem_datatypes(recvtypes, size, recvtypes_arr), BxiMpiComm::implem_comm(comm)))
// S4BXI_MPI_ONE_IMPLEM(int, Reduce_local,
//                      (const void* inbuf, void* inoutbuf, int count, MPI_Datatype datatype, SETUP_OPS_IN_IMPLEMS(op)))

// S4BXI_MPI_ONE_IMPLEM(int, Info_create, (MPI_Info * info));
// S4BXI_MPI_ONE_IMPLEM(int, Info_set, (MPI_Info info, const char* key, const char* value));
// S4BXI_MPI_ONE_IMPLEM(int, Info_get, (MPI_Info info, const char* key, int valuelen, char* value, int* flag));
// S4BXI_MPI_ONE_IMPLEM(int, Info_free, (MPI_Info * info));
// S4BXI_MPI_ONE_IMPLEM(int, Info_delete, (MPI_Info info, const char* key));
// S4BXI_MPI_ONE_IMPLEM(int, Info_dup, (MPI_Info info, MPI_Info* newinfo));
// S4BXI_MPI_ONE_IMPLEM(int, Info_get_nkeys, (MPI_Info info, int* nkeys));
// S4BXI_MPI_ONE_IMPLEM(int, Info_get_nthkey, (MPI_Info info, int n, char* key));
// S4BXI_MPI_ONE_IMPLEM(int, Info_get_valuelen, (MPI_Info info, const char* key, int* valuelen, int* flag));
// S4BXI_MPI_ONE_IMPLEM(MPI_Info, Info_f2c, (MPI_Fint info));
// S4BXI_MPI_ONE_IMPLEM(MPI_Fint, Info_c2f, (MPI_Info info));

// S4BXI_MPI_ONE_IMPLEM(int, Win_free, (MPI_Win * win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_create,
//                      (void* base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win* win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_allocate,
//                      (MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void* base, MPI_Win* win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_allocate_shared,
//                      (MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void* base, MPI_Win* win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_create_dynamic, (MPI_Info info, MPI_Comm comm, MPI_Win* win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_attach, (MPI_Win win, void* base, MPI_Aint size));
// S4BXI_MPI_ONE_IMPLEM(int, Win_detach, (MPI_Win win, const void* base));
// S4BXI_MPI_ONE_IMPLEM(int, Win_set_name, (MPI_Win win, const char* name));
// S4BXI_MPI_ONE_IMPLEM(int, Win_get_name, (MPI_Win win, char* name, int* len));
// S4BXI_MPI_ONE_IMPLEM(int, Win_set_info, (MPI_Win win, MPI_Info info));
// S4BXI_MPI_ONE_IMPLEM(int, Win_get_info, (MPI_Win win, MPI_Info* info));
// S4BXI_MPI_ONE_IMPLEM(int, Win_get_group, (MPI_Win win, MPI_Group* group));
// S4BXI_MPI_ONE_IMPLEM(int, Win_fence, (int assert, MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_get_attr, (MPI_Win type, int type_keyval, void* attribute_val, int* flag));
// S4BXI_MPI_ONE_IMPLEM(int, Win_set_attr, (MPI_Win type, int type_keyval, void* att));
// S4BXI_MPI_ONE_IMPLEM(int, Win_delete_attr, (MPI_Win type, int comm_keyval));
// S4BXI_MPI_ONE_IMPLEM(int, Win_create_keyval,
//                      (MPI_Win_copy_attr_function * copy_fn, MPI_Win_delete_attr_function* delete_fn, int* keyval,
//                       void* extra_state));
// S4BXI_MPI_ONE_IMPLEM(int, Win_free_keyval, (int* keyval));
// S4BXI_MPI_ONE_IMPLEM(int, Win_complete, (MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_post, (MPI_Group group, int assert, MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_start, (MPI_Group group, int assert, MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_wait, (MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_lock, (int lock_type, int rank, int assert, MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_lock_all, (int assert, MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_unlock, (int rank, MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_unlock_all, (MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_flush, (int rank, MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_flush_local, (int rank, MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_flush_all, (MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_flush_local_all, (MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Win_shared_query, (MPI_Win win, int rank, MPI_Aint* size, int* disp_unit, void* baseptr));
// S4BXI_MPI_ONE_IMPLEM(int, Win_sync, (MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(MPI_Win, Win_f2c, (MPI_Fint win));
// S4BXI_MPI_ONE_IMPLEM(MPI_Fint, Win_c2f, (MPI_Win win));

// S4BXI_MPI_ONE_IMPLEM(int, Get,
//                      (void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
//                       MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Put,
//                      (const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
//                       MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win));
// S4BXI_MPI_ONE_IMPLEM(int, Accumulate,
//                      (const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
//                       MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, SETUP_OPS_IN_IMPLEMS(op,
//                       MPI_Win win)))
// S4BXI_MPI_ONE_IMPLEM(int, Get_accumulate,
//                      (const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, void* result_addr,
//                       int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
//                       int target_count, MPI_Datatype target_datatype, SETUP_OPS_IN_IMPLEMS(op, MPI_Win win)))
// S4BXI_MPI_ONE_IMPLEM(int, Rget,
//                      (void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
//                       MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win,
//                       MPI_Request* request));
// S4BXI_MPI_ONE_IMPLEM(int, Rput,
//                      (const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
//                       MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win,
//                       MPI_Request* request));
// S4BXI_MPI_ONE_IMPLEM(int, Raccumulate,
//                      (const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
//                       MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
//                       MPI_Request* request));
// S4BXI_MPI_ONE_IMPLEM(int, Rget_accumulate,
//                      (const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, void* result_addr,
//                       int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
//                       int target_count, MPI_Datatype target_datatype, SETUP_OPS_IN_IMPLEMS(op, MPI_Win win,
//                       MPI_Request* request)))
// S4BXI_MPI_ONE_IMPLEM(int, Fetch_and_op,
//                      (const void* origin_addr, void* result_addr, MPI_Datatype datatype, int target_rank,
//                       MPI_Aint target_disp, SETUP_OPS_IN_IMPLEMS(op, MPI_Win win)))
// S4BXI_MPI_ONE_IMPLEM(int, Compare_and_swap,
//                      (const void* origin_addr, void* compare_addr, void* result_addr, MPI_Datatype datatype,
//                       int target_rank, MPI_Aint target_disp, MPI_Win win));

// S4BXI_MPI_ONE_IMPLEM(int, Cart_coords, (MPI_Comm comm, int rank, int maxdims, int* coords));
// S4BXI_MPI_ONE_IMPLEM(int, Cart_create,
//                      (MPI_Comm comm_old, int ndims, const int* dims, const int* periods, int reorder,
//                       MPI_Comm* comm_cart));
// S4BXI_MPI_ONE_IMPLEM(int, Cart_get, (MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords));
// S4BXI_MPI_ONE_IMPLEM(int, Cart_rank, (MPI_Comm comm, const int* coords, int* rank));
// S4BXI_MPI_ONE_IMPLEM(int, Cart_shift, (MPI_Comm comm, int direction, int displ, int* source, int* dest));
// S4BXI_MPI_ONE_IMPLEM(int, Cart_sub, (MPI_Comm comm, const int* remain_dims, MPI_Comm* comm_new));
// S4BXI_MPI_ONE_IMPLEM(int, Cartdim_get, (MPI_Comm comm, int* ndims));
// S4BXI_MPI_ONE_IMPLEM(int, Dims_create, (int nnodes, int ndims, int* dims));
// S4BXI_MPI_ONE_IMPLEM(int, Request_get_status, (MPI_Request request, int* flag, MPI_Status* status));
// S4BXI_MPI_ONE_IMPLEM(int, Grequest_start,
//                      (MPI_Grequest_query_function * query_fn, MPI_Grequest_free_function* free_fn,
//                       MPI_Grequest_cancel_function* cancel_fn, void* extra_state, MPI_Request* request));
// S4BXI_MPI_ONE_IMPLEM(int, Grequest_complete, (MPI_Request request));
// S4BXI_MPI_ONE_IMPLEM(int, Status_set_cancelled, (MPI_Status * status, int flag));
// S4BXI_MPI_ONE_IMPLEM(int, Status_set_elements, (MPI_Status * status, MPI_Datatype datatype, int count));
// S4BXI_MPI_ONE_IMPLEM(int, Status_set_elements_x, (MPI_Status * status, MPI_Datatype datatype, MPI_Count count));
// S4BXI_MPI_ONE_IMPLEM(int, Type_create_subarray,
//                      (int ndims, const int* array_of_sizes, const int* array_of_subsizes, const int* array_of_starts,
//                       int order, MPI_Datatype oldtype, MPI_Datatype* newtype));

// S4BXI_MPI_ONE_IMPLEM(int, File_open, (MPI_Comm comm, const char* filename, int amode, MPI_Info info, MPI_File* fh));
// S4BXI_MPI_ONE_IMPLEM(int, File_close, (MPI_File * fh));
// S4BXI_MPI_ONE_IMPLEM(int, File_delete, (const char* filename, MPI_Info info));
// S4BXI_MPI_ONE_IMPLEM(int, File_get_size, (MPI_File fh, MPI_Offset* size));
// S4BXI_MPI_ONE_IMPLEM(int, File_get_group, (MPI_File fh, MPI_Group* group));
// S4BXI_MPI_ONE_IMPLEM(int, File_get_amode, (MPI_File fh, int* amode));
// S4BXI_MPI_ONE_IMPLEM(int, File_set_info, (MPI_File fh, MPI_Info info));
// S4BXI_MPI_ONE_IMPLEM(int, File_get_info, (MPI_File fh, MPI_Info* info_used));
// S4BXI_MPI_ONE_IMPLEM(int, File_read_at,
//                      (MPI_File fh, MPI_Offset offset, void* buf, int count, MPI_Datatype datatype, MPI_Status*
//                      status));
// S4BXI_MPI_ONE_IMPLEM(int, File_read_at_all,
//                      (MPI_File fh, MPI_Offset offset, void* buf, int count, MPI_Datatype datatype, MPI_Status*
//                      status));
// S4BXI_MPI_ONE_IMPLEM(int, File_write_at,
//                      (MPI_File fh, MPI_Offset offset, const void* buf, int count, MPI_Datatype datatype,
//                       MPI_Status* status));
// S4BXI_MPI_ONE_IMPLEM(int, File_write_at_all,
//                      (MPI_File fh, MPI_Offset offset, const void* buf, int count, MPI_Datatype datatype,
//                       MPI_Status* status));
// S4BXI_MPI_ONE_IMPLEM(int, File_read, (MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
// S4BXI_MPI_ONE_IMPLEM(int, File_read_all,
//                      (MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
// S4BXI_MPI_ONE_IMPLEM(int, File_write,
//                      (MPI_File fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
// S4BXI_MPI_ONE_IMPLEM(int, File_write_all,
//                      (MPI_File fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
// S4BXI_MPI_ONE_IMPLEM(int, File_seek, (MPI_File fh, MPI_Offset offset, int whenace));
// S4BXI_MPI_ONE_IMPLEM(int, File_get_position, (MPI_File fh, MPI_Offset* offset));
// S4BXI_MPI_ONE_IMPLEM(int, File_read_shared,
//                      (MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
// S4BXI_MPI_ONE_IMPLEM(int, File_write_shared,
//                      (MPI_File fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
// S4BXI_MPI_ONE_IMPLEM(int, File_read_ordered,
//                      (MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
// S4BXI_MPI_ONE_IMPLEM(int, File_write_ordered,
//                      (MPI_File fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
// S4BXI_MPI_ONE_IMPLEM(int, File_seek_shared, (MPI_File fh, MPI_Offset offset, int whence));
// S4BXI_MPI_ONE_IMPLEM(int, File_get_position_shared, (MPI_File fh, MPI_Offset* offset));
// S4BXI_MPI_ONE_IMPLEM(int, File_sync, (MPI_File fh));
// S4BXI_MPI_ONE_IMPLEM(int, File_set_view,
//                      (MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char* datarep,
//                       MPI_Info info));
// S4BXI_MPI_ONE_IMPLEM(int, File_get_view,
//                      (MPI_File fh, MPI_Offset* disp, MPI_Datatype* etype, MPI_Datatype* filetype, char* datarep));

// S4BXI_MPI_ONE_IMPLEM(int, Errhandler_set, (MPI_Comm comm, MPI_Errhandler errhandler));
// S4BXI_MPI_ONE_IMPLEM(int, Errhandler_create, (MPI_Handler_function * function, MPI_Errhandler* errhandler));
// S4BXI_MPI_ONE_IMPLEM(int, Errhandler_free, (MPI_Errhandler * errhandler));
// S4BXI_MPI_ONE_IMPLEM(int, Errhandler_get, (MPI_Comm comm, MPI_Errhandler* errhandler));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_set_errhandler, (MPI_Comm comm, MPI_Errhandler errhandler));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_get_errhandler, (MPI_Comm comm, MPI_Errhandler* errhandler));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_create_errhandler, (MPI_Comm_errhandler_fn * function, MPI_Errhandler* errhandler));
// S4BXI_MPI_ONE_IMPLEM(int, Comm_call_errhandler, (MPI_Comm comm, int errorcode));
// S4BXI_MPI_ONE_IMPLEM(int, Win_set_errhandler, (MPI_Win win, MPI_Errhandler errhandler));
// S4BXI_MPI_ONE_IMPLEM(int, Win_get_errhandler, (MPI_Win win, MPI_Errhandler* errhandler));
// S4BXI_MPI_ONE_IMPLEM(int, Win_create_errhandler, (MPI_Win_errhandler_fn * function, MPI_Errhandler* errhandler));
// S4BXI_MPI_ONE_IMPLEM(int, Win_call_errhandler, (MPI_Win win, int errorcode));
// S4BXI_MPI_ONE_IMPLEM(MPI_Errhandler, Errhandler_f2c, (MPI_Fint errhandler));
// S4BXI_MPI_ONE_IMPLEM(MPI_Fint, Errhandler_c2f, (MPI_Errhandler errhandler));

// S4BXI_MPI_ONE_IMPLEM(int, Type_create_f90_integer, (int count, MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_create_f90_real, (int prec, int exp, MPI_Datatype* newtype));
// S4BXI_MPI_ONE_IMPLEM(int, Type_create_f90_complex, (int prec, int exp, MPI_Datatype* newtype));

// S4BXI_MPI_ONE_IMPLEM(int, Type_get_contents,
//                      (MPI_Datatype datatype, int max_integers, int max_addresses, int max_datatypes,
//                       int* array_of_integers, MPI_Aint* array_of_addresses, MPI_Datatype* array_of_datatypes));
// S4BXI_MPI_ONE_IMPLEM(int, Type_get_envelope,
//                      (MPI_Datatype datatype, int* num_integers, int* num_addresses, int* num_datatypes, int*
//                      combiner));
// S4BXI_MPI_ONE_IMPLEM(int, File_call_errhandler, (MPI_File fh, int errorcode));
// S4BXI_MPI_ONE_IMPLEM(int, File_create_errhandler,
//                      (MPI_File_errhandler_function * function, MPI_Errhandler* errhandler));
// S4BXI_MPI_ONE_IMPLEM(int, File_set_errhandler, (MPI_File file, MPI_Errhandler errhandler));
// S4BXI_MPI_ONE_IMPLEM(int, File_get_errhandler, (MPI_File file, MPI_Errhandler* errhandler));

void set_mpi_middleware_ops(void* bull_libhandle, void* smpi_libhandle)
{
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    main_actor->bull_mpi_ops = make_unique<struct s4bxi_mpi_ops>();
    bool setup_smpi          = smpi_mpi_ops == nullptr;
    if (setup_smpi)
        smpi_mpi_ops = make_unique<struct s4bxi_mpi_ops>();

    SETUP_SYMBOLS_IN_IMPLEMS(Init)
    SETUP_SYMBOLS_IN_IMPLEMS(Finalize)
    SETUP_SYMBOLS_IN_IMPLEMS(Finalized)
    SETUP_SYMBOLS_IN_IMPLEMS(Init_thread)
    SETUP_SYMBOLS_IN_IMPLEMS(Initialized)
    SETUP_SYMBOLS_IN_IMPLEMS(Query_thread)
    SETUP_SYMBOLS_IN_IMPLEMS(Is_thread_main)
    SETUP_SYMBOLS_IN_IMPLEMS(Get_version)
    SETUP_SYMBOLS_IN_IMPLEMS(Get_library_version)
    SETUP_SYMBOLS_IN_IMPLEMS(Get_processor_name)
    SETUP_SYMBOLS_IN_IMPLEMS(Abort)
    SETUP_SYMBOLS_IN_IMPLEMS(Alloc_mem)
    SETUP_SYMBOLS_IN_IMPLEMS(Free_mem)
    SETUP_SYMBOLS_IN_IMPLEMS(Wtime)
    SETUP_SYMBOLS_IN_IMPLEMS(Wtick)
    SETUP_SYMBOLS_IN_IMPLEMS(Buffer_attach)
    SETUP_SYMBOLS_IN_IMPLEMS(Buffer_detach)
    SETUP_SYMBOLS_IN_IMPLEMS(Address)
    SETUP_SYMBOLS_IN_IMPLEMS(Get_address)
    SETUP_SYMBOLS_IN_IMPLEMS(Aint_diff)
    SETUP_SYMBOLS_IN_IMPLEMS(Aint_add)
    SETUP_SYMBOLS_IN_IMPLEMS(Error_class)
    SETUP_SYMBOLS_IN_IMPLEMS(Error_string)
    SETUP_SYMBOLS_IN_IMPLEMS(Attr_delete)
    SETUP_SYMBOLS_IN_IMPLEMS(Attr_get)
    SETUP_SYMBOLS_IN_IMPLEMS(Attr_put)
    SETUP_SYMBOLS_IN_IMPLEMS(Keyval_create)
    SETUP_SYMBOLS_IN_IMPLEMS(Keyval_free)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_free)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_size)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_size_x)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_get_extent)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_get_extent_x)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_get_true_extent)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_get_true_extent_x)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_extent)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_lb)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_ub)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_commit)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_hindexed)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_create_hindexed)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_create_hindexed_block)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_hvector)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_create_hvector)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_indexed)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_create_indexed)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_create_indexed_block)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_struct)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_create_struct)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_vector)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_contiguous)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_create_resized)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_f2c)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_c2f)
    SETUP_SYMBOLS_IN_IMPLEMS(Get_count)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_get_attr)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_set_attr)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_delete_attr)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_create_keyval)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_free_keyval)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_dup)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_set_name)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_get_name)
    SETUP_SYMBOLS_IN_IMPLEMS(Pack)
    SETUP_SYMBOLS_IN_IMPLEMS(Pack_size)
    SETUP_SYMBOLS_IN_IMPLEMS(Unpack)
    SETUP_SYMBOLS_IN_IMPLEMS(Op_create)
    SETUP_SYMBOLS_IN_IMPLEMS(Op_free)
    SETUP_SYMBOLS_IN_IMPLEMS(Op_commutative)
    SETUP_SYMBOLS_IN_IMPLEMS(Op_f2c)
    SETUP_SYMBOLS_IN_IMPLEMS(Op_c2f)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_free)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_size)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_rank)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_translate_ranks)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_compare)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_union)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_intersection)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_difference)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_incl)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_excl)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_range_incl)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_range_excl)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_f2c)
    SETUP_SYMBOLS_IN_IMPLEMS(Group_c2f)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_rank)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_size)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_get_name)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_set_name)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_dup)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_dup_with_info)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_get_attr)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_set_attr)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_delete_attr)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_create_keyval)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_free_keyval)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_group)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_compare)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_create)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_create_group)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_free)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_disconnect)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_split)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_set_info)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_get_info)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_split_type)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_f2c)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_c2f)
    SETUP_SYMBOLS_IN_IMPLEMS(Start)
    SETUP_SYMBOLS_IN_IMPLEMS(Startall)
    SETUP_SYMBOLS_IN_IMPLEMS(Request_free)
    SETUP_SYMBOLS_IN_IMPLEMS(Recv)
    SETUP_SYMBOLS_IN_IMPLEMS(Recv_init)
    SETUP_SYMBOLS_IN_IMPLEMS(Irecv)
    SETUP_SYMBOLS_IN_IMPLEMS(Send)
    SETUP_SYMBOLS_IN_IMPLEMS(Send_init)
    SETUP_SYMBOLS_IN_IMPLEMS(Isend)
    SETUP_SYMBOLS_IN_IMPLEMS(Ssend)
    SETUP_SYMBOLS_IN_IMPLEMS(Ssend_init)
    SETUP_SYMBOLS_IN_IMPLEMS(Issend)
    SETUP_SYMBOLS_IN_IMPLEMS(Bsend)
    SETUP_SYMBOLS_IN_IMPLEMS(Bsend_init)
    SETUP_SYMBOLS_IN_IMPLEMS(Ibsend)
    SETUP_SYMBOLS_IN_IMPLEMS(Rsend)
    SETUP_SYMBOLS_IN_IMPLEMS(Rsend_init)
    SETUP_SYMBOLS_IN_IMPLEMS(Irsend)
    SETUP_SYMBOLS_IN_IMPLEMS(Sendrecv)
    SETUP_SYMBOLS_IN_IMPLEMS(Sendrecv_replace)
    SETUP_SYMBOLS_IN_IMPLEMS(Test)
    SETUP_SYMBOLS_IN_IMPLEMS(Testany)
    SETUP_SYMBOLS_IN_IMPLEMS(Testall)
    SETUP_SYMBOLS_IN_IMPLEMS(Testsome)
    SETUP_SYMBOLS_IN_IMPLEMS(Test_cancelled)
    SETUP_SYMBOLS_IN_IMPLEMS(Wait)
    SETUP_SYMBOLS_IN_IMPLEMS(Waitany)
    SETUP_SYMBOLS_IN_IMPLEMS(Waitall)
    SETUP_SYMBOLS_IN_IMPLEMS(Waitsome)
    SETUP_SYMBOLS_IN_IMPLEMS(Iprobe)
    SETUP_SYMBOLS_IN_IMPLEMS(Probe)
    SETUP_SYMBOLS_IN_IMPLEMS(Request_f2c)
    SETUP_SYMBOLS_IN_IMPLEMS(Request_c2f)
    SETUP_SYMBOLS_IN_IMPLEMS(Cancel)
    SETUP_SYMBOLS_IN_IMPLEMS(Bcast)
    SETUP_SYMBOLS_IN_IMPLEMS(Barrier)
    SETUP_SYMBOLS_IN_IMPLEMS(Ibarrier)
    SETUP_SYMBOLS_IN_IMPLEMS(Ibcast)
    SETUP_SYMBOLS_IN_IMPLEMS(Igather)
    SETUP_SYMBOLS_IN_IMPLEMS(Igatherv)
    SETUP_SYMBOLS_IN_IMPLEMS(Iallgather)
    SETUP_SYMBOLS_IN_IMPLEMS(Iallgatherv)
    SETUP_SYMBOLS_IN_IMPLEMS(Iscatter)
    SETUP_SYMBOLS_IN_IMPLEMS(Iscatterv)
    SETUP_SYMBOLS_IN_IMPLEMS(Ireduce)
    SETUP_SYMBOLS_IN_IMPLEMS(Iallreduce)
    SETUP_SYMBOLS_IN_IMPLEMS(Iscan)
    SETUP_SYMBOLS_IN_IMPLEMS(Iexscan)
    SETUP_SYMBOLS_IN_IMPLEMS(Ireduce_scatter)
    SETUP_SYMBOLS_IN_IMPLEMS(Ireduce_scatter_block)
    SETUP_SYMBOLS_IN_IMPLEMS(Ialltoall)
    SETUP_SYMBOLS_IN_IMPLEMS(Ialltoallv)
    SETUP_SYMBOLS_IN_IMPLEMS(Ialltoallw)
    SETUP_SYMBOLS_IN_IMPLEMS(Gather)
    SETUP_SYMBOLS_IN_IMPLEMS(Gatherv)
    SETUP_SYMBOLS_IN_IMPLEMS(Allgather)
    SETUP_SYMBOLS_IN_IMPLEMS(Allgatherv)
    SETUP_SYMBOLS_IN_IMPLEMS(Scatter)
    SETUP_SYMBOLS_IN_IMPLEMS(Scatterv)
    SETUP_SYMBOLS_IN_IMPLEMS(Reduce)
    SETUP_SYMBOLS_IN_IMPLEMS(Allreduce)
    SETUP_SYMBOLS_IN_IMPLEMS(Scan)
    SETUP_SYMBOLS_IN_IMPLEMS(Exscan)
    SETUP_SYMBOLS_IN_IMPLEMS(Reduce_scatter)
    SETUP_SYMBOLS_IN_IMPLEMS(Reduce_scatter_block)
    SETUP_SYMBOLS_IN_IMPLEMS(Alltoall)
    SETUP_SYMBOLS_IN_IMPLEMS(Alltoallv)
    SETUP_SYMBOLS_IN_IMPLEMS(Alltoallw)
    SETUP_SYMBOLS_IN_IMPLEMS(Reduce_local)
    SETUP_SYMBOLS_IN_IMPLEMS(Info_create)
    SETUP_SYMBOLS_IN_IMPLEMS(Info_set)
    SETUP_SYMBOLS_IN_IMPLEMS(Info_get)
    SETUP_SYMBOLS_IN_IMPLEMS(Info_free)
    SETUP_SYMBOLS_IN_IMPLEMS(Info_delete)
    SETUP_SYMBOLS_IN_IMPLEMS(Info_dup)
    SETUP_SYMBOLS_IN_IMPLEMS(Info_get_nkeys)
    SETUP_SYMBOLS_IN_IMPLEMS(Info_get_nthkey)
    SETUP_SYMBOLS_IN_IMPLEMS(Info_get_valuelen)
    SETUP_SYMBOLS_IN_IMPLEMS(Info_f2c)
    SETUP_SYMBOLS_IN_IMPLEMS(Info_c2f)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_free)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_create)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_allocate)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_allocate_shared)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_create_dynamic)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_attach)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_detach)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_set_name)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_get_name)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_set_info)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_get_info)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_get_group)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_fence)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_get_attr)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_set_attr)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_delete_attr)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_create_keyval)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_free_keyval)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_complete)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_post)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_start)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_wait)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_lock)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_lock_all)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_unlock)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_unlock_all)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_flush)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_flush_local)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_flush_all)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_flush_local_all)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_shared_query)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_sync)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_f2c)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_c2f)
    SETUP_SYMBOLS_IN_IMPLEMS(Get)
    SETUP_SYMBOLS_IN_IMPLEMS(Put)
    SETUP_SYMBOLS_IN_IMPLEMS(Accumulate)
    SETUP_SYMBOLS_IN_IMPLEMS(Get_accumulate)
    SETUP_SYMBOLS_IN_IMPLEMS(Rget)
    SETUP_SYMBOLS_IN_IMPLEMS(Rput)
    SETUP_SYMBOLS_IN_IMPLEMS(Raccumulate)
    SETUP_SYMBOLS_IN_IMPLEMS(Rget_accumulate)
    SETUP_SYMBOLS_IN_IMPLEMS(Fetch_and_op)
    SETUP_SYMBOLS_IN_IMPLEMS(Compare_and_swap)
    SETUP_SYMBOLS_IN_IMPLEMS(Cart_coords)
    SETUP_SYMBOLS_IN_IMPLEMS(Cart_create)
    SETUP_SYMBOLS_IN_IMPLEMS(Cart_get)
    SETUP_SYMBOLS_IN_IMPLEMS(Cart_rank)
    SETUP_SYMBOLS_IN_IMPLEMS(Cart_shift)
    SETUP_SYMBOLS_IN_IMPLEMS(Cart_sub)
    SETUP_SYMBOLS_IN_IMPLEMS(Cartdim_get)
    SETUP_SYMBOLS_IN_IMPLEMS(Dims_create)
    SETUP_SYMBOLS_IN_IMPLEMS(Request_get_status)
    SETUP_SYMBOLS_IN_IMPLEMS(Grequest_start)
    SETUP_SYMBOLS_IN_IMPLEMS(Grequest_complete)
    SETUP_SYMBOLS_IN_IMPLEMS(Status_set_cancelled)
    SETUP_SYMBOLS_IN_IMPLEMS(Status_set_elements)
    SETUP_SYMBOLS_IN_IMPLEMS(Status_set_elements_x)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_create_subarray)
    SETUP_SYMBOLS_IN_IMPLEMS(File_open)
    SETUP_SYMBOLS_IN_IMPLEMS(File_close)
    SETUP_SYMBOLS_IN_IMPLEMS(File_delete)
    SETUP_SYMBOLS_IN_IMPLEMS(File_get_size)
    SETUP_SYMBOLS_IN_IMPLEMS(File_get_group)
    SETUP_SYMBOLS_IN_IMPLEMS(File_get_amode)
    SETUP_SYMBOLS_IN_IMPLEMS(File_set_info)
    SETUP_SYMBOLS_IN_IMPLEMS(File_get_info)
    SETUP_SYMBOLS_IN_IMPLEMS(File_read_at)
    SETUP_SYMBOLS_IN_IMPLEMS(File_read_at_all)
    SETUP_SYMBOLS_IN_IMPLEMS(File_write_at)
    SETUP_SYMBOLS_IN_IMPLEMS(File_write_at_all)
    SETUP_SYMBOLS_IN_IMPLEMS(File_read)
    SETUP_SYMBOLS_IN_IMPLEMS(File_read_all)
    SETUP_SYMBOLS_IN_IMPLEMS(File_write)
    SETUP_SYMBOLS_IN_IMPLEMS(File_write_all)
    SETUP_SYMBOLS_IN_IMPLEMS(File_seek)
    SETUP_SYMBOLS_IN_IMPLEMS(File_get_position)
    SETUP_SYMBOLS_IN_IMPLEMS(File_read_shared)
    SETUP_SYMBOLS_IN_IMPLEMS(File_write_shared)
    SETUP_SYMBOLS_IN_IMPLEMS(File_read_ordered)
    SETUP_SYMBOLS_IN_IMPLEMS(File_write_ordered)
    SETUP_SYMBOLS_IN_IMPLEMS(File_seek_shared)
    SETUP_SYMBOLS_IN_IMPLEMS(File_get_position_shared)
    SETUP_SYMBOLS_IN_IMPLEMS(File_sync)
    SETUP_SYMBOLS_IN_IMPLEMS(File_set_view)
    SETUP_SYMBOLS_IN_IMPLEMS(File_get_view)
    SETUP_SYMBOLS_IN_IMPLEMS(Errhandler_set)
    SETUP_SYMBOLS_IN_IMPLEMS(Errhandler_create)
    SETUP_SYMBOLS_IN_IMPLEMS(Errhandler_free)
    SETUP_SYMBOLS_IN_IMPLEMS(Errhandler_get)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_set_errhandler)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_get_errhandler)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_create_errhandler)
    SETUP_SYMBOLS_IN_IMPLEMS(Comm_call_errhandler)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_set_errhandler)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_get_errhandler)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_create_errhandler)
    SETUP_SYMBOLS_IN_IMPLEMS(Win_call_errhandler)
    SETUP_SYMBOLS_IN_IMPLEMS(Errhandler_f2c)
    SETUP_SYMBOLS_IN_IMPLEMS(Errhandler_c2f)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_create_f90_integer)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_create_f90_real)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_create_f90_complex)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_get_contents)
    SETUP_SYMBOLS_IN_IMPLEMS(Type_get_envelope)
    SETUP_SYMBOLS_IN_IMPLEMS(File_call_errhandler)
    SETUP_SYMBOLS_IN_IMPLEMS(File_create_errhandler)
    SETUP_SYMBOLS_IN_IMPLEMS(File_set_errhandler)
    SETUP_SYMBOLS_IN_IMPLEMS(File_get_errhandler)

    // Communicators

    SETUP_COMMS_IN_IMPLEMS(world, WORLD)
    SETUP_COMMS_IN_IMPLEMS(self, SELF)

    // Datatypes
    SETUP_DATATYPES_IN_IMPLEMS(char, CHAR)
    SETUP_DATATYPES_IN_IMPLEMS(datatype_null, DATATYPE_NULL)
    SETUP_DATATYPES_IN_IMPLEMS(char, CHAR)
    SETUP_DATATYPES_IN_IMPLEMS(short, SHORT)
    SETUP_DATATYPES_IN_IMPLEMS(int, INT)
    SETUP_DATATYPES_IN_IMPLEMS(long, LONG)
    SETUP_DATATYPES_IN_IMPLEMS(long_long_int, LONG_LONG)
    SETUP_DATATYPES_IN_IMPLEMS(signed_char, SIGNED_CHAR)
    SETUP_DATATYPES_IN_IMPLEMS(unsigned_char, UNSIGNED_CHAR)
    SETUP_DATATYPES_IN_IMPLEMS(unsigned_short, UNSIGNED_SHORT)
    SETUP_DATATYPES_IN_IMPLEMS(unsigned, UNSIGNED)
    SETUP_DATATYPES_IN_IMPLEMS(unsigned_long, UNSIGNED_LONG)
    SETUP_DATATYPES_IN_IMPLEMS(unsigned_long_long, UNSIGNED_LONG_LONG)
    SETUP_DATATYPES_IN_IMPLEMS(float, FLOAT)
    SETUP_DATATYPES_IN_IMPLEMS(double, DOUBLE)
    SETUP_DATATYPES_IN_IMPLEMS(long_double, LONG_DOUBLE)
    SETUP_DATATYPES_IN_IMPLEMS(wchar, WCHAR)
    SETUP_DATATYPES_IN_IMPLEMS(c_bool, C_BOOL)
    SETUP_DATATYPES_IN_IMPLEMS(int8_t, INT8_T)
    SETUP_DATATYPES_IN_IMPLEMS(int16_t, INT16_T)
    SETUP_DATATYPES_IN_IMPLEMS(int32_t, INT32_T)
    SETUP_DATATYPES_IN_IMPLEMS(int64_t, INT64_T)
    SETUP_DATATYPES_IN_IMPLEMS(uint8_t, UINT8_T)
    SETUP_DATATYPES_IN_IMPLEMS(byte, BYTE)
    SETUP_DATATYPES_IN_IMPLEMS(uint16_t, UINT16_T)
    SETUP_DATATYPES_IN_IMPLEMS(uint32_t, UINT32_T)
    SETUP_DATATYPES_IN_IMPLEMS(uint64_t, UINT64_T)
    SETUP_DATATYPES_IN_IMPLEMS(c_float_complex, C_FLOAT_COMPLEX)
    SETUP_DATATYPES_IN_IMPLEMS(c_double_complex, C_DOUBLE_COMPLEX)
    SETUP_DATATYPES_IN_IMPLEMS(c_long_double_complex, C_LONG_DOUBLE_COMPLEX)
    SETUP_DATATYPES_IN_IMPLEMS(aint, AINT)
    SETUP_DATATYPES_IN_IMPLEMS(offset, OFFSET)
    SETUP_DATATYPES_IN_IMPLEMS(lb, LB)
    SETUP_DATATYPES_IN_IMPLEMS(ub, UB)
    SETUP_DATATYPES_IN_IMPLEMS(float_int, FLOAT_INT)
    SETUP_DATATYPES_IN_IMPLEMS(long_int, LONG_INT)
    SETUP_DATATYPES_IN_IMPLEMS(double_int, DOUBLE_INT)
    SETUP_DATATYPES_IN_IMPLEMS(short_int, SHORT_INT)
    SETUP_DATATYPES_IN_IMPLEMS(2int, 2INT)
    SETUP_DATATYPES_IN_IMPLEMS(longdbl_int, LONG_DOUBLE_INT)

    // These don't exist in Bull implem ?
    // SETUP_DATATYPES_IN_IMPLEMS(2float, 2FLOAT)
    // SETUP_DATATYPES_IN_IMPLEMS(2double, 2DOUBLE)
    // SETUP_DATATYPES_IN_IMPLEMS(2long, 2LONG)

    SETUP_DATATYPES_IN_IMPLEMS(real, REAL)
    SETUP_DATATYPES_IN_IMPLEMS(real4, REAL4)
    SETUP_DATATYPES_IN_IMPLEMS(real8, REAL8)
    SETUP_DATATYPES_IN_IMPLEMS(real16, REAL16)
    SETUP_DATATYPES_IN_IMPLEMS(complex8, COMPLEX8)
    SETUP_DATATYPES_IN_IMPLEMS(complex16, COMPLEX16)
    SETUP_DATATYPES_IN_IMPLEMS(complex32, COMPLEX32)
    SETUP_DATATYPES_IN_IMPLEMS(integer1, INTEGER1)
    SETUP_DATATYPES_IN_IMPLEMS(integer2, INTEGER2)
    SETUP_DATATYPES_IN_IMPLEMS(integer4, INTEGER4)
    SETUP_DATATYPES_IN_IMPLEMS(integer8, INTEGER8)
    SETUP_DATATYPES_IN_IMPLEMS(integer16, INTEGER16)
    SETUP_DATATYPES_IN_IMPLEMS(count, COUNT)

    // Operations
    SETUP_OPS_IN_IMPLEMS(max, MAX)
    SETUP_OPS_IN_IMPLEMS(min, MIN)
    SETUP_OPS_IN_IMPLEMS(maxloc, MAXLOC)
    SETUP_OPS_IN_IMPLEMS(minloc, MINLOC)
    SETUP_OPS_IN_IMPLEMS(sum, SUM)
    SETUP_OPS_IN_IMPLEMS(prod, PROD)
    SETUP_OPS_IN_IMPLEMS(land, LAND)
    SETUP_OPS_IN_IMPLEMS(lor, LOR)
    SETUP_OPS_IN_IMPLEMS(lxor, LXOR)
    SETUP_OPS_IN_IMPLEMS(band, BAND)
    SETUP_OPS_IN_IMPLEMS(bor, BOR)
    SETUP_OPS_IN_IMPLEMS(bxor, BXOR)
    SETUP_OPS_IN_IMPLEMS(replace, REPLACE)
    SETUP_OPS_IN_IMPLEMS(no_op, NO_OP)

    // Request
    main_actor->bull_mpi_ops->REQUEST_NULL = (MPI_Request)dlsym(bull_libhandle, "ompi_request_null");
    assert(main_actor->bull_mpi_ops->REQUEST_NULL != nullptr);
}

#undef S4BXI_MPI_CALL
