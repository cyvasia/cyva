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

#include <graphene/chain/protocol/authority.hpp>
#include <graphene/app/impacted.hpp>

namespace graphene { namespace app {

using namespace fc;
using namespace graphene::chain;

// TODO:  Review all of these, especially no-ops
struct get_impacted_account_visitor
{
   flat_set<account_id_type>& _impacted;
   get_impacted_account_visitor( flat_set<account_id_type>& impact ):_impacted(impact) {}
   typedef void result_type;

   void operator()( const transfer_operation& op )
   {
      add_authority_accounts( _impacted, authority(1, op.to, 1) );
   }

   void operator()( const set_transfer_freeze_block_operation& ) { }

   void operator()( const account_create_operation& op )
   {
      _impacted.insert( op.registrar );
      add_authority_accounts( _impacted, op.owner );
   }

   void operator()( const account_update_operation& op )
   {
      _impacted.insert( op.account );
      if( op.owner )
         add_authority_accounts( _impacted, *(op.owner) );
   }

   void operator()( const miner_create_operation& op )
   {
      _impacted.insert( op.miner_account );
   }
   void operator()( const miner_update_operation& op )
   {
      _impacted.insert( op.miner_account );
   }
   void operator()( const miner_update_global_parameters_operation& ){}

   void operator()( const proposal_create_operation& op )
   {
      vector<authority> other;
      for( const auto& proposed_op : op.proposed_ops )
         operation_get_required_authorities( proposed_op.op, _impacted, _impacted, other );
      for( auto& o : other )
         add_authority_accounts( _impacted, o );
   }

   void operator()( const proposal_update_operation& ) {}
   void operator()( const proposal_delete_operation& ) {}

   void operator()( const vesting_balance_create_operation& op )
   {
      _impacted.insert( op.owner );
   }

   void operator()( const vesting_balance_withdraw_operation& ) {}

   void operator()( const transfer_to_blind_operation& op ) { _impacted.insert( op.from ); for( const auto& out : op.outputs ) add_authority_accounts( _impacted, out.owner ); }
   void operator()( const blind_transfer_operation& op ) { for( const auto& in : op.inputs ) add_authority_accounts( _impacted, in.owner ); for( const auto& out : op.outputs ) add_authority_accounts( _impacted, out.owner ); }
   void operator()( const transfer_from_blind_operation& op ) { _impacted.insert( op.to ); for( const auto& in : op.inputs ) add_authority_accounts( _impacted, in.owner ); }

   void operator()( const rfu_operation_4& ) {}
   void operator()( const rfu_operation_5& ) {}
   void operator()( const rfu_operation_6& ) {}
   void operator()( const rfu_operation_7& ) {}
   void operator()( const rfu_operation_8& ) {}
   void operator()( const rfu_operation_9& ) {}
   void operator()( const rfu_operation_10& ) {}
   void operator()( const rfu_operation_11& ) {}
   void operator()( const rfu_operation_12& ) {}
   void operator()( const rfu_operation_13& ) {}
   void operator()( const rfu_operation_14& ) {}
   void operator()( const rfu_operation_15& ) {}
   void operator()( const rfu_operation_16& ) {}
   void operator()( const rfu_operation_17& ) {}
   void operator()( const rfu_operation_18& ) {}
   void operator()( const rfu_operation_19& ) {}
   void operator()( const rfu_operation_20& ) {}
   void operator()( const rfu_operation_21& ) {}
   void operator()( const rfu_operation_22& ) {}
   void operator()( const rfu_operation_23& ) {}
   void operator()( const rfu_operation_24& ) {}
   void operator()( const rfu_operation_25& ) {}
   void operator()( const rfu_operation_26& ) {}
   void operator()( const rfu_operation_27& ) {}
   void operator()( const rfu_operation_28& ) {}
   void operator()( const rfu_operation_29& ) {}
   void operator()( const rfu_operation_30& ) {}
   void operator()( const rfu_operation_31& ) {}
   void operator()( const rfu_operation_32& ) {}
   void operator()( const rfu_operation_33& ) {}
   void operator()( const rfu_operation_34& ) {}
   void operator()( const rfu_operation_35& ) {}
   void operator()( const rfu_operation_36& ) {}
   void operator()( const rfu_operation_37& ) {}
   void operator()( const rfu_operation_38& ) {}
   void operator()( const rfu_operation_39& ) {}
   void operator()( const rfu_operation_40& ) {}
   void operator()( const rfu_operation_41& ) {}
   void operator()( const rfu_operation_42& ) {}
   void operator()( const rfu_operation_43& ) {}
   void operator()( const rfu_operation_44& ) {}
   void operator()( const rfu_operation_45& ) {}
   void operator()( const rfu_operation_46& ) {}
   void operator()( const rfu_operation_47& ) {}
   void operator()( const rfu_operation_48& ) {}
   void operator()( const rfu_operation_49& ) {}
   void operator()( const rfu_operation_50& ) {}
   void operator()( const rfu_operation_51& ) {}
   void operator()( const rfu_operation_52& ) {}
   void operator()( const rfu_operation_53& ) {}
   void operator()( const rfu_operation_54& ) {}
   void operator()( const rfu_operation_55& ) {}
   void operator()( const rfu_operation_56& ) {}
   void operator()( const rfu_operation_57& ) {}
   void operator()( const rfu_operation_58& ) {}
   void operator()( const rfu_operation_59& ) {}
   void operator()( const rfu_operation_60& ) {}
   void operator()( const rfu_operation_61& ) {}
   void operator()( const rfu_operation_62& ) {}
   void operator()( const rfu_operation_63& ) {}
   void operator()( const rfu_operation_64& ) {}
};

void operation_get_impacted_accounts( const operation& op, flat_set<account_id_type>& result )
{
   get_impacted_account_visitor vtor = get_impacted_account_visitor( result );
   op.visit( vtor );
}

void transaction_get_impacted_accounts( const transaction& tx, flat_set<account_id_type>& result )
{
   for( const auto& op : tx.operations )
      operation_get_impacted_accounts( op, result );
}

} }
