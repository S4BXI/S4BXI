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

#include "s4bxi/plugins/BxiActorExt.hpp"
#include "s4bxi/BxiEngine.hpp"
#include <simgrid/s4u.hpp>

using namespace simgrid;

class BxiActorExt {
  public:
    static xbt::Extension<s4u::Actor, BxiActorExt> EXTENSION_ID;
    static BxiMainActor* get_current_main_actor();
    BxiMainActor* main_actor = nullptr;
};

simgrid::xbt::Extension<simgrid::s4u::Actor, BxiActorExt> BxiActorExt::EXTENSION_ID;

/**
 * @brief Initializes the BxiActorExt plugin
 */
void s4bxi_actor_ext_plugin_init()
{
    if (BxiActorExt::EXTENSION_ID.valid()) // Don't do the job twice
        return;

    // First register our extension of Actors properly
    BxiActorExt::EXTENSION_ID = s4u::Actor::extension_create<BxiActorExt>();

    // If SimGrid is already initialized, we need to attach an extension to each existing actor
    if (s4u::Engine::is_initialized()) {
        const s4u::Engine* e = s4u::Engine::get_instance();
        for (auto& actor : e->get_all_actors()) {
            actor->extension_set(new BxiActorExt);
        }
    }

    // Make sure that every future actor also gets an extension (in case the platform is not loaded yet)
    s4u::Actor::on_creation_cb([](s4u::Actor& actor) { actor.extension_set(new BxiActorExt); });
}

BxiMainActor* get_cached_current_main_actor() {
  s4u::ActorPtr actor = s4u::Actor::self();
  BxiActorExt *ext = actor->extension<BxiActorExt>();

  if (!ext->main_actor)
    ext->main_actor = BxiEngine::get_instance()->get_current_main_actor();

  return ext->main_actor;
}