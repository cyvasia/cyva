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
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/protocol/confidential.hpp>
#include <graphene/chain/confidential_evaluator.hpp>
#include <graphene/chain/confidential_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/transaction_detail_object.hpp>
#include <boost/range/combine.hpp>

namespace graphene { namespace chain {

void_result transfer_to_blind_evaluator::do_evaluate( const transfer_to_blind_operation& o )
{ try {
   const auto& d = db();

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
       FC_ASSERT(out.commitment != fc::ecc::commitment_type( ), "commitment cannot be 0");
       range_proof_type range_proof{};
       vector<char>     message{};
       if(2 == out.extension.which( ))
       {
           auto ext    = out.extension.get<confidential_tx_x>( );
           range_proof = ext.range_proof;
           message     = ext.message;
       }
       else if(1 == out.extension.which( ))
       {
           range_proof = out.extension.get<range_proof_type>( );
       }

       db( ).create<confidential_tx_object>([&](confidential_tx_object &obj) {
           obj.commitment   = out.commitment;
           obj.tx_key       = out.tx_key;
           obj.owner        = out.owner;
           obj.range_proof  = range_proof;
           obj.data         = out.data;
           obj.message      = message;
           obj.unspent      = true;
           obj.timestamp    = db( ).head_block_time( );
           obj.block_number = db( ).head_block_num( );
       });
   }

   db().create<transaction_detail_object>([&](transaction_detail_object& obj)
                                          {
                                              obj.m_operation_type = (uint8_t)transaction_detail_object::confidential_transfer;

                                              obj.m_from_account = op.from;
                                              obj.m_to_account = GRAPHENE_NULL_ACCOUNT;
                                              obj.m_transaction_amount = op.amount;
                                              obj.m_transaction_fee = op.fee;
                                              obj.m_str_description = "confidential transfer";
                                              obj.m_timestamp = db().head_block_time();
                                              obj.m_block_number = db().head_block_num();
                                          });
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void transfer_to_confidential_evaluator::pay_fee() { generic_evaluator::pay_fee(); }


void_result transfer_from_confidential_evaluator::do_evaluate( const operation_type& op )
{ try { return void_result(); } FC_CAPTURE_AND_RETHROW( (op) ) }


void_result transfer_from_confidential_evaluator::do_apply( const operation_type& op )
{ try {

   const auto& cti = db().get_index_type<confidential_tx_index>();
   const auto& ci = cti.indices().get<by_commitment>();
   const auto& add = op.fee.asset_id(db()).dynamic_asset_data_id(db());  // verify fee is a legit asset
   const auto& ai = db().get_index_type<account_index>().indices().get<by_name>();

   auto outputs = op.outputs;
   for(auto b : boost::combine(op.to, op.amount))
   {
       confidential_tx c;
       auto            to = boost::get<0>(b);

       c.commitment = commitment_type( );
       c.owner      = to;
       vector<char> data;
       data.resize(16);
       memcpy(&data[0], &boost::get<1>(b).amount.value, 8);
       auto unit = uint64_t(boost::get<1>(b).asset_id);
       memcpy(&data[8], &unit, 8);
       c.data   = data;
       c.tx_key = c.owner;

       outputs.push_back(c);
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
      FC_ASSERT(itr->unspent, "already spent commitment", ("commitment", in.commitment));

      db().modify( *itr, [&]( confidential_tx_object& obj ){
          obj.unspent = false;
          obj.range_proof.clear();
      });
   }
   for(const auto& out : outputs)
   {
       if(out.commitment != fc::ecc::commitment_type())
       {
           range_proof_type range_proof{};
           vector<char>     message{};
           if(2 == out.extension.which( ))
           {
               auto ext    = out.extension.get<confidential_tx_x>( );
               range_proof = ext.range_proof;
               message     = ext.message;
           }
           else if(1 == out.extension.which( ))
           {
               range_proof = out.extension.get<range_proof_type>( );
           }

           db( ).create<confidential_tx_object>([&](confidential_tx_object &obj) {
               obj.commitment   = out.commitment;
               obj.tx_key       = out.tx_key;
               obj.owner        = out.owner;
               obj.range_proof  = range_proof;
               obj.data         = out.data;
               obj.message      = message;
               obj.unspent      = true;
               obj.timestamp    = db( ).head_block_time( );
               obj.block_number = db( ).head_block_num( );
           });
       }
       else
       {
           auto to = out.owner;
           std::string to_name(to);

           auto itr_name = ai.find(to_name);
           FC_ASSERT(itr_name != ai.end(), "address not found", ("address", *itr_name));

           uint64_t value = 0;
           uint64_t unit  = 0;
           memcpy(&value, &out.data[0], 8);
           memcpy(&unit, &out.data[8], 8);
           asset amount{share_type(value), object_id_type(unit)};

           db().adjust_balance(itr_name->get_id(), amount);
           db().modify( add, [&]( asset_dynamic_data_object& obj )
                       {
                           obj.confidential_supply -= amount.amount;
                           FC_ASSERT( obj.confidential_supply >= 0 );
                       });

           db().create<transaction_detail_object>([&](transaction_detail_object& obj)
                                                  {
                                                      obj.m_operation_type = (uint8_t)transaction_detail_object::confidential_transfer;

                                                      obj.m_from_name = std::string(out.tx_key);
                                                      obj.m_to_name = to_name;
                                                      obj.m_to_account = itr_name->get_id();
                                                      obj.m_transaction_amount = amount;
                                                      obj.m_transaction_fee = asset(0, op.fee.asset_id);
                                                      obj.m_str_description = "confidential transfer";
                                                      if(2 == out.extension.which())
                                                      {
                                                          auto ext = out.extension.get<confidential_tx_x>();
                                                          memo_data md;
                                                          string message(ext.message.begin(), ext.message.end());
                                                          md.set_message(fc::ecc::private_key(), fc::ecc::public_key(), message);
                                                          obj.m_transaction_encrypted_memo = md;
                                                      }
                                                      obj.m_timestamp = db().head_block_time();
                                                      obj.m_block_number = db().head_block_num();
                                                  });
       }
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void transfer_from_confidential_evaluator::pay_fee()
{
      generic_evaluator::pay_fee();
}
} } // graphene::chain
