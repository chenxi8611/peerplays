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
#include <graphene/chain/rock_paper_scissors.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {
   class database;
   using namespace graphene::db;

   enum class game_state
   {
      game_in_progress,
      expecting_commit_moves,
      expecting_reveal_moves,
      game_complete
   };

   class game_object : public graphene::db::abstract_object<game_object>
   {
   public:
      static const uint8_t space_id = protocol_ids;
      static const uint8_t type_id  = game_object_type;

      match_id_type match_id;

      vector<account_id_type> players;

      flat_set<account_id_type> winners;

      game_specific_details game_details;

      fc::optional<time_point_sec> next_timeout;
      
      game_state get_state() const;

      game_object();
      game_object(const game_object& rhs);
      ~game_object();
      game_object& operator=(const game_object& rhs);

      void evaluate_move_operation(const database& db, const game_move_operation& op) const;
      void make_automatic_moves(database& db);
      void determine_winner(database& db);

      void on_move(database& db, const game_move_operation& op);
      void on_timeout(database& db);
      void start_game(database& db, const std::vector<account_id_type>& players);

      class impl;
      std::unique_ptr<impl> my;
   };

   struct by_next_timeout {};
   typedef multi_index_container<
      game_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_next_timeout>, 
            composite_key<game_object, 
               member<game_object, optional<time_point_sec>, &game_object::next_timeout>,
               member<object, object_id_type, &object::id> > > >
   > game_object_multi_index_type;
   typedef generic_index<game_object, game_object_multi_index_type> game_index;
} }

FC_REFLECT_ENUM(graphene::chain::game_state,
                (game_in_progress)
                (expecting_commit_moves)
                (expecting_reveal_moves)
                (game_complete))

//FC_REFLECT_TYPENAME(graphene::chain::game_object) // manually serialized
FC_REFLECT_DERIVED(graphene::chain::game_object, (graphene::db::object),
                   (match_id)
                   (players)
                   (winners)
                   (game_details)
                   (next_timeout))

GRAPHENE_EXTERNAL_SERIALIZATION( extern, graphene::chain::game_object )