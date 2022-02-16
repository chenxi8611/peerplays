#pragma once
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {
   class database;
   using namespace graphene::db;

   enum class match_state
   {
      waiting_on_previous_matches,
      match_in_progress,
      match_complete
   };

   class match_object : public graphene::db::abstract_object<match_object>
   {
   public:
      static const uint8_t space_id = protocol_ids;
      static const uint8_t type_id  = match_object_type;

      tournament_id_type tournament_id;

      /// The players in the match
      vector<account_id_type> players;

      /// The list of games in the match
      /// Unlike tournaments where the list of matches is known at the start,
      /// the list of games will start with one game and grow until we have played
      /// enough games to declare a winner for the match.
      vector<game_id_type> games;

      /// A list of the winners of each round of the game.  This information is 
      /// also stored in the game object, but is duplicated here to allow displaying
      /// information about a match without having to request all game objects
      vector<flat_set<account_id_type> > game_winners;

      /// A count of the number of wins for each player
      vector<uint32_t> number_of_wins;

      /// the total number of games that ended up in a tie/draw/stalemate
      uint32_t number_of_ties;

      // If the match is not yet complete, this will be empty
      // If the match is in the "match_complete" state, it will contain the
      // list of winners.
      // For Rock-paper-scissors, there will be one winner, unless there is
      // a stalemate (in that case, there are no winners)
      flat_set<account_id_type> match_winners;

      /// the time the match started
      time_point_sec start_time;

      /// If the match has ended, the time it ended
      optional<time_point_sec> end_time;

      match_object();
      match_object(const match_object& rhs);
      ~match_object();
      match_object& operator=(const match_object& rhs);
      
      match_state get_state() const;

      void on_initiate_match(database& db);
      void on_game_complete(database& db, const game_object& game);
      game_id_type start_next_game(database& db, match_id_type match_id);

      class impl;
      std::unique_ptr<impl> my;
   };
      
   typedef multi_index_container<
      match_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >      >
   > match_object_multi_index_type;
   typedef generic_index<match_object, match_object_multi_index_type> match_index;

} }

FC_REFLECT_ENUM(graphene::chain::match_state,
                (waiting_on_previous_matches)
                (match_in_progress)
                (match_complete))

//FC_REFLECT_TYPENAME(graphene::chain::match_object) // manually serialized
FC_REFLECT_DERIVED(graphene::chain::match_object, (graphene::db::object),
                   (tournament_id)
                   (players)
                   (games)
                   (game_winners)
                   (number_of_wins)
                   (number_of_ties)
                   (match_winners)
                   (start_time)
                   (end_time))

GRAPHENE_EXTERNAL_SERIALIZATION( extern, graphene::chain::match_object )