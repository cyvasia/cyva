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

#include <graphene/app/full_account.hpp>

#include <graphene/chain/protocol/types.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/miner_object.hpp>
#include <graphene/chain/budget_record_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/transaction_detail_object.hpp>
#include <graphene/chain/confidential_object.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <utility>

/**
 * @defgroup DatabaseAPI Database API
 */
namespace graphene { namespace app {

      using namespace graphene::chain;
      using namespace std;

      class database_api_impl;

      struct order
      {
         double                     price;
         double                     quote;
         double                     base;
      };

      struct order_book
      {
         string                      base;
         string                      quote;
         vector< order >             bids;
         vector< order >             asks;
      };

      struct market_ticker
      {
         string                     base;
         string                     quote;
         double                     latest;
         double                     lowest_ask;
         double                     highest_bid;
         double                     percent_change;
         double                     base_volume;
         double                     quote_volume;
      };

      struct market_volume
      {
         string                     base;
         string                     quote;
         double                     base_volume;
         double                     quote_volume;
      };

      struct market_trade
      {
         fc::time_point_sec         date;
         double                     price;
         double                     amount;
         double                     value;
      };

/**
 * @brief The database_api class implements the RPC API for the chain database.
 *
 * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
 * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
 * the @ref network_broadcast_api.
 */
      class database_api : public fc::api_base<database_api>
      {
      public:
         database_api(graphene::chain::database& db);
         ~database_api();

         /////////////
         // Objects //
         /////////////

         /**
          * @brief Get the objects corresponding to the provided IDs
          * @param ids IDs of the objects to retrieve
          * @return The objects retrieved, in the order they are mentioned in ids
          * @ingroup DatabaseAPI
          *
          * If any of the provided IDs does not map to an object, a null variant is returned in its position.
          */
         fc::variants get_objects(const vector<object_id_type>& ids)const;

         ///////////////////
         // Subscriptions //
         ///////////////////

         /**
          *
          * @param cb
          * @param clear_filter
          * @ingroup DatabaseAPI
          */
         void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );

         /**
          *
          * @param cb
          * @ingroup DatabaseAPI
          */
         void set_pending_transaction_callback( std::function<void(const variant&)> cb );

         /**
          *
          * @param cb
          * @ingroup DatabaseAPI
          */
         void set_block_applied_callback( std::function<void(const variant& block_id)> cb );

         /**
          * @brief Stop receiving any notifications
          * @ingroup DatabaseAPI
          *
          * This unsubscribes from all subscribed markets and objects.
          */
         void cancel_all_subscriptions();

         /////////////////////////////
         // Blocks and transactions //
         /////////////////////////////

         /**
          * @brief Retrieve a block header
          * @param block_num Height of the block whose header should be returned
          * @return header of the referenced block, or null if no matching block was found
          * @ingroup DatabaseAPI
          */
         optional<block_header> get_block_header(uint32_t block_num)const;

         /**
          * @brief Retrieve a full, signed block
          * @param block_num Height of the block to be returned
          * @return the referenced block, or null if no matching block was found
          * @ingroup DatabaseAPI
          */
         optional<signed_block_with_info> get_block(uint32_t block_num)const;

         /**
          * @brief Retrieve an array of full, signed blocks
          * @param blocks Heights of blocks to be returned
          * @return an array of referenced blocks, or null if no matching block was found
          * @ingroup DatabaseAPI
          */
         vector<pair<uint32_t, optional<signed_block_with_info>>> get_blocks(vector<uint32_t> blocks) const;

         /**
          * @brief used to fetch an individual transaction.
          * @param block_num id of the block
          * @param trx_in_block Specifies the position of the transaction within the block
          * @ingroup DatabaseAPI
          */
         processed_transaction get_transaction( uint32_t block_num, uint32_t trx_in_block )const;

         /**
          * @brief Query the last local block
          * @return the block time
          */
         fc::time_point_sec head_block_time()const;

         /**
          * @brief Query the last local block
          * @return the block
          */
         optional<signed_block> get_head_block()const;

         /**
          * @brief Query local block by timestamp
          * @return the block
          */
         optional<signed_block> get_nearest_block(const string& time_iso_str) const;

         /**
          * @brief If the transaction has not expired, this method will return the transaction for the given ID or
          * it will return NULL if it is not known.  Just because it is not known does not mean it wasn't
          * included in the blockchain.
          * @param id ID of the transaction to retrieve
          * @ingroup DatabaseAPI
          */
         optional<signed_transaction> get_recent_transaction_by_id( const transaction_id_type& id )const;
         map<string, signed_transaction> get_transactions_by_id(vector<string> ids )const;

