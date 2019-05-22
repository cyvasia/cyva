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
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/protocol/confidential.hpp>
#include <graphene/chain/confidential_evaluator.hpp>
#include <graphene/chain/confidential_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/hardfork.hpp>

#include <boost/range/combine.hpp>

namespace graphene { namespace chain {

void_result transfer_to_blind_evaluator::do_evaluate( const transfer_to_blind_operation& o )
{ try {
   const auto& d = db();

   const auto& atype = o.amount.asset_id(db());

   for( const auto& out : o.outputs )
   {
      for( const auto& a : out.owner.account_auths )
         a.first(d); // verify all accounts exist and are valid
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }


void_result transfer_to_blind_evaluator::do_apply( const transfer_to_blind_operation& o ) 
{ try {
   db().adjust_balance( o.from, -o.amount ); 

   const auto& add = o.amount.asset_id(db()).dynamic_asset_data_id(db());  // verify fee is a legit asset 
   db().modify( add, [&]( asset_dynamic_data_object& obj ){
      obj.confidential_supply += o.amount.amount;
      FC_ASSERT( obj.confidential_supply >= 0 );
   });
   for( const auto& out : o.outputs )
   {
      db().create<blinded_balance_object>( [&]( blinded_balance_object& obj ){
          obj.asset_id   = o.amount.asset_id;
          obj.owner      = out.owner;
          obj.commitment = out.commitment;
      });
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void transfer_to_blind_evaluator::pay_fee()
{
      generic_evaluator::pay_fee();
}

void_result transfer_from_blind_evaluator::do_evaluate( const transfer_from_blind_operation& o )
{ try {
   const auto& d = db();
   o.fee.asset_id(d);  // verify fee is a legit asset 
   const auto& bbi = d.get_index_type<blinded_balance_index>();
   const auto& cidx = bbi.indices().get<by_commitment>();
   for( const auto& in : o.inputs )
   {
      auto itr = cidx.find( in.commitment );
      FC_ASSERT( itr != cidx.end() );
      FC_ASSERT( itr->asset_id == o.fee.asset_id );
      FC_ASSERT( itr->owner == in.owner );
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result transfer_from_blind_evaluator::do_apply( const transfer_from_blind_operation& o ) 
{ try {
   db().adjust_balance( o.fee_payer(), o.fee ); 
   db().adjust_balance( o.to, o.amount ); 
   const auto& bbi = db().get_index_type<blinded_balance_index>();
   const auto& cidx = bbi.indices().get<by_commitment>();
   for( const auto& in : o.inputs )
   {
      auto itr = cidx.find( in.commitment );
      FC_ASSERT( itr != cidx.end() );
      db().remove( *itr );
   }
   const auto& add = o.amount.asset_id(db()).dynamic_asset_data_id(db());  // verify fee is a legit asset 
   db().modify( add, [&]( asset_dynamic_data_object& obj ){
      obj.confidential_supply -= o.amount.amount + o.fee.amount;
      FC_ASSERT( obj.confidential_supply >= 0 );
   });
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void transfer_from_blind_evaluator::pay_fee()
{
      generic_evaluator::pay_fee();
}

void_result blind_transfer_evaluator::do_evaluate( const blind_transfer_operation& o )
{ try {
   const auto& d = db();
   o.fee.asset_id(db());  // verify fee is a legit asset 
   const auto& bbi = db().get_index_type<blinded_balance_index>();
   const auto& cidx = bbi.indices().get<by_commitment>();
   for( const auto& out : o.outputs )
   {
      for( const auto& a : out.owner.account_auths )
         a.first(d); // verify all accounts exist and are valid
   }
   for( const auto& in : o.inputs )
   {
      auto itr = cidx.find( in.commitment );
      GRAPHENE_ASSERT( itr != cidx.end(), blind_transfer_unknown_commitment, "", ("commitment",in.commitment) );
      FC_ASSERT( itr->asset_id == o.fee.asset_id );
      FC_ASSERT( itr->owner == in.owner );
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result blind_transfer_evaluator::do_apply( const blind_transfer_operation& o ) 
{ try {
   db().adjust_balance( o.fee_payer(), o.fee ); // deposit the fee to the temp account
   const auto& bbi = db().get_index_type<blinded_balance_index>();
   const auto& cidx = bbi.indices().get<by_commitment>();
   for( const auto& in : o.inputs )
   {
      auto itr = cidx.find( in.commitment );
      GRAPHENE_ASSERT( itr != cidx.end(), blind_transfer_unknown_commitment, "", ("commitment",in.commitment) );
      db().remove( *itr );
   }
   for( const auto& out : o.outputs )
   {
      db().create<blinded_balance_object>( [&]( blinded_balance_object& obj ){
          obj.asset_id   = o.fee.asset_id;
          obj.owner      = out.owner;
          obj.commitment = out.commitment;
      });
   }
   const auto& add = o.fee.asset_id(db()).dynamic_asset_data_id(db());  
   db().modify( add, [&]( asset_dynamic_data_object& obj ){
      obj.confidential_supply -= o.fee.amount;
      FC_ASSERT( obj.confidential_supply >= 0 );
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void blind_transfer_evaluator::pay_fee()
{
      generic_evaluator::pay_fee();
}

void_result transfer_to_confidential_evaluator::do_evaluate( const operation_type& op )
{ try { return void_result(); } FC_CAPTURE_AND_RETHROW( (op) ) }


void_result transfer_to_confidential_evaluator::do_apply( const operation_type& op )
{ try {

   const auto& add = op.amount.asset_id(db()).dynamic_asset_data_id(db());  // verify fee is a legit asset

   db().adjust_balance( op.from, -op.amount );
   db().modify( add, [&]( asset_dynamic_data_object& obj )
   {
      obj.confidential_supply += op.amount.amount;
      FC_ASSERT( obj.confidential_supply >= 0 );
   });
   for( const auto& out : op.outputs )
   {
       db( ).create<confidential_tx_object>([&](confidential_tx_object &obj) {
           obj.commitment   = out.commitment;
           obj.tx_key       = out.tx_key;
           obj.owner        = out.owner;
           obj.range_proof  = out.range_proof;
           obj.data         = out.data;
           obj.valid        = true;
           obj.timestamp    = db( ).head_block_time( );
           obj.block_number = db( ).head_block_num( );
       });
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void transfer_to_confidential_evaluator::pay_fee() { generic_evaluator::pay_fee(); }


void_result transfer_from_confidential_evaluator::do_evaluate( const operation_type& op )
{ try { return void_result(); } FC_CAPTURE_AND_RETHROW( (op) ) }


void_result transfer_from_confidential_evaluator::do_apply( const operation_type& op )
{ try {

   const auto& ati = db().get_index_type<confidential_tx_index>();
   const auto& ci = ati.indices().get<by_commitment>();
   const auto& add = op.fee.asset_id(db()).dynamic_asset_data_id(db());  // verify fee is a legit asset

   for (auto b : boost::combine(op.to, op.amount))
   {
       db().adjust_balance(boost::get<0>(b), boost::get<1>(b));
       db().modify( add, [&]( asset_dynamic_data_object& obj )
       {
          obj.confidential_supply -= boost::get<1>(b).amount;
          FC_ASSERT( obj.confidential_supply >= 0 );
       });
   }
   db().adjust_balance(op.fee_payer(), op.fee.amount);
   db().modify( add, [&]( asset_dynamic_data_object& obj )
   {
       obj.confidential_supply -= op.fee.amount;
       FC_ASSERT( obj.confidential_supply >= 0 );
   });

   for(const auto& in : op.inputs)
   {
      auto itr = ci.find(in.commitment);
      GRAPHENE_ASSERT( itr != ci.end(), blind_transfer_unknown_commitment, "", ("commitment", in.commitment) );
      db().modify( *itr, [&]( confidential_tx_object& obj ){
          obj.valid = false;
          obj.range_proof.reset();
      });
   }
   for(const auto& out : op.outputs)
   {
       db( ).create<confidential_tx_object>([&](confidential_tx_object &obj) {
           obj.commitment   = out.commitment;
           obj.tx_key       = out.tx_key;
           obj.owner        = out.owner;
           obj.range_proof  = out.range_proof;
           obj.data         = out.data;
           obj.valid        = true;
           obj.timestamp    = db( ).head_block_time( );
           obj.block_number = db( ).head_block_num( );
       });
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void transfer_from_confidential_evaluator::pay_fee()
{
      generic_evaluator::pay_fee();
}
} } // graphene::chain
