/* (c) 2018 CYVA. For details refer to LICENSE */
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

#include <fc/smart_ref_impl.hpp>

#include <graphene/chain/account_evaluator.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/internal_exceptions.hpp>
#include <graphene/chain/transaction_detail_object.hpp>

#include <algorithm>

namespace graphene { namespace chain {

void verify_authority_accounts( const database& db, const authority& a )
{
   const auto& chain_params = db.get_global_properties().parameters;
   GRAPHENE_ASSERT(
      a.num_auths() <= chain_params.maximum_authority_membership,
      internal_verify_auth_max_auth_exceeded,
      "Maximum authority membership exceeded" );
   for( const auto& acnt : a.account_auths )
   {
      GRAPHENE_ASSERT( db.find_object( acnt.first ) != nullptr,
         internal_verify_auth_account_not_found,
         "Account ${a} specified in authority does not exist",
         ("a", acnt.first) );
   }
}

void verify_account_votes( const database& db, const account_options& options )
{
   // ensure account's votes satisfy requirements
   // NB only the part of vote checking that requires chain state is here,
   // the rest occurs in account_options::validate()

   const auto& gpo = db.get_global_properties();
   const auto& chain_params = gpo.parameters;

   FC_ASSERT( options.num_miner <= chain_params.maximum_miner_count,
              "Voted for more miners than currently allowed (${c})", ("c", chain_params.maximum_miner_count) );

   uint32_t max_vote_id = gpo.next_available_vote_id;
   for( auto id : options.votes )
   {
      FC_ASSERT( id < max_vote_id );
   }

}


void_result account_create_evaluator::do_evaluate( const account_create_operation& op )
{ try {
   database& d = db();

   FC_ASSERT( d.find_object(op.options.voting_account), "Invalid proxy account specified." );

   try
   {
      verify_authority_accounts( d, op.owner );
   }
   GRAPHENE_RECODE_EXC( internal_verify_auth_max_auth_exceeded, account_create_max_auth_exceeded )
   GRAPHENE_RECODE_EXC( internal_verify_auth_account_not_found, account_create_auth_account_not_found )

   auto keys = op.owner.get_keys();
   FC_ASSERT(keys.empty() == false);
   std::string to_name(keys.front());

   auto& acnt_indx = d.get_index_type<account_index>();
   if( to_name.size() )
   {
      auto current_account_itr = acnt_indx.indices().get<by_name>().find( to_name );
      FC_ASSERT( current_account_itr == acnt_indx.indices().get<by_name>().end() );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type account_create_evaluator::do_apply( const account_create_operation& o )
{ try {

   database& d = db();

   const auto& new_acnt_object = db().create<account_object>( [&]( account_object& obj ){
         obj.registrar = o.registrar;

         auto& params = db().get_global_properties().parameters;

         auto keys = o.owner.get_keys();
         FC_ASSERT(keys.empty() == false);
         std::string to_name(keys.front());

         obj.name             = to_name;
         obj.owner            = o.owner;
         obj.options          = o.options;
         obj.statistics = db().create<account_statistics_object>([&](account_statistics_object& s){s.owner = obj.id;}).id;
   });

   const auto& dynamic_properties = db().get_dynamic_global_properties();
   db().modify(dynamic_properties, [](dynamic_global_property_object& p) {
      ++p.accounts_registered_this_interval;
   });

   const auto& global_properties = db().get_global_properties();

   db().create<transaction_detail_object>([&o, &new_acnt_object, &d](transaction_detail_object& obj)
                                          {
                                             obj.m_operation_type = (uint8_t)transaction_detail_object::account_create;

                                             obj.m_from_account = o.registrar;
                                             obj.m_to_account = new_acnt_object.id;
                                             obj.m_transaction_amount = asset();
                                             obj.m_transaction_fee = o.fee;
                                             obj.m_str_description = string();
                                             obj.m_timestamp = d.head_block_time();
                                          });

   return new_acnt_object.id;
} FC_CAPTURE_AND_RETHROW((o)) }


void_result account_update_evaluator::do_evaluate( const account_update_operation& o )
{ try {
   database& d = db();
   try
   {
      if( o.owner )  verify_authority_accounts( d, *o.owner );
   }
   GRAPHENE_RECODE_EXC( internal_verify_auth_max_auth_exceeded, account_update_max_auth_exceeded )
   GRAPHENE_RECODE_EXC( internal_verify_auth_account_not_found, account_update_auth_account_not_found )

   acnt = &o.account(d);

   if( o.new_options.valid() ) {
      verify_account_votes(d, *o.new_options);
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_update_evaluator::do_apply( const account_update_operation& o )
{ try {
   database& d = db();
   d.modify( *acnt, [&](account_object& a){
      if( o.owner )
      {
         a.owner = *o.owner;
         a.top_n_control_flags = 0;
      }
      if( o.new_options ){
         a.options = *o.new_options;

      }

   });
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

} } // graphene::chain
