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

#include <graphene/chain/protocol/confidential.hpp>
#include <graphene/chain/protocol/authority.hpp>
#include <graphene/chain/protocol/types.hpp>

#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/crypto/elliptic.hpp>

namespace graphene { namespace chain {

/**
 * @class blinded_balance_object
 * @brief tracks a blinded balance commitment
 * @ingroup object
 * @ingroup protocol
 */
class blinded_balance_object : public graphene::db::abstract_object<blinded_balance_object>
{
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_blinded_balance_object_type;

      fc::ecc::commitment_type                commitment;
      asset_id_type                           asset_id;
      authority                               owner;
};

struct by_asset;
struct by_owner;
struct by_commitment;

/**
 * @ingroup object_index
 */
typedef multi_index_container<
   blinded_balance_object,
   indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
      ordered_unique< tag<by_commitment>, member<blinded_balance_object, commitment_type, &blinded_balance_object::commitment> >
   >
> blinded_balance_object_multi_index_type;
typedef generic_index<blinded_balance_object, blinded_balance_object_multi_index_type> blinded_balance_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::blinded_balance_object, (graphene::db::object), (commitment)(asset_id)(owner) )

/** Confidential transactions */
namespace graphene { namespace chain {

/**
 * @class confidential_tx_object
 * @brief tracks confidential transactions
 * @ingroup object
 * @ingroup protocol
 */

class confidential_tx_object : public graphene::db::abstract_object<confidential_tx_object>
{
  public:
    static const uint8_t space_id = implementation_ids;
    static const uint8_t type_id  = impl_confidential_tx_object_type;

    fc::ecc::commitment_type commitment;
    public_key_type          tx_key;
    public_key_type          owner;
    vector<char>             data;
    range_proof_type         range_proof;
    vector<char>             message;
    bool                     unspent;
    fc::time_point_sec       timestamp;
    uint32_t                 block_number;
};

struct by_tx;
struct by_unspent;
struct by_time;
struct by_block_number;

/**
 * @ingroup object_index
 */
typedef multi_index_container<
    confidential_tx_object,
    indexed_by<
        ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>,
        ordered_unique<tag<by_commitment>, member<confidential_tx_object, commitment_type, &confidential_tx_object::commitment>>,
        ordered_unique<tag<by_tx>, member<confidential_tx_object, public_key_type, &confidential_tx_object::tx_key>>,
        ordered_non_unique<tag<by_unspent>, member<confidential_tx_object, bool, &confidential_tx_object::unspent>>,
        ordered_non_unique<tag<by_time>, member<confidential_tx_object, fc::time_point_sec, &confidential_tx_object::timestamp>>,
        ordered_non_unique<tag<by_block_number>, member<confidential_tx_object, uint32_t, &confidential_tx_object::block_number>>>>
    confidential_tx_object_multi_index_type;

typedef generic_index<confidential_tx_object, confidential_tx_object_multi_index_type> confidential_tx_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::confidential_tx_object, (graphene::db::object), (commitment)(tx_key)(owner)(data)(range_proof)(message)(unspent)(timestamp)(block_number) )