         /////////////
         // Globals //
         /////////////

         /**
          * @brief Retrieve the @ref chain_property_object associated with the chain
          * @ingroup DatabaseAPI
          */
         chain_property_object get_chain_properties()const;

         /**
          * @brief Retrieve the current @ref global_property_object
          * @ingroup DatabaseAPI
          */
         global_property_object get_global_properties()const;

         /**
          * @brief Retrieve compile-time constants
          * @ingroup DatabaseAPI
          */
         fc::variant_object get_config()const;

         /**
          * @brief Get the chain ID
          * @ingroup DatabaseAPI
          */
         chain_id_type get_chain_id()const;

         /**
          * @brief Retrieve the current @ref dynamic_global_property_object
          * @ingroup DatabaseAPI
          */
         dynamic_global_property_object get_dynamic_global_properties()const;

         //////////
         // Keys //
         //////////

         /**
          * @brief Get all accounts that refer to the key in their owner or active authorities
          * @param key List of public keys
          * @return List of lists of account IDs. One list of account IDs per public key
          * @ingroup DatabaseAPI
          */
         vector<vector<account_id_type>> get_key_references( vector<public_key_type> key )const;

         //////////////
         // Accounts //
         //////////////

         /**
          * @brief Get a list of accounts by ID
          * @param account_ids IDs of the accounts to retrieve
          * @return The accounts corresponding to the provided IDs
          * @ingroup DatabaseAPI
          *
          * This function has semantics identical to @ref get_objects
          */
         vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids)const;

