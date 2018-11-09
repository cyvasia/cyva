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
#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/account.hpp>
#include <graphene/chain/protocol/proposal.hpp>
#include <graphene/chain/protocol/transfer.hpp>
#include <graphene/chain/protocol/vesting.hpp>
#include <graphene/chain/protocol/miner.hpp>
#include <graphene/chain/protocol/cyva.hpp>
#include <graphene/chain/protocol/rfu.hpp>

namespace graphene { namespace chain {

   /**
    * @ingroup operations
    *
    * Defines the set of valid operations as a discriminated union type.
    */
   typedef fc::static_variant<
            transfer_operation,
            account_create_operation,
            account_update_operation,
            miner_create_operation,
            miner_update_operation,
            miner_update_global_parameters_operation,
            proposal_create_operation,
            proposal_update_operation, //7
            proposal_delete_operation,
            set_transfer_freeze_block_operation,
            vesting_balance_create_operation,//10
            vesting_balance_withdraw_operation,
            rfu_operation_1,
            rfu_operation_2,
            rfu_operation_3,
            rfu_operation_4,
            rfu_operation_5,
            rfu_operation_6,
            rfu_operation_7,
            rfu_operation_8,
            rfu_operation_9,
            rfu_operation_10,
            rfu_operation_11,
            rfu_operation_12,
            rfu_operation_13,
            rfu_operation_14,
            rfu_operation_15,
            rfu_operation_16,
            rfu_operation_17,
            rfu_operation_18,
            rfu_operation_19,
            rfu_operation_20,
            rfu_operation_21,
            rfu_operation_22,
            rfu_operation_23,
            rfu_operation_24,
            rfu_operation_25,
            rfu_operation_26,
            rfu_operation_27,
            rfu_operation_28,
            rfu_operation_29,
            rfu_operation_30,
            rfu_operation_31,
            rfu_operation_32,
            rfu_operation_33,
            rfu_operation_34,
            rfu_operation_35,
            rfu_operation_36,
            rfu_operation_37,
            rfu_operation_38,
            rfu_operation_39,
            rfu_operation_40,
            rfu_operation_41,
            rfu_operation_42,
            rfu_operation_43,
            rfu_operation_44,
            rfu_operation_45,
            rfu_operation_46,
            rfu_operation_47,
            rfu_operation_48,
            rfu_operation_49,
            rfu_operation_50,
            rfu_operation_51,
            rfu_operation_52,
            rfu_operation_53,
            rfu_operation_54,
            rfu_operation_55,
            rfu_operation_56,
            rfu_operation_57,
            rfu_operation_58,
            rfu_operation_59,
            rfu_operation_60,
            rfu_operation_61,
            rfu_operation_62,
            rfu_operation_63,
            rfu_operation_64
         > operation;

   /// @} // operations group

   /**
    *  Appends required authorites to the result vector.  The authorities appended are not the
    *  same as those returned by get_required_auth 
    *
    *  @return a set of required authorities for @ref op
    */
   void operation_get_required_authorities( const operation& op, 
                                            flat_set<account_id_type>& active,
                                            flat_set<account_id_type>& owner,
                                            vector<authority>&  other );

   void operation_validate( const operation& op );

   /**
    *  @brief necessary to support nested operations inside the proposal_create_operation
    */
   struct op_wrapper
   {
      public:
         op_wrapper(const operation& op = operation()):op(op){}
         operation op;
   };

} } // graphene::chain

FC_REFLECT_TYPENAME( graphene::chain::operation )
FC_REFLECT( graphene::chain::op_wrapper, (op) )
