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
#include "s4bxi/actors/BxiUserAppActor.hpp"
#include "s4bxi/s4bxi_util.hpp"
#include "s4bxi/s4bxi_c_util.h"
#include "portals4.h"
#include "s4bxi/s4bxi_bench.hpp"
#include "s4bxi/s4bxi_xbt_log.h"
#include <unistd.h>

S4BXI_LOG_NEW_DEFAULT_CATEGORY(c_util, "Messages specific to C util");

using namespace simgrid;

int _s4bxi_should_allow_polling = 1;

void s4bxi_compute(double flops)
{
    auto main_actor = GET_CURRENT_MAIN_ACTOR;

    s4bxi_bench_end();

    auto nid = main_actor->getNid();
    S4BXI_STARTLOG(S4BXILOG_COMPUTE, nid, nid)
    s4u::this_actor::execute(flops);
    S4BXI_WRITELOG()

    s4bxi_bench_begin();
}

void s4bxi_compute_s(double seconds)
{
    s4bxi_bench_end();
    s4bxi_execute(seconds);
    s4bxi_bench_begin();
}

int s4bxi_fprintf(FILE* stream, const char* fmt, ...)
{
    int rank = s4bxi_get_my_rank();
    if (isatty(fileno(stream))) {
        // Start at 36 and go in reverse, to avoid red and green as much as possible
        // (they could be misleading since they evoke success or failure)
        fprintf(stream, "[\033[1;%dm%f - %s:%d\033[0m] ", 36 - (rank % 6), s4u::Engine::get_clock(),
                s4u::Actor::self()->get_host()->get_cname(), rank);
    } else {
        fprintf(stream, "[%f - %s:%d] ", s4u::Engine::get_clock(), s4u::Actor::self()->get_host()->get_cname(), rank);
    }

    va_list argp;
    va_start(argp, fmt);
    int res = vfprintf(stream, fmt, argp);
    va_end(argp);

    return res;
}

/**
 * This is stupid but I can't find how to re-use s4bxi_fprintf because of the variable number of parameters
 * (and for some reason making this a define won't work)
 */
int s4bxi_printf(const char* fmt, ...)
{
    int rank = s4bxi_get_my_rank();

    if (isatty(fileno(stdout))) {
        printf("[\033[1;%dm%f - %s:%d\033[0m] ", 36 - (rank % 6), s4u::Engine::get_clock(),
               s4u::Actor::self()->get_host()->get_cname(), rank);
    } else {
        printf("[%f - %s:%d] ", s4u::Engine::get_clock(), s4u::Actor::self()->get_host()->get_cname(), rank);
    }

    va_list argp;
    va_start(argp, fmt);
    int res = vprintf(fmt, argp);
    va_end(argp);

    return res;
}

uint32_t s4bxi_num_local_peers()
{
    return BxiEngine::get_instance()->main_actors_on_host(GET_CURRENT_MAIN_ACTOR->getSlug().c_str()).size() - 1;
}

uint32_t s4bxi_num_nodes()
{
    return BxiEngine::get_instance()->used_nodes().size();
}

void s4bxi_local_peers_list(char* list, size_t maxlen)
{
    list[0] = '\0';
    for (auto pair : BxiEngine::get_instance()->main_actors_on_host(GET_CURRENT_MAIN_ACTOR->getSlug().c_str())) {
        size_t cur_val_length = strlen(list);

        auto ret =
            snprintf(list + cur_val_length, maxlen - cur_val_length, "%d,", (((BxiUserAppActor*)pair.second)->my_rank));

        if (ret >= maxlen - cur_val_length)
            ptl_panic_fmt("Overflowing snprintf at %s:%d\n", __FILE__, __LINE__);
    }

    list[strlen(list) - 1] = '\0';
}

uint32_t s4bxi_get_my_rank()
{
    return ((BxiUserAppActor*)GET_CURRENT_MAIN_ACTOR)->my_rank;
}

uint32_t s4bxi_get_my_local_rank()
{
    return ((BxiUserAppActor*)GET_CURRENT_MAIN_ACTOR)->my_local_rank;
}

uint32_t s4bxi_global_rank_from_local(uint32_t local)
{
    std::string slug = GET_CURRENT_MAIN_ACTOR->getSlug();
    auto actor       = BxiEngine::get_instance()->get_actor_from_slug_and_localrank(slug, local);

    if (!actor)
        ptl_panic_fmt("Couldn't find actor with local rank %d on host %s", local, slug.c_str());

    return ((BxiUserAppActor*)actor)->my_rank;
}

uint32_t s4bxi_get_rank_number()
{
    return BxiEngine::get_instance()->get_main_actor_count();
}

void s4bxi_get_hostname_from_rank(char* hostname, int rank)
{
    std::string slug = BxiEngine::get_instance()->get_actor_from_rank(rank)->getSlug();
    slug.copy(hostname, slug.size());
}

int s4bxi_get_ptl_process_from_rank(int rank, ptl_process_t* out)
{
    BxiMainActor* actor = BxiEngine::get_instance()->get_actor_from_rank(rank);

    return actor->PtlGetId(actor->default_ni, out);
}

void s4bxi_set_ptl_process_for_rank(ptl_handle_ni_t ni)
{
    BxiMainActor* actor = BxiEngine::get_instance()->get_current_main_actor();

    actor->default_ni = (BxiNI*)ni;
}

double s4bxi_simtime()
{
    s4bxi_bench_end();
    double res = s4u::Engine::get_clock();
    s4bxi_bench_begin();

    return res;
}

int s4bxi_is_polling()
{
    return ((BxiUserAppActor*)GET_CURRENT_MAIN_ACTOR)->is_polling;
}

void s4bxi_set_polling(int p)
{
    ((BxiUserAppActor*)GET_CURRENT_MAIN_ACTOR)->is_polling = p;
}

void s4bxi_barrier()
{
    BxiMainActor::barrier();
}

void s4bxi_keyval_store_pointer(char* key, void* value)
{
    GET_CURRENT_MAIN_ACTOR->keyval_store.emplace(key, value);
}

void* s4bxi_keyval_fetch_pointer(int rank, char* key)
{
    auto a = BxiEngine::get_instance();
    auto b = a->get_actor_from_rank(rank);
    auto c = b->keyval_store;
    return c[key];
}

/**
 * Only allow a positive log level if there is a valid config file
 */
void s4bxi_set_loglevel(int l)
{
    auto config       = BxiEngine::get_config();
    config->log_level = config->log_folder != "/dev/null" ? l : 0;
}

void s4bxi_use_smpi_implem(int v)
{
    GET_CURRENT_MAIN_ACTOR->use_smpi_implem = (bool)v;
}