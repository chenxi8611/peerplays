/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
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
#include <graphene/chain/committee_member_evaluator.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/protocol/vote.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>

namespace graphene { namespace chain {

void_result committee_member_create_evaluator::do_evaluate( const committee_member_create_operation& op )
{ try {
   FC_ASSERT(db().get(op.committee_member_account).is_lifetime_member());
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type committee_member_create_evaluator::do_apply( const committee_member_create_operation& op )
{ try {
   vote_id_type vote_id;
   db().modify(db().get_global_properties(), [&vote_id](global_property_object& p) {
      vote_id = get_next_vote_id(p, vote_id_type::committee);
   });

   const auto& new_del_object = db().create<committee_member_object>( [&]( committee_member_object& obj ){
         obj.committee_member_account   = op.committee_member_account;
         obj.vote_id            = vote_id;
         obj.url                = op.url;
   });
   return new_del_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_member_update_evaluator::do_evaluate( const committee_member_update_operation& op )
{ try {
   FC_ASSERT(db().get(op.committee_member).committee_member_account == op.committee_member_account);
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_member_update_evaluator::do_apply( const committee_member_update_operation& op )
{ try {
   database& _db = db();
   _db.modify(
      _db.get(op.committee_member),
      [&]( committee_member_object& com )
      {
         if( op.new_url.valid() )
            com.url = *op.new_url;
      });
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_member_update_global_parameters_evaluator::do_evaluate(const committee_member_update_global_parameters_operation& o)
{ try {
   std::cout<<"DAVOR!!!"<<std::endl;
   FC_ASSERT(trx_state->_is_proposed_trx);
   std::cout<<"DAVOR, DAVOR!!!"<<std::endl;
   
   dgpo = &db().get_global_properties();
   if( o.new_parameters.extensions.value.min_bet_multiplier.valid()
        && !o.new_parameters.extensions.value.max_bet_multiplier.valid() )
       FC_ASSERT( *o.new_parameters.extensions.value.min_bet_multiplier < dgpo->parameters.max_bet_multiplier() );
   if( !o.new_parameters.extensions.value.min_bet_multiplier.valid()
        && o.new_parameters.extensions.value.max_bet_multiplier.valid() )
       FC_ASSERT( dgpo->parameters.min_bet_multiplier() < *o.new_parameters.extensions.value.max_bet_multiplier );

   if( o.new_parameters.extensions.value.son_heartbeat_frequency.valid() ) {

       if( o.new_parameters.extensions.value.son_deregister_time.valid() ) {
           if( *o.new_parameters.extensions.value.son_heartbeat_frequency > *o.new_parameters.extensions.value.son_deregister_time )
               FC_ASSERT( *o.new_parameters.extensions.value.son_heartbeat_frequency  > *o.new_parameters.extensions.value.son_deregister_time );
       }else {
           if( *o.new_parameters.extensions.value.son_heartbeat_frequency > dgpo->parameters.son_deregister_time() )
               FC_ASSERT( *o.new_parameters.extensions.value.son_heartbeat_frequency  > dgpo->parameters.son_deregister_time() );
       }

       if( o.new_parameters.extensions.value.son_down_time.valid() ) {
           if( *o.new_parameters.extensions.value.son_heartbeat_frequency > *o.new_parameters.extensions.value.son_down_time )
               FC_ASSERT( *o.new_parameters.extensions.value.son_heartbeat_frequency  > *o.new_parameters.extensions.value.son_down_time );
       }else {
           if( *o.new_parameters.extensions.value.son_heartbeat_frequency > dgpo->parameters.son_down_time() )
               FC_ASSERT( *o.new_parameters.extensions.value.son_heartbeat_frequency  > dgpo->parameters.son_down_time() );
       }

      if( o.new_parameters.extensions.value.son_pay_time.valid() ) {
          if( *o.new_parameters.extensions.value.son_heartbeat_frequency > *o.new_parameters.extensions.value.son_pay_time )
               FC_ASSERT( *o.new_parameters.extensions.value.son_heartbeat_frequency  > *o.new_parameters.extensions.value.son_pay_time );
       }else {
          if( *o.new_parameters.extensions.value.son_heartbeat_frequency > dgpo->parameters.son_pay_time() )
               FC_ASSERT( *o.new_parameters.extensions.value.son_heartbeat_frequency  > dgpo->parameters.son_pay_time() );
       }
   }

   if( o.new_parameters.extensions.value.gpos_period_start.valid() ) {
      if( *o.new_parameters.extensions.value.gpos_period_start != dgpo->parameters.gpos_period_start() )
            FC_ASSERT( "Changing gpos_period_start is not allowed" );
   }

   if( o.new_parameters.extensions.value.son_account.valid() ) {
        if( *o.new_parameters.extensions.value.son_account != dgpo->parameters.son_account() )
              FC_ASSERT( "Changing son_account is not allowed" );
   }

   if( o.new_parameters.extensions.value.btc_asset.valid() ) {
        if( *o.new_parameters.extensions.value.btc_asset != dgpo->parameters.btc_asset() )
              FC_ASSERT( "Changing btc_asset is not allowed" );
   }

   if( o.new_parameters.extensions.value.hbd_asset.valid() ) {
        if( *o.new_parameters.extensions.value.hbd_asset != dgpo->parameters.hbd_asset() )
              FC_ASSERT( "Changing hbd_asset is not allowed" );
   }

   if( o.new_parameters.extensions.value.hive_asset.valid() ) {
        if( *o.new_parameters.extensions.value.hive_asset != dgpo->parameters.hive_asset() )
              FC_ASSERT( "Changing hive_asset is not allowed" );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result committee_member_update_global_parameters_evaluator::do_apply(const committee_member_update_global_parameters_operation& o)
{ try {
   db().modify(*dgpo, [&o](global_property_object& p) {
      p.pending_parameters = o.new_parameters;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

} } // graphene::chain
