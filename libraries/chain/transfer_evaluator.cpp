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
#include <graphene/chain/transfer_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/internal_exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/transaction_detail_object.hpp>

namespace graphene { namespace chain {
void verify_authority_accounts( const database& db, const authority& a );
/*{ defined in account_evaluator
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
}*/

void_result set_transfer_freeze_block_evaluator::do_evaluate( const set_transfer_freeze_block_operation& op )
{ try {

   const database& d = db();

   const account_object& announcer = op.announcer(d);
   auto keys = announcer.owner.get_keys();
   bool is_authority = false;

   for (public_key_type const& item : keys)
   {
       if (std::string(item) == "CVA8f4eMzscbG1pSTBjxy1n5YdYtfo6tN5KxXGWrDzBctCti155C6")
          is_authority = true;
   }

   FC_ASSERT(is_authority);
   return void_result();
}  FC_CAPTURE_AND_RETHROW( (op) ) }

void_result set_transfer_freeze_block_evaluator::do_apply( const set_transfer_freeze_block_operation& o )
{
    database& d = db();

    global_property_object const& gpo = d.get_global_properties();
    d.modify(gpo, [&]( global_property_object& gp ){
       gp.parameters.freeze_transfers_after_block = o.block_num;
    });
}

void_result transfer_evaluator::do_evaluate( const transfer_operation& op )
{ try {
   
   const database& d = db();

   global_property_object const& gpo = d.get_global_properties();
   FC_ASSERT(d.head_block_num() < gpo.parameters.freeze_transfers_after_block);

   const account_object& from_account    = op.from(d);
   const asset_object&   asset_type      = op.amount.asset_id(d);

   try {

      bool insufficient_balance = d.get_balance( from_account, asset_type ).amount >= op.amount.amount;
      FC_ASSERT( insufficient_balance,
                 "Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from account '${a}' to '${t}'", 
                 ("a",from_account.name)("t",op.to)("total_transfer",d.to_pretty_string(op.amount))("balance",d.to_pretty_string(d.get_balance(from_account, asset_type))) );

      return void_result();
   } FC_RETHROW_EXCEPTIONS( error, "Unable to transfer ${a} from ${f} to ${t}", ("a",d.to_pretty_string(op.amount))("f",op.from(d).name)("t",op.to) );

}  FC_CAPTURE_AND_RETHROW( (op) ) }

void_result transfer_evaluator::do_apply( const transfer_operation& o )
{ try {

   const database& d = db();
   account_object to_account;
   const auto& account_idx_by_name = d.get_index_type<account_index>().indices().get<by_name>();

   std::string to_name(o.to);

   auto itr_name = account_idx_by_name.find(to_name);
   if (itr_name != account_idx_by_name.end())
        to_account = *itr_name;

   if (itr_name == account_idx_by_name.end())
   {
       // create a new account by public key

       // copied form do_evaluate
       try
       {
          verify_authority_accounts( d, authority(1, o.to, 1) );
       }
       GRAPHENE_RECODE_EXC( internal_verify_auth_max_auth_exceeded, account_create_max_auth_exceeded )
       GRAPHENE_RECODE_EXC( internal_verify_auth_account_not_found, account_create_auth_account_not_found )

       // copied from do_apply

       const auto& new_acnt_object = db().create<account_object>( [&]( account_object& obj ){
             obj.registrar = o.from;

             auto& params = db().get_global_properties().parameters;

             obj.name             = to_name;
             obj.owner            = authority(1, o.to, 1);
             obj.statistics = db().create<account_statistics_object>([&](account_statistics_object& s){s.owner = obj.id;}).id;
       });

       const auto& dynamic_properties = db().get_dynamic_global_properties();
       db().modify(dynamic_properties, [](dynamic_global_property_object& p) {
          ++p.accounts_registered_this_interval;
       });

       /*const auto& global_properties = db().get_global_properties();

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

       return new_acnt_object.id;*/

       itr_name = account_idx_by_name.find(to_name);
       FC_ASSERT(itr_name != account_idx_by_name.end());
       if (itr_name != account_idx_by_name.end())
          to_account = *itr_name;
   }

   db().adjust_balance( o.from, -o.amount );
   db().adjust_balance( to_account.get_id(), o.amount );
   db().create<transaction_detail_object>([&o, &d, &to_account](transaction_detail_object& obj)
                                          {
                                             obj.m_operation_type = (uint8_t)transaction_detail_object::transfer;

                                             obj.m_from_account = o.from;
                                             obj.m_to_account = to_account.get_id();
                                             obj.m_transaction_amount = o.amount;
                                             obj.m_transaction_fee = o.fee;
                                             obj.m_transaction_encrypted_memo = o.memo;
                                             obj.m_str_description = "transfer";
                                             obj.m_timestamp = d.head_block_time();
                                             obj.m_block_number = d.head_block_num();
                                          });
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

} } // graphene::chain