         /**
          * @brief Fetch all objects relevant to the specified accounts and subscribe to updates
          * @param subscribe true to subscribe to updates
          * @param names_or_ids Each item must be the name or ID of an account to retrieve
          * @return Map of string from @ref names_or_ids to the corresponding account
          * @ingroup DatabaseAPI
          *
          * This function fetches all relevant objects for the given accounts, and subscribes to updates to the given
          * accounts. If any of the strings in @ref names_or_ids cannot be tied to an account, that input will be
          * ignored. All other accounts will be retrieved and subscribed.
          */
         std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );

         /**
          * @brief Get an account by name
          * @param name Name of the account to retrieve
          * @return The @ref account_object corresponding to the provided name, or null if no matching content was found
          * @ingroup DatabaseAPI
          */
         optional<account_object> get_account_by_name( string name )const;

         /**
          * @brief Get all accounts that refer to the account id in their owner or active authorities
          * @param account_id
          * @return List of account IDs
          * @ingroup DatabaseAPI
          */
         vector<account_id_type> get_account_references( account_id_type account_id )const;

         /**
          * @brief Get a list of accounts by name
          * @param account_names Names of the accounts to retrieve
          * @return The accounts holding the provided names
          * @ingroup DatabaseAPI
          *
          * This function has semantics identical to @ref get_objects
          */
         vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;

         /**
          * @brief Get names and IDs for registered accounts
          * @param lower_bound_name Lower bound of the first name to return
          * @param limit Maximum number of results to return -- must not exceed 1000
          * @return Map of account names to corresponding IDs
          * @ingroup DatabaseAPI
          */
         map<string,account_id_type> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;

         /**
          * @brief Get names and IDs for registered accounts that match search term
          * @param search_term will try to partially match account name or id
          * @param limit Maximum number of results to return -- must not exceed 1000
          * @return Map of account names to corresponding IDs
          * @ingroup DatabaseAPI
          */
         vector<account_object> search_accounts(const string& search_term, const string order, const object_id_type& id, uint32_t limit) const;

         /**
          * @brief Returns the operations on the named account.
          *
          * This returns a list of transaction detail object, which describe activity on the account.
          *
          * @param account_name the account name to search
          * @param order Sort data by field
          * @param id object_id to start searching from
          * @param limit the number of entries to return (starting from the most recent) (max 100)
          * @returns a list of \c transaction_detail_object
          * @ingroup WalletCLI
          */
         vector<class transaction_detail_object> search_account_history(std::string const& account_name,
                                                                        std::string const& order,
                                                                        std::string const& id,
                                                                        int limit) const;

         //////////////
         // Balances //
         //////////////

         /**
          * @brief Get an account's balances in various assets
          * @param id ID of the account to get balances for
          * @param assets IDs of the assets to get balances of; if empty, get all assets account has a balance in
          * @return Balances of the account
          * @ingroup DatabaseAPI
          */
         vector<asset> get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;

         /**
          * @brief Semantically equivalent to @ref get_account_balances, but takes a name instead of an ID.
          * @param name of the account to get balances for
          * @assets IDs of the assets to get balances of; if empty, get all assets account has a balance in
          * @ingroup DatabaseAPI
          */
         vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const;

         /**
          * @brief Get information about a vesting balance object.
          *
          * @param account_id An account ID.
          * @ingroup DatabaseAPI
          */
         vector<vesting_balance_object> get_vesting_balances( account_id_type account_id )const;
         vector<vesting_balance_object_with_info> get_vesting_balances_with_info(account_id_type account_id) const;
         /**
          * @brief Get the total number of accounts registered with the blockchain
          * @ingroup DatabaseAPI
          */
         uint64_t get_account_count()const;

         ////////////
         // Assets //
         ////////////

         /**
          * @brief Get a list of assets by ID
          * @param asset_ids IDs of the assets to retrieve
          * @return The assets corresponding to the provided IDs
          * @ingroup DatabaseAPI
          *
          * This function has semantics identical to @ref get_objects
          */
         vector<optional<asset_object>> get_assets(const vector<asset_id_type>& asset_ids)const;

         /**
          * @brief Get assets alphabetically by symbol name
          * @param lower_bound_symbol Lower bound of symbol names to retrieve
          * @param limit Maximum number of assets to fetch (must not exceed 100)
          * @return The assets found
          * @ingroup DatabaseAPI
          */
         vector<asset_object> list_assets(const string& lower_bound_symbol, uint32_t limit)const;

         /**
          * @brief Get a list of assets by symbol
          * @param symbols_or_ids Symbols or stringified IDs of the assets to retrieve
          * @return The assets corresponding to the provided symbols or IDs
          * @ingroup DatabaseAPI
          *
          * This function has semantics identical to @ref get_objects
          */
         vector<optional<asset_object>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;

         share_type get_new_asset_per_block() const;
         share_type get_asset_per_block_by_block_num(uint32_t block_num) const;


         ///////////////
         // Miners //
         ///////////////

         /**
          * @brief Get a list of miners by ID
          * @param miner_ids IDs of the miners to retrieve
          * @return The miners corresponding to the provided IDs
          * @ingroup DatabaseAPI
          *
          * This function has semantics identical to @ref get_objects
          */
         vector<optional<miner_object>> get_miners(const vector<miner_id_type>& miner_ids)const;

         /**
          * @brief Get the miner owned by a given account
          * @param account The ID of the account whose miner should be retrieved
          * @return The miner object, or null if the account does not have a miner
          * @ingroup DatabaseAPI
          */
         fc::optional<miner_object> get_miner_by_account(account_id_type account)const;

         /**
          * @brief Get names and IDs for registered miners
          * @param lower_bound_name Lower bound of the first name to return
          * @param limit Maximum number of results to return -- must not exceed 1000
          * @return Map of miner names to corresponding IDs
          * @ingroup DatabaseAPI
          */
         map<string, miner_object_ex> lookup_miner_accounts(const string& lower_bound_name, uint32_t limit)const;

         /**
          * @brief Get the total number of miners registered with the blockchain
          * @ingroup DatabaseAPI
          */
         uint64_t get_miner_count()const;
         vector<account_balance_object> get_miner_voters(const string &miner_id)const;

         ///////////
         // Votes //
         ///////////

         /**
          *  @brief Given a set of votes, return the objects they are voting for.
          *  @ingroup DatabaseAPI
          *  @param votes Set of votes
          *  The results will be in the same order as the votes.  Null will be returned for
          *  any vote ids that are not found.
          */
         vector<variant> lookup_vote_ids( const vector<vote_id_type>& votes )const;

         ////////////////////////////
         // Authority / validation //
         ////////////////////////////

         /**
          * @brief Get a hexdump of the serialized binary form of a transaction
          * @param trx
          * @return
          * @ingroup DatabaseAPI
          */
         std::string get_transaction_hex(const signed_transaction& trx)const;

         /**
          *  @brief This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
          *  and return the minimal subset of public keys that should add signatures to the transaction.
          *  @param trx
          *  @param available_keys Set of available public keys
          *  @ingroup DatabaseAPI
          */
         set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;

         /**
          *  @brief This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
          *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
          *  to get the minimum subset.
          *  @param trx
          *  @ingroup DatabaseAPI
          */
         set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;

         /**
          * @return true if the @ref trx has all of the required signatures, otherwise throws an exception
          * @ingroup DatabaseAPI
          * @param trx
          */
         bool           verify_authority( const signed_transaction& trx )const;

         /**
          * @return true if the signers have enough authority to authorize an account
          * @ingroup DatabaseAPI
          * @param name_or_id The name or id of the account
          * @param signers Set of public keys
          */
         bool           verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;

         /**
          *  @brief Validates a transaction against the current state without broadcasting it on the network.
          *  @param trx
          *  @ingroup DatabaseAPI
          */
         processed_transaction validate_transaction( const signed_transaction& trx )const;

         /**
          *  @brief For each operation calculate the required fee in the specified asset type. If the asset type does
          *  not have a valid core_exchange_rate
          *  @param ops The set of operations
          *  @param id  The asset ID
          *  @ingroup DatabaseAPI
          */
         vector< fc::variant > get_required_fees( const vector<operation>& ops, asset_id_type id )const;

         ///////////////////////////
         // Proposed transactions //
         ///////////////////////////

         /**
          * @brief Get the set of proposed transactions relevant to the specified account id
          * @param id The account ID
          * @return Set of proposed transactions
          * @ingroup DatabaseAPI
          */
         vector<proposal_object> get_proposed_transactions( account_id_type id )const;

         //////////////////////
         // Blinded balances //
         //////////////////////

         /**
          *  @return the set of blinded balance objects by commitment ID
          */
         vector<blinded_balance_object> get_blinded_balances( const flat_set<commitment_type>& commitments )const;
         vector<confidential_tx_object> get_confidential_transactions(const string &a, const string &B , bool unspent)const;

         ////////////
         // CYVA //
         ////////////


         /**
          * @brief Return current core asset supply
          * @return current supply
          * @ingroup DatabaseAPI
          */
         real_supply get_real_supply()const;

         miner_reward_input get_time_to_maint_by_block_time(fc::time_point_sec block_time) const;

      private:
         std::shared_ptr< database_api_impl > my;
      };

   } }

