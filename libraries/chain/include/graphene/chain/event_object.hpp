/*
 * Copyright (c) 2018 Peerplays Blockchain Standards Association, and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/event.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {

class database;

class event_object : public graphene::db::abstract_object< event_object >
{
   public:
      static const uint8_t space_id = protocol_ids;
      static const uint8_t type_id = event_object_type;

      event_object();
      event_object(const event_object& rhs);
      ~event_object();
      event_object& operator=(const event_object& rhs);

      internationalized_string_type name;
      
      internationalized_string_type season;

      optional<time_point_sec> start_time;

      event_group_id_type event_group_id;

      bool at_least_one_betting_market_group_settled;

      event_status get_status() const;
      vector<string> scores;

      void on_upcoming_event(database& db);
      void on_in_progress_event(database& db);
      void on_frozen_event(database& db);
      void on_finished_event(database& db);
      void on_canceled_event(database& db);
      void on_settled_event(database& db);
      void on_betting_market_group_resolved(database& db, betting_market_group_id_type resolved_group, bool was_canceled);
      void on_betting_market_group_closed(database& db, betting_market_group_id_type closed_group);
      void dispatch_new_status(database& db, event_status new_status);
   private:
      class impl;
      std::unique_ptr<impl> my;
};

struct by_event_group_id;
struct by_event_status;
typedef multi_index_container<
   event_object,
   indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
      ordered_unique< tag<by_event_group_id>, composite_key<event_object,
                                                            member< event_object, event_group_id_type, &event_object::event_group_id >,
                                                            member<object, object_id_type, &object::id> > >,
      ordered_unique< tag<by_event_status>, composite_key<event_object,
                                                          const_mem_fun< event_object, event_status, &event_object::get_status >,
                                                          member<object, object_id_type, &object::id> > > > > event_object_multi_index_type;

typedef generic_index<event_object, event_object_multi_index_type> event_object_index;
} } // graphene::chain

FC_REFLECT_DERIVED(graphene::chain::event_object, (graphene::db::object),
                   (name)
                   (season)
                   (start_time)
                   (event_group_id)
                   (at_least_one_betting_market_group_settled)
                   (scores))

GRAPHENE_EXTERNAL_SERIALIZATION( extern, graphene::chain::event_object )


