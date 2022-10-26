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

#include <dlfcn.h>
#include <fcntl.h>

#include "s4bxi/mpi_middleware/s4bxi_mpi_middleware.h"
#include "s4bxi/s4bxi_xbt_log.h"
#include "s4bxi/actors/BxiMainActor.hpp"
#include "s4bxi/s4bxi_util.hpp"
#include "s4bxi/mpi_middleware/BxiMpiComm.hpp"
#include "s4bxi/mpi_middleware/BxiMpiDatatype.hpp"
#include "s4bxi/s4bxi_bench.h"
#include <smpi_actor.hpp>
#include <unordered_set>

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_mpi_middlware, "Messages generated in MPI middleware");

using namespace std;

MPI_Datatype* type_array;

unique_ptr<struct s4bxi_mpi_ops> smpi_mpi_ops = nullptr;

unordered_set<MPI_Request> smpi_requests;

void s4bxi_log_pending_requests()
{
    s4bxi_fprintf(stderr, ">>> Pending requests in middleware: %d\n", smpi_requests.size());
}

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

int s4bxi_is_mpi_request_null(MPI_Request r)
{
    return r == NULL || r == GET_CURRENT_MAIN_ACTOR->bull_mpi_ops->REQUEST_NULL;
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

static bool is_smpi_request(const MPI_Request& r)
{
    if (r == NULL) // SMPI's MPI_REQUEST_NULL
        return true;

    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    if (r == main_actor->bull_mpi_ops->REQUEST_NULL) // Bull's MPI_REQUEST_NULL
        return false;

    return smpi_requests.find(r) != smpi_requests.end();
}

MPI_Datatype* implem_datatypes(const MPI_Datatype* original, int size, MPI_Datatype* out)
{
    for (int i = 0; i < size; ++i)
        out[i] = BxiMpiDatatype::implem_datatype(original[i]);

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

#ifdef LOG_MPI_CALLS
#define LOG_CALL(name, file, line)                                                                                     \
    do {                                                                                                               \
        s4bxi_fprintf(stderr, "Calling " #name " from line %d in file %s\n", line, file);                              \
        /* xbt_backtrace_display_current(); */                                                                         \
    } while (0)
#else
#define LOG_CALL(name, file, line)                                                                                     \
    do {                                                                                                               \
    } while (0)
#endif

#define S4BXI_MPI_ONE_IMPLEM(rtype, name, argsval, ...)                                                                \
    typedef rtype (*name##_func)(__VA_ARGS__);                                                                         \
    rtype S4BXI_MPI_##name(const char* __file, int __line, ##__VA_ARGS__)                                              \
    {                                                                                                                  \
        LOG_CALL(name, __file, __line);                                                                                \
        BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;                                                             \
        return ((name##_func)(main_actor->use_smpi_implem ? smpi_mpi_ops : main_actor->bull_mpi_ops)->name)argsval;    \
    }

#define S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(rtype, name, argsval, ...)                                                  \
    typedef rtype (*name##_func)(__VA_ARGS__);                                                                         \
    rtype S4BXI_MPI_##name(const char* __file, int __line, ##__VA_ARGS__)                                              \
    {                                                                                                                  \
        LOG_CALL(name, __file, __line);                                                                                \
        BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;                                                             \
        bool smpi                = false;                                                                              \
        if (main_actor->use_smpi_implem)                                                                               \
            smpi = true;                                                                                               \
                                                                                                                       \
        rtype out = ((name##_func)(smpi ? smpi_mpi_ops : main_actor->bull_mpi_ops)->name)argsval;                      \
        s4bxi_bench_end();                                                                                             \
        if (smpi) {                                                                                                    \
            smpi_requests.emplace(*request);                                                                           \
        }                                                                                                              \
        s4bxi_bench_begin();                                                                                           \
                                                                                                                       \
        return out;                                                                                                    \
    }

#define S4BXI_MPI_BULL_IMPLEM(rtype, name, argsval, ...)                                                               \
    typedef rtype (*name##_func)(__VA_ARGS__);                                                                         \
    rtype S4BXI_MPI_##name(const char* __file, int __line, ##__VA_ARGS__)                                              \
    {                                                                                                                  \
        LOG_CALL(name, __file, __line);                                                                                \
        BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;                                                             \
        return ((name##_func)(main_actor->bull_mpi_ops)->name)argsval;                                                 \
    }

#define S4BXI_MPI_UNSUPPORTED(rtype, name, ...)                                                                        \
    typedef rtype (*name##_func)(__VA_ARGS__);                                                                         \
    rtype S4BXI_MPI_##name(const char* __file, int __line, ##__VA_ARGS__)                                              \
    {                                                                                                                  \
        ptl_panic("Unsupported function " #name " intercepted in MPI middleware");                                     \
    }

// This one is kind of specific, it's only used for [I]alltoallw because of the datatype arrays
#define S4BXI_MPI_W_COLLECTIVE(rtype, name, argsval, ...)                                                              \
    typedef rtype (*name##_func)(__VA_ARGS__);                                                                         \
    rtype S4BXI_MPI_##name(const char* __file, int __line, ##__VA_ARGS__)                                              \
    {                                                                                                                  \
        LOG_CALL(name, __file, __line);                                                                                \
        BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;                                                             \
        int size;                                                                                                      \
        S4BXI_MPI_Comm_size(__file, __line, comm, &size);                                                              \
        MPI_Datatype sendtypes_arr[size];                                                                              \
        MPI_Datatype recvtypes_arr[size];                                                                              \
        return ((name##_func)(main_actor->use_smpi_implem ? smpi_mpi_ops : main_actor->bull_mpi_ops)->name)argsval;    \
    }

typedef int (*Init_func)(int* argc, char*** argv);
int S4BXI_MPI_Init(const char* __file, int __line, int* argc, char*** argv)
{
    LOG_CALL(Init, __file, __line);

    smpi_bench_end();

    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    int smpi                 = ((Init_func)smpi_mpi_ops->Init)(argc, argv); // This does smpi_bench_begin
    int bull                 = ((Init_func)main_actor->bull_mpi_ops->Init)(argc, argv);

    return bull > smpi ? bull : smpi;
}

typedef int (*Finalize_func)();
int S4BXI_MPI_Finalize(const char* __file, int __line)
{
    LOG_CALL(Finalize, __file, __line);

    s4bxi_barrier();

    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    int smpi                 = ((Finalize_func)smpi_mpi_ops->Finalize)(); // This does smpi_bench_end
    smpi_bench_begin();

    int bull                 = ((Finalize_func)main_actor->bull_mpi_ops->Finalize)();

    return bull > smpi ? bull : smpi;
}

S4BXI_MPI_UNSUPPORTED(int, Finalized, int* flag)
S4BXI_MPI_UNSUPPORTED(int, Init_thread, int* argc, char*** argv, int required, int* provided)
S4BXI_MPI_UNSUPPORTED(int, Initialized, int* flag)
S4BXI_MPI_UNSUPPORTED(int, Query_thread, int* provided)
S4BXI_MPI_UNSUPPORTED(int, Is_thread_main, int* flag)
S4BXI_MPI_BULL_IMPLEM(int, Get_version, (version, subversion), int* version, int* subversion)
S4BXI_MPI_UNSUPPORTED(int, Get_library_version, char* version, int* len)
S4BXI_MPI_ONE_IMPLEM(int, Get_processor_name, (name, resultlen), char* name, int* resultlen)
S4BXI_MPI_UNSUPPORTED(int, Abort, /* (BxiMpiComm::implem_comm(comm), errorcode), */ MPI_Comm comm, int errorcode)
S4BXI_MPI_UNSUPPORTED(int, Alloc_mem, MPI_Aint size, MPI_Info info, void* baseptr)
S4BXI_MPI_UNSUPPORTED(int, Free_mem, void* base)
S4BXI_MPI_ONE_IMPLEM(double, Wtime, ())
S4BXI_MPI_ONE_IMPLEM(double, Wtick, ())
S4BXI_MPI_UNSUPPORTED(int, Buffer_attach, void* buffer, int size)
S4BXI_MPI_UNSUPPORTED(int, Buffer_detach, void* buffer, int* size)
S4BXI_MPI_UNSUPPORTED(int, Address, const void* location, MPI_Aint* address)
S4BXI_MPI_UNSUPPORTED(int, Get_address, const void* location, MPI_Aint* address)
S4BXI_MPI_UNSUPPORTED(MPI_Aint, Aint_diff, MPI_Aint base, MPI_Aint disp)
S4BXI_MPI_UNSUPPORTED(MPI_Aint, Aint_add, MPI_Aint base, MPI_Aint disp)
S4BXI_MPI_UNSUPPORTED(int, Error_class, int errorcode, int* errorclass)
S4BXI_MPI_UNSUPPORTED(int, Error_string, int errorcode, char* string, int* resultlen)

S4BXI_MPI_UNSUPPORTED(int, Attr_delete, MPI_Comm comm, int keyval)
S4BXI_MPI_UNSUPPORTED(int, Attr_get, MPI_Comm comm, int keyval, void* attr_value, int* flag)
S4BXI_MPI_UNSUPPORTED(int, Attr_put, MPI_Comm comm, int keyval, void* attr_value)
S4BXI_MPI_UNSUPPORTED(int, Keyval_create, MPI_Copy_function* copy_fn, MPI_Delete_function* delete_fn, int* keyval,
                      void* extra_state)
S4BXI_MPI_UNSUPPORTED(int, Keyval_free, int* keyval)

typedef int (*Type_free_func)(MPI_Datatype* datatype);
int S4BXI_MPI_Type_free(const char* __file, int __line, MPI_Datatype* datatype)
{
    LOG_CALL(Type_free, __file, __line);
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    // We should probably check that the comm is not a predefined one, but I feel that it's the user app problem if
    // someone tried to free one of those...
    auto s4bxi_type = (BxiMpiDatatype*)(*datatype);
    int bull        = ((Type_free_func)(main_actor->bull_mpi_ops->Type_free))(&s4bxi_type->bull);
    int smpi        = ((Type_free_func)(smpi_mpi_ops->Type_free))(&s4bxi_type->smpi);
    delete s4bxi_type;

    return bull > smpi ? bull : smpi;
}
S4BXI_MPI_UNSUPPORTED(int, Type_size, MPI_Datatype datatype, int* size)
S4BXI_MPI_UNSUPPORTED(int, Type_size_x, MPI_Datatype datatype, MPI_Count* size)
S4BXI_MPI_UNSUPPORTED(int, Type_get_extent, MPI_Datatype datatype, MPI_Aint* lb, MPI_Aint* extent)
S4BXI_MPI_UNSUPPORTED(int, Type_get_extent_x, MPI_Datatype datatype, MPI_Count* lb, MPI_Count* extent)
S4BXI_MPI_UNSUPPORTED(int, Type_get_true_extent, MPI_Datatype datatype, MPI_Aint* lb, MPI_Aint* extent)
S4BXI_MPI_UNSUPPORTED(int, Type_get_true_extent_x, MPI_Datatype datatype, MPI_Count* lb, MPI_Count* extent)
S4BXI_MPI_UNSUPPORTED(int, Type_extent, MPI_Datatype datatype, MPI_Aint* extent)
S4BXI_MPI_UNSUPPORTED(int, Type_lb, MPI_Datatype datatype, MPI_Aint* disp)
S4BXI_MPI_UNSUPPORTED(int, Type_ub, MPI_Datatype datatype, MPI_Aint* disp)
typedef int (*Type_commit_func)(MPI_Datatype* datatype);
int S4BXI_MPI_Type_commit(const char* __file, int __line, MPI_Datatype* datatype)
{
    LOG_CALL(Type_commit, __file, __line);
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    auto s4bxi_type          = (BxiMpiDatatype*)(*datatype);
    int bull                 = ((Type_commit_func)(main_actor->bull_mpi_ops->Type_commit))(&s4bxi_type->bull);
    int smpi                 = ((Type_commit_func)(smpi_mpi_ops->Type_commit))(&s4bxi_type->smpi);

    return bull > smpi ? bull : smpi;
}
S4BXI_MPI_UNSUPPORTED(int, Type_hindexed, int count, const int* blocklens, const MPI_Aint* indices,
                      MPI_Datatype old_type, MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_create_hindexed, int count, const int* blocklens, const MPI_Aint* indices,
                      MPI_Datatype old_type, MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_create_hindexed_block, int count, int blocklength, const MPI_Aint* indices,
                      MPI_Datatype old_type, MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_hvector, int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type,
                      MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_create_hvector, int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type,
                      MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_indexed, int count, const int* blocklens, const int* indices, MPI_Datatype old_type,
                      MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_create_indexed, int count, const int* blocklens, const int* indices,
                      MPI_Datatype old_type, MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_create_indexed_block, int count, int blocklength, const int* indices,
                      MPI_Datatype old_type, MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_struct, int count, const int* blocklens, const MPI_Aint* indices,
                      const MPI_Datatype* old_types, MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_create_struct, int count, const int* blocklens, const MPI_Aint* indices,
                      const MPI_Datatype* old_types, MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_vector, int count, int blocklen, int stride, MPI_Datatype old_type,
                      MPI_Datatype* newtype)
typedef int (*Type_contiguous_func)(int count, MPI_Datatype old_type, MPI_Datatype* newtype);
int S4BXI_MPI_Type_contiguous(const char* __file, int __line, int count, MPI_Datatype old_type, MPI_Datatype* newtype)
{
    LOG_CALL(Type_contiguous, __file, __line);
    BxiMainActor* main_actor  = GET_CURRENT_MAIN_ACTOR;
    MPI_Datatype type_in_bull = BxiMpiDatatype::bull_datatype(old_type);
    MPI_Datatype type_in_smpi = BxiMpiDatatype::smpi_datatype(old_type);
    MPI_Datatype type_out_bull;
    MPI_Datatype type_out_smpi;

    int bull = ((Type_contiguous_func)(main_actor->bull_mpi_ops->Type_contiguous))(count, type_in_bull, &type_out_bull);
    int smpi = ((Type_contiguous_func)(smpi_mpi_ops->Type_contiguous))(count, type_in_smpi, &type_out_smpi);

    *newtype = (MPI_Datatype)(new BxiMpiDatatype(type_out_bull, type_out_smpi));

    return bull > smpi ? bull : smpi;
}
S4BXI_MPI_UNSUPPORTED(int, Type_create_resized, MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                      MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(MPI_Datatype, Type_f2c, MPI_Fint datatype)
S4BXI_MPI_UNSUPPORTED(MPI_Fint, Type_c2f, MPI_Datatype datatype)
S4BXI_MPI_UNSUPPORTED(int, Get_count, const MPI_Status* status, MPI_Datatype datatype, int* count)
S4BXI_MPI_UNSUPPORTED(int, Type_get_attr, MPI_Datatype type, int type_keyval, void* attribute_val, int* flag)
S4BXI_MPI_UNSUPPORTED(int, Type_set_attr, MPI_Datatype type, int type_keyval, void* att)
S4BXI_MPI_UNSUPPORTED(int, Type_delete_attr, MPI_Datatype type, int comm_keyval)
S4BXI_MPI_UNSUPPORTED(int, Type_create_keyval, MPI_Type_copy_attr_function* copy_fn,
                      MPI_Type_delete_attr_function* delete_fn, int* keyval, void* extra_state)
S4BXI_MPI_UNSUPPORTED(int, Type_free_keyval, int* keyval)
S4BXI_MPI_UNSUPPORTED(int, Type_dup, MPI_Datatype datatype, MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_set_name, MPI_Datatype datatype, const char* name)
S4BXI_MPI_UNSUPPORTED(int, Type_get_name, MPI_Datatype datatype, char* name, int* len)

S4BXI_MPI_UNSUPPORTED(int, Pack, const void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount,
                      int* position, MPI_Comm comm)
S4BXI_MPI_UNSUPPORTED(int, Pack_size, int incount, MPI_Datatype datatype, MPI_Comm comm, int* size)
S4BXI_MPI_UNSUPPORTED(int, Unpack, const void* inbuf, int insize, int* position, void* outbuf, int outcount,
                      MPI_Datatype type, MPI_Comm comm)

S4BXI_MPI_UNSUPPORTED(int, Op_create, MPI_User_function* function, int commute, MPI_Op* op)
S4BXI_MPI_UNSUPPORTED(int, Op_free, MPI_Op* op)
S4BXI_MPI_UNSUPPORTED(int, Op_commutative, MPI_Op op, int* commute)
S4BXI_MPI_UNSUPPORTED(MPI_Op, Op_f2c, MPI_Fint op)
S4BXI_MPI_UNSUPPORTED(MPI_Fint, Op_c2f, MPI_Op op)

S4BXI_MPI_UNSUPPORTED(int, Group_free, MPI_Group* group)
S4BXI_MPI_UNSUPPORTED(int, Group_size, MPI_Group group, int* size)
S4BXI_MPI_UNSUPPORTED(int, Group_rank, MPI_Group group, int* rank)
S4BXI_MPI_UNSUPPORTED(int, Group_translate_ranks, MPI_Group group1, int n, const int* ranks1, MPI_Group group2,
                      int* ranks2)
S4BXI_MPI_UNSUPPORTED(int, Group_compare, MPI_Group group1, MPI_Group group2, int* result)
S4BXI_MPI_UNSUPPORTED(int, Group_union, MPI_Group group1, MPI_Group group2, MPI_Group* newgroup)
S4BXI_MPI_UNSUPPORTED(int, Group_intersection, MPI_Group group1, MPI_Group group2, MPI_Group* newgroup)
S4BXI_MPI_UNSUPPORTED(int, Group_difference, MPI_Group group1, MPI_Group group2, MPI_Group* newgroup)
S4BXI_MPI_UNSUPPORTED(int, Group_incl, MPI_Group group, int n, const int* ranks, MPI_Group* newgroup)
S4BXI_MPI_UNSUPPORTED(int, Group_excl, MPI_Group group, int n, const int* ranks, MPI_Group* newgroup)
S4BXI_MPI_UNSUPPORTED(int, Group_range_incl, MPI_Group group, int n, int ranges[][3], MPI_Group* newgroup)
S4BXI_MPI_UNSUPPORTED(int, Group_range_excl, MPI_Group group, int n, int ranges[][3], MPI_Group* newgroup)
S4BXI_MPI_UNSUPPORTED(MPI_Group, Group_f2c, MPI_Fint group)
S4BXI_MPI_UNSUPPORTED(MPI_Fint, Group_c2f, MPI_Group group)

S4BXI_MPI_ONE_IMPLEM(int, Comm_rank, (BxiMpiComm::implem_comm(comm), rank), MPI_Comm comm, int* rank)
S4BXI_MPI_ONE_IMPLEM(int, Comm_size, (BxiMpiComm::implem_comm(comm), size), MPI_Comm comm, int* size)
S4BXI_MPI_UNSUPPORTED(int, Comm_get_name, MPI_Comm comm, char* name, int* len)
S4BXI_MPI_UNSUPPORTED(int, Comm_set_name, MPI_Comm comm, const char* name)
S4BXI_MPI_UNSUPPORTED(int, Comm_dup, MPI_Comm comm, MPI_Comm* newcomm)
S4BXI_MPI_UNSUPPORTED(int, Comm_dup_with_info, MPI_Comm comm, MPI_Info info, MPI_Comm* newcomm)
S4BXI_MPI_UNSUPPORTED(int, Comm_get_attr, MPI_Comm comm, int comm_keyval, void* attribute_val, int* flag)
S4BXI_MPI_UNSUPPORTED(int, Comm_set_attr, MPI_Comm comm, int comm_keyval, void* attribute_val)
S4BXI_MPI_UNSUPPORTED(int, Comm_delete_attr, MPI_Comm comm, int comm_keyval)
S4BXI_MPI_UNSUPPORTED(int, Comm_create_keyval, MPI_Comm_copy_attr_function* copy_fn,
                      MPI_Comm_delete_attr_function* delete_fn, int* keyval, void* extra_state)
S4BXI_MPI_UNSUPPORTED(int, Comm_free_keyval, int* keyval)
S4BXI_MPI_UNSUPPORTED(int, Comm_group, MPI_Comm comm, MPI_Group* group)
S4BXI_MPI_UNSUPPORTED(int, Comm_compare, MPI_Comm comm1, MPI_Comm comm2, int* result)
S4BXI_MPI_UNSUPPORTED(int, Comm_create, MPI_Comm comm, MPI_Group group, MPI_Comm* newcomm)
S4BXI_MPI_UNSUPPORTED(int, Comm_create_group, MPI_Comm comm, MPI_Group group, int tag, MPI_Comm* newcomm)
typedef int (*Comm_free_func)(MPI_Comm* comm);
int S4BXI_MPI_Comm_free(const char* __file, int __line, MPI_Comm* comm)
{
    LOG_CALL(Comm_free, __file, __line);
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    // We should probably check that the comm is not WORLD or SELF, but I feel that it's the user app problem if someone
    // tried to free WORLD or SELF...
    auto s4bxi_comm = (BxiMpiComm*)(*comm);
    int bull        = ((Comm_free_func)(main_actor->bull_mpi_ops->Comm_free))(&s4bxi_comm->bull);
    int smpi        = ((Comm_free_func)(smpi_mpi_ops->Comm_free))(&s4bxi_comm->smpi);
    delete s4bxi_comm;

    return bull > smpi ? bull : smpi;
}
S4BXI_MPI_UNSUPPORTED(int, Comm_disconnect, MPI_Comm* comm)
typedef int (*Comm_split_func)(MPI_Comm comm, int color, int key, MPI_Comm* comm_out);
int S4BXI_MPI_Comm_split(const char* __file, int __line, MPI_Comm comm, int color, int key, MPI_Comm* comm_out)
{
    LOG_CALL(Comm_split, __file, __line);
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
S4BXI_MPI_UNSUPPORTED(int, Comm_set_info, MPI_Comm comm, MPI_Info info)
S4BXI_MPI_UNSUPPORTED(int, Comm_get_info, MPI_Comm comm, MPI_Info* info)
S4BXI_MPI_UNSUPPORTED(int, Comm_split_type, MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm* newcomm)
S4BXI_MPI_UNSUPPORTED(MPI_Comm, Comm_f2c, MPI_Fint comm)
S4BXI_MPI_UNSUPPORTED(MPI_Fint, Comm_c2f, MPI_Comm comm)

S4BXI_MPI_ONE_IMPLEM(int, Start, (request), MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM(int, Startall, (count, requests), int count, MPI_Request* requests)
S4BXI_MPI_ONE_IMPLEM(int, Request_free, (request), MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM(int, Recv,
                     (buf, count, BxiMpiDatatype::implem_datatype(datatype), src, tag, BxiMpiComm::implem_comm(comm),
                      status),
                     void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status* status)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Recv_init,
                                   (buf, count, BxiMpiDatatype::implem_datatype(datatype), src, tag,
                                    BxiMpiComm::implem_comm(comm), request),
                                   void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
                                   MPI_Request* request);
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Irecv,
                                   (buf, count, BxiMpiDatatype::implem_datatype(datatype), src, tag,
                                    BxiMpiComm::implem_comm(comm), request),
                                   void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
                                   MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM(int, Send,
                     (buf, count, BxiMpiDatatype::implem_datatype(datatype), dst, tag, BxiMpiComm::implem_comm(comm)),
                     const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Send_init,
                                   (buf, count, BxiMpiDatatype::implem_datatype(datatype), dst, tag,
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm,
                                   MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Isend,
                                   (buf, count, BxiMpiDatatype::implem_datatype(datatype), dst, tag,
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm,
                                   MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM(int, Ssend,
                     (buf, count, BxiMpiDatatype::implem_datatype(datatype), dest, tag, BxiMpiComm::implem_comm(comm)),
                     const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Ssend_init,
                                   (buf, count, BxiMpiDatatype::implem_datatype(datatype), dest, tag,
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                                   MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Issend,
                                   (buf, count, BxiMpiDatatype::implem_datatype(datatype), dest, tag,
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                                   MPI_Request* request)

S4BXI_MPI_ONE_IMPLEM(int, Bsend,
                     (buf, count, BxiMpiDatatype::implem_datatype(datatype), dest, tag, BxiMpiComm::implem_comm(comm)),
                     const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)

S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Bsend_init,
                                   (buf, count, BxiMpiDatatype::implem_datatype(datatype), dest, tag,
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                                   MPI_Request* request)

S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Ibsend,
                                   (buf, count, BxiMpiDatatype::implem_datatype(datatype), dest, tag,
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                                   MPI_Request* request)

S4BXI_MPI_ONE_IMPLEM(int, Rsend,
                     (buf, count, BxiMpiDatatype::implem_datatype(datatype), dest, tag, BxiMpiComm::implem_comm(comm)),
                     const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)

S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Rsend_init,
                                   (buf, count, BxiMpiDatatype::implem_datatype(datatype), dest, tag,
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                                   MPI_Request* request)

S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Irsend,
                                   (buf, count, BxiMpiDatatype::implem_datatype(datatype), dest, tag,
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                                   MPI_Request* request)

S4BXI_MPI_ONE_IMPLEM(int, Sendrecv,
                     (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), dst, sendtag, recvbuf, recvcount,
                      BxiMpiDatatype::implem_datatype(recvtype), src, recvtag, BxiMpiComm::implem_comm(comm), status),
                     const void* sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag, void* recvbuf,
                     int recvcount, MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status* status)
S4BXI_MPI_ONE_IMPLEM(int, Sendrecv_replace,
                     (buf, count, BxiMpiDatatype::implem_datatype(datatype), dst, sendtag, src, recvtag,
                      BxiMpiComm::implem_comm(comm), status),
                     void* buf, int count, MPI_Datatype datatype, int dst, int sendtag, int src, int recvtag,
                     MPI_Comm comm, MPI_Status* status)

typedef int (*Test_func)(MPI_Request* request, int* flag, MPI_Status* status);
int S4BXI_MPI_Test(const char* __file, int __line, MPI_Request* request, int* flag, MPI_Status* status)
{
    s4bxi_bench_end();
    LOG_CALL(Test, __file, __line);
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    MPI_Request req_backup = nullptr;
    bool smpi              = false;
    if (is_smpi_request(*request)) {
        smpi       = true;
        req_backup = *request; // Backup request now because MPI_Test might wreck it
    }
    s4bxi_bench_begin();

    int out = ((Test_func)(smpi ? smpi_mpi_ops : main_actor->bull_mpi_ops)->Test)(request, flag, status);

    s4bxi_bench_end();
    if (*flag && smpi) { // If the request was deallocated and it's an SMPI one
        smpi_requests.erase(req_backup);
    }
    s4bxi_bench_begin();

    return out;
}

typedef int (*Testany_func)(int count, MPI_Request requests[], int* index, int* flag, MPI_Status* status);
int S4BXI_MPI_Testany(const char* __file, int __line, int count, MPI_Request requests[], int* index, int* flag,
                      MPI_Status* status)
{
    s4bxi_bench_end();
    LOG_CALL(Testany, __file, __line);
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    MPI_Request req_backup[count];

    bool smpi = false;
    for (int i = 0; i < count; ++i) { // Backup requests now because MPI_Testany might wreck them
        req_backup[i] = is_smpi_request(requests[i]) ? requests[i] : nullptr;
        if (req_backup[i])
            smpi = true;
    }

    s4bxi_bench_begin();

    int out =
        ((Testany_func)(smpi ? smpi_mpi_ops : main_actor->bull_mpi_ops)->Testany)(count, requests, index, flag, status);

    s4bxi_bench_end();
    if (*flag && req_backup[*index]) { // If a request was deallocated and it's an SMPI one
        smpi_requests.erase(req_backup[*index]);
    }
    s4bxi_bench_begin();

    return out;
}

typedef int (*Testall_func)(int count, MPI_Request* requests, int* flag, MPI_Status* statuses);
int S4BXI_MPI_Testall(const char* __file, int __line, int count, MPI_Request* requests, int* flag, MPI_Status* statuses)
{
    s4bxi_bench_end();
    LOG_CALL(Testall, __file, __line);
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    MPI_Request req_backup[count];

    bool smpi = false;
    for (int i = 0; i < count; ++i) { // Backup requests now because MPI_Testall might wreck them
        req_backup[i] = is_smpi_request(requests[i]) ? requests[i] : nullptr;
        if (req_backup[i])
            smpi = true;
    }

    s4bxi_bench_begin();

    int out =
        ((Testall_func)(smpi ? smpi_mpi_ops : main_actor->bull_mpi_ops)->Testall)(count, requests, flag, statuses);

    s4bxi_bench_end();
    for (int i = 0; i < count; ++i)
        if (!requests[i] && req_backup[i]) { // request is deallocated (i.e. == NULL) but it didn't use to be NULL
            smpi_requests.erase(req_backup[i]);
        }
    s4bxi_bench_begin();

    return out;
}

S4BXI_MPI_UNSUPPORTED(int, Testsome, /* (incount, requests, outcount, indices, status)), */ int incount,
                      MPI_Request requests[], int* outcount, int* indices, MPI_Status status[])
S4BXI_MPI_ONE_IMPLEM(int, Test_cancelled, (status, flag), const MPI_Status* status, int* flag)

typedef int (*Wait_func)(MPI_Request* request, MPI_Status* status);
int S4BXI_MPI_Wait(const char* __file, int __line, MPI_Request* request, MPI_Status* status)
{
    s4bxi_bench_end();
    LOG_CALL(Wait, __file, __line);
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    bool smpi = false;
    if (is_smpi_request(*request)) {
        smpi = true;
        if (*request) { // Don't try to remove MPI_REQUEST_NULL
            smpi_requests.erase(*request);
        }
    }
    s4bxi_bench_begin();

    return ((Wait_func)(smpi ? smpi_mpi_ops : main_actor->bull_mpi_ops)->Wait)(request, status);
}

// TODO: clean smpi_requests
typedef int (*Waitany_func)(int count, MPI_Request requests[], int* index, MPI_Status* status);
int S4BXI_MPI_Waitany(const char* __file, int __line, int count, MPI_Request requests[], int* index, MPI_Status* status)
{
    s4bxi_bench_end();
    LOG_CALL(Waitany, __file, __line);
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    bool smpi                = is_smpi_request(requests[0]);
    s4bxi_bench_begin();

    int out = ((Waitany_func)(smpi ? smpi_mpi_ops : main_actor->bull_mpi_ops)->Waitany)(count, requests, index, status);

    return out;
}

// This one technically doesn't do the right thing, but I don't see how we could do better
typedef int (*Waitall_func)(int count, MPI_Request requests[], MPI_Status status[]);
int S4BXI_MPI_Waitall(const char* __file, int __line, int count, MPI_Request requests[], MPI_Status status[])
{
    s4bxi_bench_end();
    LOG_CALL(Waitall, __file, __line);
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;

    bool smpi = is_smpi_request(requests[0]);
    for (int i = 0; i < count; ++i) {
        if (requests[i]) {
            smpi_requests.erase(requests[i]);
        }
    }
    s4bxi_bench_begin();

    return ((Waitall_func)(main_actor->use_smpi_implem ? smpi_mpi_ops : main_actor->bull_mpi_ops)->Waitall)(
        count, requests, status);
}
S4BXI_MPI_ONE_IMPLEM(int, Waitsome, (incount, requests, outcount, indices, status), int incount, MPI_Request requests[],
                     int* outcount, int* indices, MPI_Status status[])
S4BXI_MPI_ONE_IMPLEM(int, Iprobe, (source, tag, BxiMpiComm::implem_comm(comm), flag, status), int source, int tag,
                     MPI_Comm comm, int* flag, MPI_Status* status)
S4BXI_MPI_ONE_IMPLEM(int, Probe, (source, tag, BxiMpiComm::implem_comm(comm), status), int source, int tag,
                     MPI_Comm comm, MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(MPI_Request, Request_f2c, MPI_Fint request)
S4BXI_MPI_UNSUPPORTED(MPI_Fint, Request_c2f, MPI_Request request)

typedef int (*Cancel_func)(MPI_Request* request);
int S4BXI_MPI_Cancel(const char* __file, int __line, MPI_Request* request)
{
    s4bxi_bench_end();
    LOG_CALL(Cancel, __file, __line);
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    bool smpi                = is_smpi_request(*request);
    s4bxi_bench_begin();

    return ((Cancel_func)(smpi ? smpi_mpi_ops : main_actor->bull_mpi_ops)->Cancel)(request);
}

S4BXI_MPI_ONE_IMPLEM(int, Bcast,
                     (buf, count, BxiMpiDatatype::implem_datatype(datatype), root, BxiMpiComm::implem_comm(comm)),
                     void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Barrier, (BxiMpiComm::implem_comm(comm)), MPI_Comm comm)

S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Ibarrier, (BxiMpiComm::implem_comm(comm), request), MPI_Comm comm,
                                   MPI_Request* request)

S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(
    int, Ibcast, (buf, count, BxiMpiDatatype::implem_datatype(datatype), root, BxiMpiComm::implem_comm(comm), request),
    void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request* request)
typedef int (*Igather_func)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                            MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request);
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Igather,
                                   (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcount,
                                    BxiMpiDatatype::implem_datatype(recvtype), root, BxiMpiComm::implem_comm(comm),
                                    request),
                                   const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                   int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Igatherv,
                                   (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcounts,
                                    displs, BxiMpiDatatype::implem_datatype(recvtype), root,
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                   const int* recvcounts, const int* displs, MPI_Datatype recvtype, int root,
                                   MPI_Comm comm, MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Iallgather,
                                   (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcount,
                                    BxiMpiDatatype::implem_datatype(recvtype), BxiMpiComm::implem_comm(comm), request),
                                   const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                   int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Iallgatherv,
                                   (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcounts,
                                    displs, BxiMpiDatatype::implem_datatype(recvtype), BxiMpiComm::implem_comm(comm),
                                    request),
                                   const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                   const int* recvcounts, const int* displs, MPI_Datatype recvtype, MPI_Comm comm,
                                   MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Iscatter,
                                   (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcount,
                                    BxiMpiDatatype::implem_datatype(recvtype), root, BxiMpiComm::implem_comm(comm),
                                    request),
                                   const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                   int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Iscatterv,
                                   (sendbuf, sendcounts, displs, BxiMpiDatatype::implem_datatype(sendtype), recvbuf,
                                    recvcount, BxiMpiDatatype::implem_datatype(recvtype), root,
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype,
                                   void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                                   MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Ireduce,
                                   (sendbuf, recvbuf, count, BxiMpiDatatype::implem_datatype(datatype), implem_op(op),
                                    root, BxiMpiComm::implem_comm(comm), request),
                                   const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                   int root, MPI_Comm comm, MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Iallreduce,
                                   (sendbuf, recvbuf, count, BxiMpiDatatype::implem_datatype(datatype), implem_op(op),
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                   MPI_Comm comm, MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Iscan,
                                   (sendbuf, recvbuf, count, BxiMpiDatatype::implem_datatype(datatype), implem_op(op),
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                   MPI_Comm comm, MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Iexscan,
                                   (sendbuf, recvbuf, count, BxiMpiDatatype::implem_datatype(datatype), implem_op(op),
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                   MPI_Comm comm, MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Ireduce_scatter,
                                   (sendbuf, recvbuf, recvcounts, BxiMpiDatatype::implem_datatype(datatype),
                                    implem_op(op), BxiMpiComm::implem_comm(comm), request),
                                   const void* sendbuf, void* recvbuf, const int* recvcounts, MPI_Datatype datatype,
                                   MPI_Op op, MPI_Comm comm, MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Ireduce_scatter_block,
                                   (sendbuf, recvbuf, recvcount, BxiMpiDatatype::implem_datatype(datatype),
                                    implem_op(op), BxiMpiComm::implem_comm(comm), request),
                                   const void* sendbuf, void* recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op,
                                   MPI_Comm comm, MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Ialltoall,
                                   (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcount,
                                    BxiMpiDatatype::implem_datatype(recvtype), BxiMpiComm::implem_comm(comm), request),
                                   const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                   int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request)
S4BXI_MPI_ONE_IMPLEM_STORE_REQUEST(int, Ialltoallv,
                                   (sendbuf, sendcounts, senddisps, BxiMpiDatatype::implem_datatype(sendtype), recvbuf,
                                    recvcounts, recvdisps, BxiMpiDatatype::implem_datatype(recvtype),
                                    BxiMpiComm::implem_comm(comm), request),
                                   const void* sendbuf, const int* sendcounts, const int* senddisps,
                                   MPI_Datatype sendtype, void* recvbuf, const int* recvcounts, const int* recvdisps,
                                   MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request)
typedef int (*Ialltoallw_func)(const void* sendbuf, const int* sendcounts, const int* senddisps,
                               const MPI_Datatype* sendtypes, void* recvbuf, const int* recvcounts,
                               const int* recvdisps, const MPI_Datatype* recvtypes, MPI_Comm comm,
                               MPI_Request* request);
int S4BXI_MPI_Ialltoallw(const char* __file, int __line, const void* sendbuf, const int* sendcounts,
                         const int* senddisps, const MPI_Datatype* sendtypes, void* recvbuf, const int* recvcounts,
                         const int* recvdisps, const MPI_Datatype* recvtypes, MPI_Comm comm, MPI_Request* request)
{
    LOG_CALL(Ialltoallw, __file, __line);
    BxiMainActor* main_actor = GET_CURRENT_MAIN_ACTOR;
    int size;
    S4BXI_MPI_Comm_size(__file, __line, comm, &size);
    MPI_Datatype sendtypes_arr[size];
    MPI_Datatype recvtypes_arr[size];
    bool smpi = false;
    if (main_actor->use_smpi_implem)
        smpi = true;

    int out = ((Ialltoallw_func)(smpi ? smpi_mpi_ops : main_actor->bull_mpi_ops)->Ialltoallw)(
        sendbuf, sendcounts, senddisps, implem_datatypes(sendtypes, size, sendtypes_arr), recvbuf, recvcounts,
        recvdisps, implem_datatypes(recvtypes, size, recvtypes_arr), BxiMpiComm::implem_comm(comm), request);

    s4bxi_bench_end();
    if (smpi) {
        smpi_requests.emplace(*request);
    }
    s4bxi_bench_begin();

    return out;
}

S4BXI_MPI_ONE_IMPLEM(int, Gather,
                     (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcount,
                      BxiMpiDatatype::implem_datatype(recvtype), root, BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                     MPI_Datatype recvtype, int root, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Gatherv,
                     (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcounts, displs,
                      BxiMpiDatatype::implem_datatype(recvtype), root, BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
                     const int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Allgather,
                     (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcount,
                      BxiMpiDatatype::implem_datatype(recvtype), BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                     MPI_Datatype recvtype, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Allgatherv,
                     (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcounts, displs,
                      BxiMpiDatatype::implem_datatype(recvtype), BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
                     const int* displs, MPI_Datatype recvtype, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Scatter,
                     (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcount,
                      BxiMpiDatatype::implem_datatype(recvtype), root, BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                     MPI_Datatype recvtype, int root, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Scatterv,
                     (sendbuf, sendcounts, displs, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcount,
                      BxiMpiDatatype::implem_datatype(recvtype), root, BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype,
                     void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Reduce,
                     (sendbuf, recvbuf, count, BxiMpiDatatype::implem_datatype(datatype), implem_op(op), root,
                      BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                     MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Allreduce,
                     (sendbuf, recvbuf, count, BxiMpiDatatype::implem_datatype(datatype), implem_op(op),
                      BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Scan,
                     (sendbuf, recvbuf, count, BxiMpiDatatype::implem_datatype(datatype), implem_op(op),
                      BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Exscan,
                     (sendbuf, recvbuf, count, BxiMpiDatatype::implem_datatype(datatype), implem_op(op),
                      BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Reduce_scatter,
                     (sendbuf, recvbuf, recvcounts, BxiMpiDatatype::implem_datatype(datatype), implem_op(op),
                      BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, void* recvbuf, const int* recvcounts, MPI_Datatype datatype, MPI_Op op,
                     MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Reduce_scatter_block,
                     (sendbuf, recvbuf, recvcount, BxiMpiDatatype::implem_datatype(datatype), implem_op(op),
                      BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, void* recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Alltoall,
                     (sendbuf, sendcount, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcount,
                      BxiMpiDatatype::implem_datatype(recvtype), BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                     MPI_Datatype recvtype, MPI_Comm comm)
S4BXI_MPI_ONE_IMPLEM(int, Alltoallv,
                     (sendbuf, sendcounts, senddisps, BxiMpiDatatype::implem_datatype(sendtype), recvbuf, recvcounts,
                      recvdisps, BxiMpiDatatype::implem_datatype(recvtype), BxiMpiComm::implem_comm(comm)),
                     const void* sendbuf, const int* sendcounts, const int* senddisps, MPI_Datatype sendtype,
                     void* recvbuf, const int* recvcounts, const int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm)
S4BXI_MPI_W_COLLECTIVE(int, Alltoallw,
                       (sendbuf, sendcnts, sdispls, implem_datatypes(sendtypes, size, sendtypes_arr), recvbuf, recvcnts,
                        rdispls, implem_datatypes(recvtypes, size, recvtypes_arr), BxiMpiComm::implem_comm(comm)),
                       const void* sendbuf, const int* sendcnts, const int* sdispls, const MPI_Datatype* sendtypes,
                       void* recvbuf, const int* recvcnts, const int* rdispls, const MPI_Datatype* recvtypes,
                       MPI_Comm comm)
S4BXI_MPI_UNSUPPORTED(int, Reduce_local, const void* inbuf, void* inoutbuf, int count, MPI_Datatype datatype, MPI_Op op)

S4BXI_MPI_UNSUPPORTED(int, Info_create, MPI_Info* info)
S4BXI_MPI_UNSUPPORTED(int, Info_set, MPI_Info info, const char* key, const char* value)
S4BXI_MPI_UNSUPPORTED(int, Info_get, MPI_Info info, const char* key, int valuelen, char* value, int* flag)
S4BXI_MPI_UNSUPPORTED(int, Info_free, MPI_Info* info)
S4BXI_MPI_UNSUPPORTED(int, Info_delete, MPI_Info info, const char* key)
S4BXI_MPI_UNSUPPORTED(int, Info_dup, MPI_Info info, MPI_Info* newinfo)
S4BXI_MPI_UNSUPPORTED(int, Info_get_nkeys, MPI_Info info, int* nkeys)
S4BXI_MPI_UNSUPPORTED(int, Info_get_nthkey, MPI_Info info, int n, char* key)
S4BXI_MPI_UNSUPPORTED(int, Info_get_valuelen, MPI_Info info, const char* key, int* valuelen, int* flag)
S4BXI_MPI_UNSUPPORTED(MPI_Info, Info_f2c, MPI_Fint info)
S4BXI_MPI_UNSUPPORTED(MPI_Fint, Info_c2f, MPI_Info info)

S4BXI_MPI_UNSUPPORTED(int, Win_free, MPI_Win* win)
S4BXI_MPI_UNSUPPORTED(int, Win_create, void* base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
                      MPI_Win* win)
S4BXI_MPI_UNSUPPORTED(int, Win_allocate, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void* base,
                      MPI_Win* win)
S4BXI_MPI_UNSUPPORTED(int, Win_allocate_shared, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void* base,
                      MPI_Win* win)
S4BXI_MPI_UNSUPPORTED(int, Win_create_dynamic, MPI_Info info, MPI_Comm comm, MPI_Win* win)
S4BXI_MPI_UNSUPPORTED(int, Win_attach, MPI_Win win, void* base, MPI_Aint size)
S4BXI_MPI_UNSUPPORTED(int, Win_detach, MPI_Win win, const void* base)
S4BXI_MPI_UNSUPPORTED(int, Win_set_name, MPI_Win win, const char* name)
S4BXI_MPI_UNSUPPORTED(int, Win_get_name, MPI_Win win, char* name, int* len)
S4BXI_MPI_UNSUPPORTED(int, Win_set_info, MPI_Win win, MPI_Info info)
S4BXI_MPI_UNSUPPORTED(int, Win_get_info, MPI_Win win, MPI_Info* info)
S4BXI_MPI_UNSUPPORTED(int, Win_get_group, MPI_Win win, MPI_Group* group)
S4BXI_MPI_UNSUPPORTED(int, Win_fence, int assert, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_get_attr, MPI_Win type, int type_keyval, void* attribute_val, int* flag)
S4BXI_MPI_UNSUPPORTED(int, Win_set_attr, MPI_Win type, int type_keyval, void* att)
S4BXI_MPI_UNSUPPORTED(int, Win_delete_attr, MPI_Win type, int comm_keyval)
S4BXI_MPI_UNSUPPORTED(int, Win_create_keyval, MPI_Win_copy_attr_function* copy_fn,
                      MPI_Win_delete_attr_function* delete_fn, int* keyval, void* extra_state)
S4BXI_MPI_UNSUPPORTED(int, Win_free_keyval, int* keyval)
S4BXI_MPI_UNSUPPORTED(int, Win_complete, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_post, MPI_Group group, int assert, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_start, MPI_Group group, int assert, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_wait, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_lock, int lock_type, int rank, int assert, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_lock_all, int assert, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_unlock, int rank, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_unlock_all, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_flush, int rank, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_flush_local, int rank, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_flush_all, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_flush_local_all, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Win_shared_query, MPI_Win win, int rank, MPI_Aint* size, int* disp_unit, void* baseptr)
S4BXI_MPI_UNSUPPORTED(int, Win_sync, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(MPI_Win, Win_f2c, MPI_Fint win)
S4BXI_MPI_UNSUPPORTED(MPI_Fint, Win_c2f, MPI_Win win)

S4BXI_MPI_UNSUPPORTED(int, Get, void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
                      MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Put, const void* origin_addr, int origin_count, MPI_Datatype origin_datatype,
                      int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
                      MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Accumulate, const void* origin_addr, int origin_count, MPI_Datatype origin_datatype,
                      int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op,
                      MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Get_accumulate, const void* origin_addr, int origin_count, MPI_Datatype origin_datatype,
                      void* result_addr, int result_count, MPI_Datatype result_datatype, int target_rank,
                      MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Rget, void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
                      MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win,
                      MPI_Request* request)
S4BXI_MPI_UNSUPPORTED(int, Rput, const void* origin_addr, int origin_count, MPI_Datatype origin_datatype,
                      int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
                      MPI_Win win, MPI_Request* request)
S4BXI_MPI_UNSUPPORTED(int, Raccumulate, const void* origin_addr, int origin_count, MPI_Datatype origin_datatype,
                      int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op,
                      MPI_Win win, MPI_Request* request)
S4BXI_MPI_UNSUPPORTED(int, Rget_accumulate, const void* origin_addr, int origin_count, MPI_Datatype origin_datatype,
                      void* result_addr, int result_count, MPI_Datatype result_datatype, int target_rank,
                      MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                      MPI_Request* request)
S4BXI_MPI_UNSUPPORTED(int, Fetch_and_op, const void* origin_addr, void* result_addr, MPI_Datatype datatype,
                      int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win)
S4BXI_MPI_UNSUPPORTED(int, Compare_and_swap, const void* origin_addr, void* compare_addr, void* result_addr,
                      MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Win win)

S4BXI_MPI_UNSUPPORTED(int, Cart_coords, MPI_Comm comm, int rank, int maxdims, int* coords)
S4BXI_MPI_UNSUPPORTED(int, Cart_create, MPI_Comm comm_old, int ndims, const int* dims, const int* periods, int reorder,
                      MPI_Comm* comm_cart)
S4BXI_MPI_UNSUPPORTED(int, Cart_get, MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords)
S4BXI_MPI_UNSUPPORTED(int, Cart_rank, MPI_Comm comm, const int* coords, int* rank)
S4BXI_MPI_UNSUPPORTED(int, Cart_shift, MPI_Comm comm, int direction, int displ, int* source, int* dest)
S4BXI_MPI_UNSUPPORTED(int, Cart_sub, MPI_Comm comm, const int* remain_dims, MPI_Comm* comm_new)
S4BXI_MPI_UNSUPPORTED(int, Cartdim_get, MPI_Comm comm, int* ndims)
S4BXI_MPI_UNSUPPORTED(int, Dims_create, int nnodes, int ndims, int* dims)
S4BXI_MPI_UNSUPPORTED(int, Request_get_status, MPI_Request request, int* flag, MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, Grequest_start, MPI_Grequest_query_function* query_fn, MPI_Grequest_free_function* free_fn,
                      MPI_Grequest_cancel_function* cancel_fn, void* extra_state, MPI_Request* request)
S4BXI_MPI_UNSUPPORTED(int, Grequest_complete, MPI_Request request)
S4BXI_MPI_UNSUPPORTED(int, Status_set_cancelled, MPI_Status* status, int flag)
S4BXI_MPI_UNSUPPORTED(int, Status_set_elements, MPI_Status* status, MPI_Datatype datatype, int count)
S4BXI_MPI_UNSUPPORTED(int, Status_set_elements_x, MPI_Status* status, MPI_Datatype datatype, MPI_Count count)
S4BXI_MPI_UNSUPPORTED(int, Type_create_subarray, int ndims, const int* array_of_sizes, const int* array_of_subsizes,
                      const int* array_of_starts, int order, MPI_Datatype oldtype, MPI_Datatype* newtype)

S4BXI_MPI_UNSUPPORTED(int, File_open, MPI_Comm comm, const char* filename, int amode, MPI_Info info, MPI_File* fh)
S4BXI_MPI_UNSUPPORTED(int, File_close, MPI_File* fh)
S4BXI_MPI_UNSUPPORTED(int, File_delete, const char* filename, MPI_Info info)
S4BXI_MPI_UNSUPPORTED(int, File_get_size, MPI_File fh, MPI_Offset* size)
S4BXI_MPI_UNSUPPORTED(int, File_get_group, MPI_File fh, MPI_Group* group)
S4BXI_MPI_UNSUPPORTED(int, File_get_amode, MPI_File fh, int* amode)
S4BXI_MPI_UNSUPPORTED(int, File_set_info, MPI_File fh, MPI_Info info)
S4BXI_MPI_UNSUPPORTED(int, File_get_info, MPI_File fh, MPI_Info* info_used)
S4BXI_MPI_UNSUPPORTED(int, File_read_at, MPI_File fh, MPI_Offset offset, void* buf, int count, MPI_Datatype datatype,
                      MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, File_read_at_all, MPI_File fh, MPI_Offset offset, void* buf, int count,
                      MPI_Datatype datatype, MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, File_write_at, MPI_File fh, MPI_Offset offset, const void* buf, int count,
                      MPI_Datatype datatype, MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, File_write_at_all, MPI_File fh, MPI_Offset offset, const void* buf, int count,
                      MPI_Datatype datatype, MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, File_read, MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, File_read_all, MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, File_write, MPI_File fh, const void* buf, int count, MPI_Datatype datatype,
                      MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, File_write_all, MPI_File fh, const void* buf, int count, MPI_Datatype datatype,
                      MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, File_seek, MPI_File fh, MPI_Offset offset, int whenace)
S4BXI_MPI_UNSUPPORTED(int, File_get_position, MPI_File fh, MPI_Offset* offset)
S4BXI_MPI_UNSUPPORTED(int, File_read_shared, MPI_File fh, void* buf, int count, MPI_Datatype datatype,
                      MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, File_write_shared, MPI_File fh, const void* buf, int count, MPI_Datatype datatype,
                      MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, File_read_ordered, MPI_File fh, void* buf, int count, MPI_Datatype datatype,
                      MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, File_write_ordered, MPI_File fh, const void* buf, int count, MPI_Datatype datatype,
                      MPI_Status* status)
S4BXI_MPI_UNSUPPORTED(int, File_seek_shared, MPI_File fh, MPI_Offset offset, int whence)
S4BXI_MPI_UNSUPPORTED(int, File_get_position_shared, MPI_File fh, MPI_Offset* offset)
S4BXI_MPI_UNSUPPORTED(int, File_sync, MPI_File fh)
S4BXI_MPI_UNSUPPORTED(int, File_set_view, MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype,
                      const char* datarep, MPI_Info info)
S4BXI_MPI_UNSUPPORTED(int, File_get_view, MPI_File fh, MPI_Offset* disp, MPI_Datatype* etype, MPI_Datatype* filetype,
                      char* datarep)

S4BXI_MPI_UNSUPPORTED(int, Errhandler_set, MPI_Comm comm, MPI_Errhandler errhandler)
S4BXI_MPI_UNSUPPORTED(int, Errhandler_create, MPI_Handler_function* function, MPI_Errhandler* errhandler)
S4BXI_MPI_UNSUPPORTED(int, Errhandler_free, MPI_Errhandler* errhandler)
S4BXI_MPI_UNSUPPORTED(int, Errhandler_get, MPI_Comm comm, MPI_Errhandler* errhandler)
S4BXI_MPI_UNSUPPORTED(int, Comm_set_errhandler, MPI_Comm comm, MPI_Errhandler errhandler)
S4BXI_MPI_UNSUPPORTED(int, Comm_get_errhandler, MPI_Comm comm, MPI_Errhandler* errhandler)
S4BXI_MPI_UNSUPPORTED(int, Comm_create_errhandler, MPI_Comm_errhandler_fn* function, MPI_Errhandler* errhandler)
S4BXI_MPI_UNSUPPORTED(int, Comm_call_errhandler, MPI_Comm comm, int errorcode)
S4BXI_MPI_UNSUPPORTED(int, Win_set_errhandler, MPI_Win win, MPI_Errhandler errhandler)
S4BXI_MPI_UNSUPPORTED(int, Win_get_errhandler, MPI_Win win, MPI_Errhandler* errhandler)
S4BXI_MPI_UNSUPPORTED(int, Win_create_errhandler, MPI_Win_errhandler_fn* function, MPI_Errhandler* errhandler)
S4BXI_MPI_UNSUPPORTED(int, Win_call_errhandler, MPI_Win win, int errorcode)
S4BXI_MPI_UNSUPPORTED(MPI_Errhandler, Errhandler_f2c, MPI_Fint errhandler)
S4BXI_MPI_UNSUPPORTED(MPI_Fint, Errhandler_c2f, MPI_Errhandler errhandler)

S4BXI_MPI_UNSUPPORTED(int, Type_create_f90_integer, int count, MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_create_f90_real, int prec, int exp, MPI_Datatype* newtype)
S4BXI_MPI_UNSUPPORTED(int, Type_create_f90_complex, int prec, int exp, MPI_Datatype* newtype)

S4BXI_MPI_UNSUPPORTED(int, Type_get_contents, MPI_Datatype datatype, int max_integers, int max_addresses,
                      int max_datatypes, int* array_of_integers, MPI_Aint* array_of_addresses,
                      MPI_Datatype* array_of_datatypes)
S4BXI_MPI_UNSUPPORTED(int, Type_get_envelope, MPI_Datatype datatype, int* num_integers, int* num_addresses,
                      int* num_datatypes, int* combiner)
S4BXI_MPI_UNSUPPORTED(int, File_call_errhandler, MPI_File fh, int errorcode)
S4BXI_MPI_UNSUPPORTED(int, File_create_errhandler, MPI_File_errhandler_function* function, MPI_Errhandler* errhandler)
S4BXI_MPI_UNSUPPORTED(int, File_set_errhandler, MPI_File file, MPI_Errhandler errhandler)
S4BXI_MPI_UNSUPPORTED(int, File_get_errhandler, MPI_File file, MPI_Errhandler* errhandler)

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