FC_REFLECT( graphene::app::order, (price)(quote)(base) );
FC_REFLECT( graphene::app::order_book, (base)(quote)(bids)(asks) );
FC_REFLECT( graphene::app::market_ticker, (base)(quote)(latest)(lowest_ask)(highest_bid)(percent_change)(base_volume)(quote_volume) );
FC_REFLECT( graphene::app::market_volume, (base)(quote)(base_volume)(quote_volume) );
FC_REFLECT( graphene::app::market_trade, (date)(price)(amount)(value) );

FC_API(graphene::app::database_api,
// Objects
       (get_objects)

          // Subscriptions
          (set_subscribe_callback)
          (set_pending_transaction_callback)
          (set_block_applied_callback)
          (cancel_all_subscriptions)

          // Blocks and transactions
          (get_block_header)
          (get_block)
          (get_blocks)
          (get_transaction)
          (get_transactions_by_id)
          (head_block_time)
          (get_head_block)
          (get_nearest_block)
          (get_recent_transaction_by_id)
          (get_new_asset_per_block)
          (get_asset_per_block_by_block_num)
          (get_time_to_maint_by_block_time)

          // Globals
          (get_chain_properties)
          (get_global_properties)
          (get_config)
          (get_chain_id)
          (get_dynamic_global_properties)

          // Keys
          (get_key_references)

          // Accounts
          (get_accounts)
          (get_full_accounts)
          (get_account_by_name)
          (get_account_references)
          (lookup_account_names)
          (lookup_accounts)
          (search_accounts)
          (get_account_count)
          (search_account_history)

          // Balances
          (get_account_balances)
          (get_named_account_balances)
          (get_vesting_balances)
          (get_vesting_balances_with_info)

          // Assets
          (get_assets)
          (list_assets)
          (lookup_asset_symbols)

          // Miners
          (get_miners)
          (get_miner_by_account)
          (lookup_miner_accounts)
          (get_miner_count)
          (get_miner_voters)

          // Votes
          (lookup_vote_ids)

          // Authority / validation
          (get_transaction_hex)
          (get_required_signatures)
          (get_potential_signatures)
          (verify_authority)
          (verify_account_authority)
          (validate_transaction)
          (get_required_fees)

          // Proposed transactions
          (get_proposed_transactions)

          // Blinded balances
          (get_blinded_balances)

          // CYVA
          (get_real_supply)
          (get_confidential_transactions)
)
