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

#include <graphene/app/api.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <cyva/encrypt/encryptionutils.hpp>
#include <graphene/chain/transaction_detail_object.hpp>


using namespace graphene::app;
using namespace graphene::chain;
using namespace graphene::utilities;
using namespace std;

using DInteger = cyva::encrypt::DInteger;

namespace fc
{
   void to_variant(const account_multi_index_type& accts, variant& vo);
   void from_variant(const variant &var, account_multi_index_type &vo);
}

namespace graphene { namespace wallet {

      typedef uint16_t transaction_handle_type;

/**
 * This class takes a variant and turns it into an object
 * of the given type, with the new operator.
 */

      object* create_object( const variant& v );

      struct plain_keys
      {
         map<public_key_type, string>  keys;
         fc::sha512                    checksum;
      };

      struct brain_key_info
      {
         string brain_priv_key;
         string wif_priv_key;
         public_key_type pub_key;
      };

      /* BLIND TRANSFERS */
      /**
       *  Contains the confirmation receipt the sender must give the receiver and
       *  the meta data about the receipt that helps the sender identify which receipt is
       *  for the receiver and which is for the change address.
       */
      struct blind_confirmation
      {
         struct output
         {
            string                          label;
            public_key_type                 pub_key;
            stealth_confirmation::memo_data decrypted_memo;
            stealth_confirmation            confirmation;
            authority                       auth;
            string                          confirmation_receipt;
         };

         signed_transaction     trx;
         vector<output>         outputs;
      };

      struct blind_balance
      {
         asset                     amount;
         public_key_type           from; ///< the account this balance came from
         public_key_type           to; ///< the account this balance is logically associated with
         public_key_type           one_time_key; ///< used to derive the authority key and blinding factor
         fc::sha256                blinding_factor;
         fc::ecc::commitment_type  commitment;
         bool                      used = false;
      };

      struct blind_receipt
      {
         std::pair<public_key_type,fc::time_point>        from_date()const { return std::make_pair(from_key,date); }
         std::pair<public_key_type,fc::time_point>        to_date()const   { return std::make_pair(to_key,date);   }
         std::tuple<public_key_type,asset_id_type,bool>   to_asset_used()const   { return std::make_tuple(to_key,amount.asset_id,used);   }
         const commitment_type& commitment()const        { return data.commitment; }

         fc::time_point                  date;
         public_key_type                 from_key;
         string                          from_label;
         public_key_type                 to_key;
         string                          to_label;
         asset                           amount;
         string                          memo;
         authority                       control_authority;
         stealth_confirmation::memo_data data;
         bool                            used = false;
         stealth_confirmation            conf;
      };

      struct by_from;
      struct by_to;
      struct by_to_asset_used;
      struct by_commitment;

      typedef multi_index_container< blind_receipt,
         indexed_by<
            ordered_unique< tag<by_commitment>, const_mem_fun< blind_receipt, const commitment_type&, &blind_receipt::commitment > >,
            ordered_unique< tag<by_to>, const_mem_fun< blind_receipt, std::pair<public_key_type,fc::time_point>, &blind_receipt::to_date > >,
            ordered_non_unique< tag<by_to_asset_used>, const_mem_fun< blind_receipt, std::tuple<public_key_type,asset_id_type,bool>, &blind_receipt::to_asset_used > >,
            ordered_unique< tag<by_from>, const_mem_fun< blind_receipt, std::pair<public_key_type,fc::time_point>, &blind_receipt::from_date > >
         >
      > blind_receipt_index_type;


      struct key_label
      {
         string          label;
         public_key_type key;
      };


      struct by_label;
      struct by_key;
      typedef multi_index_container<
         key_label,
         indexed_by<
            ordered_unique< tag<by_label>, member< key_label, string, &key_label::label > >,
            ordered_unique< tag<by_key>, member< key_label, public_key_type, &key_label::key > >
         >
      > key_label_index_type;



      struct operation_detail {
         string                   memo;
         string                   description;
         operation_history_object op;
      };

      struct wallet_data
      {
         /** Chain ID this wallet is used with */
         chain_id_type chain_id;
         account_multi_index_type my_accounts;
         /// @return IDs of all accounts in @ref my_accounts
         vector<object_id_type> my_account_ids()const
         {
            vector<object_id_type> ids;
            ids.reserve(my_accounts.size());
            std::transform(my_accounts.begin(), my_accounts.end(), std::back_inserter(ids),
                           [](const account_object& ao) { return ao.id; });
            return ids;
         }
         /// Add acct to @ref my_accounts, or update it if it is already in @ref my_accounts
         /// @return true if the account was newly inserted; false if it was only updated
         bool update_account(const account_object& acct)
         {
            auto& idx = my_accounts.get<by_id>();
            auto itr = idx.find(acct.get_id());
            if( itr != idx.end() )
            {
               idx.replace(itr, acct);
               return false;
            } else {
               idx.insert(acct);
               return true;
            }
         }

         /** encrypted keys */
         vector<char>              cipher_keys;

         /** map an account to a set of extra keys that have been imported for that account */
         map<account_id_type, set<public_key_type> >  extra_keys;

         // map of account_name -> base58_private_key for
         //    incomplete account regs
         map<string, vector<string> > pending_account_registrations;

         map<string, string> pending_miner_registrations;

         key_label_index_type      labeled_keys;
         blind_receipt_index_type  blind_receipts;

         string                    ws_server = "ws://localhost:8090";
         string                    ws_user;
         string                    ws_password;
         string                    libtorrent_config_path;
      };

      struct exported_account_keys
      {
         string account_name;
         vector<vector<char>> encrypted_private_keys;
         vector<public_key_type> public_keys;
      };

      struct exported_keys
      {
         fc::sha512 password_checksum;
         vector<exported_account_keys> account_keys;
      };

      struct approval_delta
      {
         vector<string> active_approvals_to_add;
         vector<string> active_approvals_to_remove;
         vector<string> owner_approvals_to_add;
         vector<string> owner_approvals_to_remove;
         vector<string> key_approvals_to_add;
         vector<string> key_approvals_to_remove;
      };

      class transaction_detail_object_ex : public transaction_detail_object
      {
      public:
          transaction_detail_object_ex(transaction_detail_object const& other)
              : transaction_detail_object(other) {}
          std::string m_from_account_public_key;
          std::string m_to_account_public_key;
      };

      struct signed_block_with_info : public signed_block
      {
         signed_block_with_info( const signed_block& block );
         signed_block_with_info( const signed_block_with_info& block ) = default;

         block_id_type block_id;
         public_key_type signing_key;
         vector< transaction_id_type > transaction_ids;
      };

      namespace detail {
         class wallet_api_impl;
      }
   
   
   
/**
 * This wallet assumes it is connected to the database server with a high-bandwidth, low-latency connection and
 * performs minimal caching. This API could be provided locally to be used by a web interface.
 *
 * @defgroup WalletCLI
 */
      class wallet_api : public fc::api_base<wallet_api>
      {
      public:
         wallet_api( const wallet_data& initial_data, fc::api<login_api> rapi );
         virtual ~wallet_api();

         /**
          * @brief Copy wallet file to a new file
          * @param destination_filename
          * @return true if the wallet is copied, false otherwise
          * @ingroup WalletCLI
          */
         bool copy_wallet_file( string destination_filename );

         /**
          * @brief Derive private key from given prefix and sequence
          * @param prefix_string
          * @param sequence_number
          * @return private_key Derived private key
          * @ingroup WalletCLI
          */
         fc::ecc::private_key derive_private_key(const std::string& prefix_string, int sequence_number) const;

         /**
          * @brief Lists all available commands
          * @return List of all available commands
          * @ingroup WalletCLI
          */
         variant                           info();

         /**
          * @brief Returns info such as client version, git version of graphene/fc, version of boost, openssl.
          * @returns compile time info and client and dependencies versions
          * @ingroup WalletCLI
          */
         variant_object                    about() const;

         /**
          * @brief Retrieve a full, signed block with info
          * @param num ID of the block
          * @return the referenced block with info, or null if no matching block was found
          * @ingroup WalletCLI
          */
         optional<signed_block_with_info>    get_block( uint32_t num ) const;

         /**
          * @brief Retrieve an array of full, signed blocks
          * @param block first block Height of blocks to be returned
          * @param skip_empty specify if don't need empty blocks
          * @return an array of referenced blocks
          * @ingroup DatabaseAPI
          */
         vector<pair<uint32_t, signed_block_with_info>> get_blocks_starting_from(uint32_t block_num, uint32_t count, bool skip_empty)const;

         /**
          * @brief Get an account's balances in various assets
          * @param id ID of the account to get balances for
          * @param assets IDs of the assets to get balances of; if empty, get all assets account has a balance in
          * @return Balances of the account
          * @ingroup DatabaseAPI
          */
         vector<asset> get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;

         /**
          * @brief Returns the number of accounts registered on the blockchain
          * @returns the number of registered accounts
          * @ingroup WalletCLI
          */
         uint64_t                          get_account_count()const;

         /**
          * @brief Lists all accounts controlled by this wallet.
          * This returns a list of the full account objects for all accounts whose private keys
          * we possess.
          * @returns a list of account objects
          * @ingroup WalletCLI
          */
         vector<account_object>            list_my_accounts();

         /**
          * @brief Lists all accounts registered in the blockchain.
          * This returns a list of all account names and their account ids, sorted by account name.
          *
          * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all accounts,
          * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
          * the last account name returned as the \c lowerbound for the next \c list_accounts() call.
          *
          * @param lowerbound the name of the first account to return.  If the named account does not exist,
          *                   the list will start at the account that comes after \c lowerbound
          * @param limit the maximum number of accounts to return (max: 1000)
          * @returns a list of accounts mapping account names to account ids
          * @ingroup WalletCLI
          */
         map<string,account_id_type>       list_accounts(const string& lowerbound, uint32_t limit);

         /**
          * @brief Get names and IDs for registered accounts that match search term
          * @param term will try to partially match account name or id
          * @param limit Maximum number of results to return -- must not exceed 1000
          * @param order Sort data by field
          * @param id object_id to start searching from
          * @return Map of account names to corresponding IDs
          * @ingroup WalletCLI
          */
         vector<account_object>       search_accounts(const string& term, const string& order, const string& id, uint32_t limit);

         /**
          * @brief update account details
          *
          * @param account_id the account_id such as "1.2.10" or the account name
          * @param broadcast true to broadcast the transaction on the network
          */
         signed_transaction update_account_meta(string const& account_id, bool broadcast);

         /**
          * @brief List the balances of an account.
          * Each account can have multiple balances, one for each type of asset owned by that
          * account.  The returned list will only contain assets for which the account has a
          * nonzero balance
          * @param id the name or id of the account whose balances you want
          * @returns a list of the given account's balances
          * @ingroup WalletCLI
          */
         vector<asset>                     list_account_balances(const string& id);

         /**
          * @brief Lists all assets registered on the blockchain.
          *
          * To list all assets, pass the empty string \c "" for the lowerbound to start
          * at the beginning of the list, and iterate as necessary.
          *
          * @param lowerbound  the symbol of the first asset to include in the list.
          * @param limit the maximum number of assets to return (max: 100)
          * @returns the list of asset objects, ordered by symbol
          * @ingroup WalletCLI
          */
         vector<asset_object>              list_assets(const string& lowerbound, uint32_t limit)const;

         /**
          * @brief Returns the operations on the named account.
          *
          * This returns a list of transaction detail object, which describe activity on the account.
          *
          * @param account_name the name or id of the account
          * @param order Sort data by field
          * @param id object_id to start searching from
          * @param limit the number of entries to return (starting from the most recent) (max 100)
          * @returns a list of \c transaction_detail_object
          * @ingroup WalletCLI
          */
         vector<class transaction_detail_object_ex> search_account_history(string const& account_name,
                                                                           string const& order,
                                                                           string const& id,
                                                                           int limit) const;


         /** Returns the most recent operations on the named account.
          *
          * This returns a list of operation history objects, which describe activity on the account.
          *
          * @note this API doesn't give a way to retrieve more than the most recent 100 transactions,
          *       you can interface directly with the blockchain to get more history
          * @param name the name or id of the account
          * @param limit the number of entries to return (starting from the most recent) (max 100)
          * @returns a list of \c operation_history_objects
          */
         vector<operation_detail>  get_account_history(string name, int limit)const;


         /**
          * @brief Returns the block chain's slowly-changing settings.
          * This object contains all of the properties of the blockchain that are fixed
          * or that change only once per maintenance interval (daily) such as the
          * current list of miners, block interval, etc.
          * @see \c get_dynamic_global_properties() for frequently changing properties
          * @returns the global properties
          * @ingroup WalletCLI
          */
         global_property_object            get_global_properties() const;

         /**
          * @brief Returns the block chain's rapidly-changing properties.
          * The returned object contains information that changes every block interval
          * such as the head block number, the next miner, etc.
          * @see \c get_global_properties() for less-frequently changing properties
          * @returns the dynamic global properties
          * @ingroup WalletCLI
          */
         dynamic_global_property_object    get_dynamic_global_properties() const;

         /**
          * @brief Returns information about the given account.
          *
          * @param account_name_or_id the name or id of the account to provide information about
          * @returns the public account data stored in the blockchain
          * @ingroup WalletCLI
          */
         account_object                    get_account(string account_name_or_id) const;

         /**
          * @brief Returns information about the given asset.
          * @param asset_name_or_id the symbol or id of the asset in question
          * @returns the information about the asset stored in the block chain
          * @ingroup WalletCLI
          */
         asset_object                      get_asset(string asset_name_or_id) const;

         /**
          * @brief Lookup the id of a named account.
          * @param account_name_or_id the name of the account to look up
          * @returns the id of the named account
          * @ingroup WalletCLI
          */
         account_id_type                   get_account_id(string account_name_or_id) const;

         /**
          * @brief Lookup the id of a named asset.
          * @param asset_name_or_id the symbol of an asset to look up
          * @returns the id of the given asset
          * @ingroup WalletCLI
          */
         asset_id_type                     get_asset_id(string asset_name_or_id) const;

         /**
          * @brief Returns the blockchain object corresponding to the given id.
          *
          * This generic function can be used to retrieve any object from the blockchain
          * that is assigned an ID.  Certain types of objects have specialized convenience
          * functions to return their objects -- e.g., assets have \c get_asset(), accounts
          * have \c get_account(), but this function will work for any object.
          *
          * @param id the id of the object to return
          * @returns the requested object
          * @ingroup WalletCLI
          */
         variant                           get_object(object_id_type id) const;

         /** @brief Returns the current wallet filename.
          *
          * This is the filename that will be used when automatically saving the wallet.
          *
          * @see set_wallet_filename()
          * @return the wallet filename
          * @ingroup WalletCLI
          */
         string                            get_wallet_filename() const;

         /**
          * @brief Get the WIF private key corresponding to a public key.  The
          * private key must already be in the wallet.
          * @param pubkey Public key
          * @return WIF private key corresponding to a public key
          * @ingroup WalletCLI
          */
         string                            get_private_key( public_key_type pubkey )const;

         /**
          * @brief Checks whether the wallet has just been created and has not yet had a password set.
          *
          * Calling \c set_password will transition the wallet to the locked state.
          * @return true if the wallet is new
          * @ingroup Wallet Management
          * @ingroup WalletCLI
          */
         bool    is_new()const;

         /**
          * @brief Checks whether the wallet is locked (is unable to use its private keys).
          *
          * This state can be changed by calling \c lock() or \c unlock().
          * @return true if the wallet is locked
          * @ingroup Wallet Management
          * @ingroup WalletCLI
          */
         bool    is_locked()const;

         /**
          * @brief Locks the wallet immediately.
          * @ingroup Wallet Management
          * @ingroup WalletCLI
          */
         void    lock();

         /**
          * @brief Unlocks the wallet.
          *
          * The wallet remain unlocked until the \c lock is called
          * or the program exits.
          * @param password the password previously set with \c set_password()
          * @ingroup Wallet Management
          * @ingroup WalletCLI
          */
         void    unlock(string password);

         /**
          * @brief Sets a new password on the wallet.
          *
          * The wallet must be either 'new' or 'unlocked' to
          * execute this command.
          * @param password
          * @ingroup Wallet Management
          * @ingroup WalletCLI
          */
         void    set_password(string password);

         /**
          * @brief Lists all miners registered in the blockchain.
          * This returns a list of all account names that own miners, and the associated miner id,
          * sorted by name.  This lists miners whether they are currently voted in or not.
          *
          * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all miners,
          * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
          * the last miner name returned as the \c lowerbound for the next \c list_miners() call.
          *
          * @param lowerbound the name of the first miner to return.  If the named miner does not exist,
          *                   the list will start at the miner that comes after \c lowerbound
          * @param limit the maximum number of miners to return (max: 1000)
          * @returns a list of miners mapping miner names to miner ids
          * @ingroup WalletCLI
          */
         map<string, miner_object_ex> list_miners(const string& lowerbound, uint32_t limit);

         /**
          * s@brief Returns information about the given miner.
          * @param owner_account the name or id of the miner account owner, or the id of the miner
          * @returns the information about the miner stored in the block chain
          * @ingroup WalletCLI
          */
         miner_object get_miner(string owner_account);

         /**
           * @brief Creates a miner object owned by the given account.
           *
           * An account can have at most one miner object.
           *
           * @param owner_account the name or id of the account which is creating the miner
           * @param url a URL to include in the miner record in the blockchain.  Clients may
           *            display this when showing a list of miners.  May be blank.
           * @param broadcast true to broadcast the transaction on the network
           * @returns the signed transaction registering a miner
           * @ingroup WalletCLI
           */
         signed_transaction create_miner(string owner_account,
                                           string url,
                                           bool broadcast = false);

         /**
          * @brief Update a miner object owned by the given account.
          *
          * @param miner_name The name of the miner's owner account.  Also accepts the ID of the owner account or the ID of the miner.
          * @param url Same as for create_miner.  The empty string makes it remain the same.
          * @param block_signing_key The new block signing public key.  The empty string makes it remain the same.
          * @param broadcast true if you wish to broadcast the transaction.
          * @ingroup WalletCLI
          */
         signed_transaction update_miner(string miner_name,
                                           string url,
                                           string block_signing_key,
                                           bool broadcast = false);

         /**
          * @brief Get information about a vesting balance object.
          *
          * @param account_name An account name, account ID, or vesting balance object ID.
          * @ingroup WalletCLI
          */
         vector< vesting_balance_object_with_info > get_vesting_balances( string account_name );

         /**
          * @brief Withdraw a vesting balance.
          *
          * @param miner_name The account name of the miner, also accepts account ID or vesting balance ID type.
          * @param amount The amount to withdraw.
          * @param asset_symbol The symbol of the asset to withdraw.
          * @param broadcast true if you wish to broadcast the transaction
          * @ingroup WalletCLI
          */
         signed_transaction withdraw_vesting(
            string miner_name,
            string amount,
            string asset_symbol,
            bool broadcast = false);

         /**
          * @brief Vote for a given miner.
          *
          * An account can publish a list of all miners they approve of.  This
          * command allows you to add or remove miners from this list.
          * Each account's vote is weighted according to the number of shares of the
          * core asset owned by that account at the time the votes are tallied.
          *
          * @note you cannot vote against a miner, you can only vote for the miner
          *       or not vote for the miner.
          *
          * @param voting_account the name or id of the account who is voting with their shares
          * @param miner the name or id of the miner' owner account
          * @param approve true if you wish to vote in favor of that miner, false to
          *                remove your vote in favor of that miner
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed transaction changing your vote for the given miner
          * @ingroup WalletCLI
          */
         signed_transaction vote_for_miner(string voting_account,
                                             string miner,
                                             bool approve,
                                             bool broadcast = false);

         /**
          * @brief Set the voting proxy for an account.
          *
          * If a user does not wish to take an active part in voting, they can choose
          * to allow another account to vote their stake.
          *
          * Setting a vote proxy does not remove your previous votes from the blockchain,
          * they remain there but are ignored.  If you later null out your vote proxy,
          * your previous votes will take effect again.
          *
          * This setting can be changed at any time.
          *
          * @param account_to_modify the name or id of the account to update
          * @param voting_account the name or id of an account authorized to vote account_to_modify's shares,
          *                       or null to vote your own shares
          *
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed transaction changing your vote proxy settings
          * @ingroup WalletCLI
          */
         signed_transaction set_voting_proxy(string account_to_modify,
                                             optional<string> voting_account,
                                             bool broadcast = false);

         /**
          * @brief Set your vote for the number of miners in the system.
          *
          * Each account can voice their opinion on how many
          * miners there should be in the active miner list.  These
          * are independent of each other.  You must vote your approval of at least as many
          * miners as you claim there should be (you can't say that there should
          * be 20 miners but only vote for 10).
          *
          * There are maximum values for each set in the blockchain parameters (currently
          * defaulting to 1001).
          *
          * This setting can be changed at any time.  If your account has a voting proxy
          * set, your preferences will be ignored.
          *
          * @param account_to_modify the name or id of the account to update
          * @param desired_number_of_miners
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed transaction changing your vote proxy settings
          * @ingroup WalletCLI
          */
         signed_transaction set_desired_miner_count(string account_to_modify,
                                                      uint16_t desired_number_of_miners,
                                                      bool broadcast = false);

         /**
          * @brief Dumps all private keys owned by the wallet.
          *
          * The keys are printed in WIF format.  You can import these keys into another wallet
          * using \c import_key()
          * @returns a map containing the private keys, indexed by their public key
          * @ingroup WalletCLI
          */
         map<public_key_type, string> dump_private_keys();

         /**
          * @brief Returns a list of all commands supported by the wallet API.
          *
          * This lists each command, along with its arguments and return types.
          * For more detailed help on a single command, use \c get_help()
          *
          * @returns a multi-line string suitable for displaying on a terminal
          * @ingroup WalletCLI
          */
         string  help()const;

         /**
          * @brief Returns detailed help on a single API command.
          * @param method the name of the API command you want help with
          * @returns a multi-line string suitable for displaying on a terminal
          * @ingroup WalletCLI
          */
         string  gethelp(const string& method)const;

         /**
          * @brief Loads a specified Graphene wallet.
          *
          * The current wallet is closed before the new wallet is loaded.
          *
          * @warning This does not change the filename that will be used for future
          * wallet writes, so this may cause you to overwrite your original
          * wallet unless you also call \c set_wallet_filename()
          *
          * @param wallet_filename the filename of the wallet JSON file to load.
          *                        If \c wallet_filename is empty, it reloads the
          *                        existing wallet file
          * @returns true if the specified wallet is loaded
          * @ingroup WalletCLI
          */
         bool    load_wallet_file(string wallet_filename = "");

         /**
          * @brief Saves the current wallet to the given filename.
          *
          * @warning This does not change the wallet filename that will be used for future
          * writes, so think of this function as 'Save a Copy As...' instead of
          * 'Save As...'.  Use \c set_wallet_filename() to make the filename
          * persist.
          * @param wallet_filename the filename of the new wallet JSON file to create
          *                        or overwrite.  If \c wallet_filename is empty,
          *                        save to the current filename.
          * @ingroup WalletCLI
          */
         void    save_wallet_file(string wallet_filename = "");

         /**
          * @brief Sets the wallet filename used for future writes.
          *
          * This does not trigger a save, it only changes the default filename
          * that will be used the next time a save is triggered.
          *
          * @param wallet_filename the new filename to use for future saves
          * @ingroup WalletCLI
          */
         void    set_wallet_filename(string wallet_filename);

         /**
          * @brief Suggests a safe brain key to use for creating your account.
          * \c create_account_with_brain_key() requires you to specify a 'brain key',
          * a long passphrase that provides enough entropy to generate cyrptographic
          * keys.  This function will suggest a suitably random string that should
          * be easy to write down (and, with effort, memorize).
          * @returns a suggested brain_key
          * @ingroup WalletCLI
          */
         brain_key_info suggest_brain_key()const;

         /**
          * @brief Calculates the private key and public key corresponding to any brain key
          * @param brain_key the brain key to be used for calculation
          * @returns the corresponding brain_key_info
          * @ingroup WalletCLI
          */
         brain_key_info get_brain_key_info(string const& brain_key) const;

         /**
          * @brief Converts a signed_transaction in JSON form to its binary representation.
          *
          * TODO: I don't see a broadcast_transaction() function, do we need one?
          *
          * @param tx the transaction to serialize
          * @returns the binary form of the transaction.  It will not be hex encoded,
          *          this returns a raw string that may have null characters embedded
          *          in it
          * @ingroup WalletCLI
          */
         string serialize_transaction(signed_transaction tx) const;

         /**
          * @brief Imports the private key for an existing account.
          *
          * The private key must match either an owner key or an active key for the
          * named account.
          *
          * @see dump_private_keys()
          *
          * @param account_name_or_id the account owning the key
          * @param wif_key the private key in WIF format
          * @returns true if the key was imported
          * @ingroup WalletCLI
          */
         bool import_key(string account_name_or_id, string wif_key);

         /**
          * @brief Imports accounts from the other wallet file
          * @param filename The filename of the wallet JSON file
          * @param password User's password to the wallet
          * @return mapped account names to boolean values indicating whether the account was successfully imported
          * @ingroup WalletCLI
          */
         map<string, bool> import_accounts( string filename, string password );

         /**
          * @brief Imports account keys from particular account from another wallet file to desired account located in wallet file currently used
          * @param filename The filename of the wallet JSON file
          * @param password User's password to the wallet
          * @param src_account_name Name of the source account
          * @param dest_account_name Name of the destination account
          * @return true if the keys were imported
          * @ingroup WalletCLI
          */
         bool import_account_keys( string filename, string password, string src_account_name, string dest_account_name );

         /**
          * @brief Transforms a brain key to reduce the chance of errors when re-entering the key from memory.
          *
          * This takes a user-supplied brain key and normalizes it into the form used
          * for generating private keys.  In particular, this upper-cases all ASCII characters
          * and collapses multiple spaces into one.
          * @param s the brain key as supplied by the user
          * @returns the brain key in its normalized form
          * @ingroup WalletCLI
          */
         string normalize_brain_key(string s) const;

         /**
          * @brief Creates a new account and registers it on the blockchain.
          *
          * @todo why no referrer_percent here?
          *
          * @see suggest_brain_key()
          *
          * @param brain_key the brain key used for generating the account's private keys
          * @param registrar_account the account which will pay the fee to register the user
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction registering the account
          * @ingroup WalletCLI
          */
         signed_transaction create_account_with_brain_key(string const& brain_key,
                                                          string const& registrar_account,
                                                          bool broadcast = false);
         /**
          * @brief Creates a new account and registers it on the blockchain, but does not import the key to wallet.
          *
          * @see suggest_brain_key()
          *
          * @param brain_key the brain key used for generating the account's private keys
          * @param registrar_account the account which will pay the fee to register the user
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction registering the account
          * @ingroup WalletCLI
          */
         signed_transaction create_account_with_brain_key_noimport(string const& brain_key,
                                                                   string const& registrar_account,
                                                                   bool broadcast = false);

         /**
          * @brief Creates a new account and registers it on the blockchain, but does not import the key to wallet.
          *
          * @see suggest_brain_key()
          *
          * @param pubkey the public key of the account, account name will be set the same
          * @param registrar_account the account which will pay the fee to register the user
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction registering the account
          * @ingroup WalletCLI
          */
         signed_transaction create_account_with_public_key(public_key_type const& pubkey,
                                                           string const& registrar_account,
                                                           bool broadcast = false);

         /** @brief Transfer an amount from one account to another.
          * @param from the name or id of the account sending the funds
          * @param to the name or id of the account receiving the funds
          * @param amount the amount to send (in nominal units -- to send half of a BTS, specify 0.5)
          * @param asset_symbol the symbol or id of the asset to send
          * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
          *             transaction and readable for the receiver.  There is no length limit
          *             other than the limit imposed by maximum transaction size, but transaction
          *             increase with transaction size
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction transferring funds
          * @ingroup WalletCLI
          */
         signed_transaction transfer(string from,
                                     string to,
                                     string amount,
                                     string asset_symbol,
                                     string memo,
                                     bool broadcast = false);

         /**
          *  @brief This method works just like transfer, except it always broadcasts and
          *  returns the transaction ID along with the signed transaction.
          *  @param from the name or id of the account sending the funds
          *  @param to the name or id of the account receiving the funds
          *  @param amount the amount to send (in nominal units -- to send half of a BTS, specify 0.5)
          *  @param asset_symbol the symbol or id of the asset to send
          *  @param memo a memo to attach to the transaction.  The memo will be encrypted in the
          *             transaction and readable for the receiver.  There is no length limit
          *             other than the limit imposed by maximum transaction size, but transaction
          *             increase with transaction size
          *  @ingroup WalletCLI
          */
         std::pair<transaction_id_type,signed_transaction> transfer2(string from,
                                                                string to,
                                                                string amount,
                                                                string asset_symbol,
                                                                string memo ) {
            auto trx = transfer( from, to, amount, asset_symbol, memo, true );
            return std::make_pair(trx.id(),trx);
         }

         /** @brief Set the block number after which transfers will be frozen
          * @param announcer the user responsible
          * @param block_num the block number
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction transferring funds
          * @ingroup WalletCLI
          */
         signed_transaction set_transfer_freeze_block(string const& announcer,
                                                      uint64_t block_num,
                                                      bool broadcast = false);
         
         /**
          * @brief This method is used to convert a JSON transaction to its transaction ID.
          * @param trx Signed transaction
          * @return
          * @ingroup WalletCLI
          */
         transaction_id_type get_transaction_id( const signed_transaction& trx )const { return trx.id(); }


         /**
          * @brief Signs a transaction.
          *
          * Given a fully-formed transaction that is only lacking signatures, this signs
          * the transaction with the necessary keys and optionally broadcasts the transaction
          * @param tx the unsigned transaction
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed version of the transaction
          * @ingroup WalletCLI
          */
         signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false);

         /**
          * @brief Returns an uninitialized object representing a given blockchain operation.
          *
          * This returns a default-initialized object of the given type; it can be used
          * during early development of the wallet when we don't yet have custom commands for
          * creating all of the operations the blockchain supports.
          *
          * Any operation the blockchain supports can be created using the transaction builder's
          * \c add_operation_to_builder_transaction() , but to do that from the CLI you need to
          * know what the JSON form of the operation looks like.  This will give you a template
          * you can fill in.  It's better than nothing.
          *
          * @param operation_type the type of operation to return, must be one of the
          *                       operations defined in `graphene/chain/operations.hpp`
          *                       (e.g., "global_parameters_update_operation")
          * @return a default-constructed operation of the given type
          * @ingroup WalletCLI
          */
         operation get_prototype_operation(string operation_type);

         /**
          * @brief Propose a fee change.
          *
          * @param proposing_account The account paying the fee to propose the tx
          * @param expiration_time Timestamp specifying when the proposal will either take effect or expire.
          * @param changed_values Map of operation type to new fee.  Operations may be specified by name or ID.
          *    The "scale" key changes the scale.  All other operations will maintain current values.
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed version of the transaction
          * @ingroup WalletCLI
          */
          signed_transaction propose_fee_change(const string         &proposing_account,
                                                const variant_object &changed_values,
                                                bool                  broadcast = false);

         /**
          * @brief Approve or disapprove a proposal.
          *
          * @param fee_paying_account The account paying the fee for the op.
          * @param proposal_id The proposal to modify.
          * @param delta Members contain approvals to create or remove.  In JSON you can leave empty members undefined.
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed version of the transaction
          * @ingroup WalletCLI
          */
         signed_transaction approve_proposal(
            const string& fee_paying_account,
            const string& proposal_id,
            const approval_delta& delta,
            bool broadcast /* = false */
         );

         /**
          *
          * @param src_filename
          * @param count
          * @ingroup WalletCLI
          */
         void dbg_push_blocks( std::string src_filename, uint32_t count );

         /**
          *
          * @param debug_wif_key
          * @param count
          * @ingroup WalletCLI
          */
         void dbg_generate_blocks( std::string debug_wif_key, uint32_t count );

         /**
          *
          * @param filename
          * @ingroup WalletCLI
          */
         void dbg_stream_json_objects( const std::string& filename );

         /**
          *
          * @param update
          * @ingroup WalletCLI
          */
         void dbg_update_object( fc::variant_object update );

         /**
          *
          * @param prefix
          * @param number_of_transactions
          * @ingroup WalletCLI
          */
         void flood_network(string prefix, uint32_t number_of_transactions);

         /**
          *
          * @param nodes
          * @ingroup WalletCLI
          */
         void network_add_nodes( const vector<string>& nodes );

         /**
          *
          * @ingroup WalletCLI
          */
         vector< variant > network_get_connected_peers();


         std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const;

         fc::signal<void(bool)> lock_changed;
         std::shared_ptr<detail::wallet_api_impl> my;
         void encrypt_keys();

         /**
          * Get current supply of the core asset
          * @ingroup WalletCLI
          */
         real_supply get_real_supply()const;

         /**
          * @brief Generates AES encryption key.
          * @return Random encryption key
          * @ingroup WalletCLI
          */
         DInteger generate_encryption_key() const;

         
         /**
          * @brief Sign a buffer
          * @param str_buffer The buffer to be signed
          * @param str_brainkey Derives the private key used for signature
          * @return The signed buffer
          * @ingroup WalletCLI
          */
         std::string sign_buffer(std::string const& str_buffer,
                                 std::string const& str_brainkey) const;

         /**
          * @brief Verify if the signature is valid
          * @param str_buffer The original buffer
          * @param str_publickey The public key used for verification
          * @param str_signature The signed buffer
          * @return true if valid, otherwise false
          * @ingroup WalletCLI
          */
         bool verify_signature(std::string const& str_buffer,
                               std::string const& str_publickey,
                               std::string const& str_signature) const;

         /**
          * @brief Query the last local block
          * @return the block time
          */
         fc::time_point_sec head_block_time() const;

         /**
          * @brief Query the last local block
          * @return the block
          */
         optional<signed_block> get_head_block()const;

         /**
          * @brief Query the block nearest to time
          * @return the block
          */
         optional<signed_block> get_nearest_block(const string& time_iso)const;


         /* BLIND TRANSFERS */
         /** These methods are used for stealth transfers */
         ///@{
         /**
          *  This method can be used to set the label for a public key
          *
          *  @note No two keys can have the same label.
          *
          *  @return true if the label was set, otherwise false
          */
         bool                        set_key_label( public_key_type, string label );
         string                      get_key_label( public_key_type )const;

         /**
          *  Generates a new blind account for the given brain key and assigns it the given label.
          */
         public_key_type             create_blind_account( string label, string brain_key  );

         /**
          * @return the total balance of all blinded commitments that can be claimed by the
          * given account key or label
          */
         vector<asset>                get_blind_balances( string key_or_label );
         /** @return all blind accounts */
         map<string,public_key_type> get_blind_accounts()const;
         /** @return all blind accounts for which this wallet has the private key */
         map<string,public_key_type> get_my_blind_accounts()const;
         /** @return the public key associated with the given label */
         public_key_type             get_public_key( string label )const;
         ///@}

         /**
          * @return all blind receipts to/form a particular account
          */
         vector<blind_receipt> blind_history( string key_or_account );

         /**
          *  Given a confirmation receipt, this method will parse it for a blinded balance and confirm
          *  that it exists in the blockchain.  If it exists then it will report the amount received and
          *  who sent it.
          *
          *  @param opt_from - if not empty and the sender is a unknown public key, then the unknown public key will be given the label opt_from
          *  @param confirmation_receipt - a base58 encoded stealth confirmation
          */
         blind_receipt receive_blind_transfer( string confirmation_receipt, string opt_from, string opt_memo );

         /**
          *  Transfers a public balance from @from to one or more blinded balances using a
          *  stealth transfer.
          */
         blind_confirmation transfer_to_blind( string from_account_id_or_name,
                                               string asset_symbol,
                                               /** map from key or label to amount */
                                               vector<pair<string, string>> to_amounts,
                                               bool broadcast = false );

         /**
          * Transfers funds from a set of blinded balances to a public account balance.
          */
         blind_confirmation transfer_from_blind(
                                               string from_blind_account_key_or_label,
                                               string to_account_id_or_name,
                                               string amount,
                                               string asset_symbol,
                                               bool broadcast = false );

         /**
          *  Used to transfer from one set of blinded balances to another
          */
         blind_confirmation blind_transfer( string from_key_or_label,
                                            string to_key_or_label,
                                            string amount,
                                            string symbol,
                                            bool broadcast = false );

         /**
          *  Used to transfer from one set of blinded balances to another
          */
         blind_confirmation blind_transfer_help( string from_key_or_label,
                                            string to_key_or_label,
                                            string amount,
                                            string symbol,
                                            bool broadcast = false,
                                            bool to_temp = false );

         /**
          *  Transfers a public balance from @from to one or more confidential balances using a
          *  confidential transfer.
          */
         signed_transaction transfer_to_confidential(string from_account_id_or_name,
                                                     string asset_symbol,
                                                     vector<
                                                         tuple<pair<string, string>,
                                                               string,
                                                               string
                                                               >
                                                         > beneficiaries);

         signed_transaction transfer_from_confidential(const string &A,
                                                       const string &B,
                                                       string        asset_symbol,
                                                       vector< tuple<
                                                           pair<string, string>,
                                                           string,
                                                           string
                                                           > > beneficiaries);

         vector<confidential_tx_object> get_confidential_transactions(const string &a, const string &B , bool unspent)const;
         asset get_confidential_balance(const string &A, const string &B, string asset_symbol) const;
      };


   } }

FC_REFLECT( graphene::wallet::key_label, (label)(key) )
FC_REFLECT( graphene::wallet::blind_balance, (amount)(from)(to)(one_time_key)(blinding_factor)(commitment)(used) )
FC_REFLECT( graphene::wallet::blind_confirmation::output, (label)(pub_key)(decrypted_memo)(confirmation)(auth)(confirmation_receipt) )
FC_REFLECT( graphene::wallet::blind_confirmation, (trx)(outputs) )




FC_REFLECT( graphene::wallet::plain_keys, (keys)(checksum) )
FC_REFLECT( graphene::wallet::wallet_data,
            (chain_id)
               (my_accounts)
               (cipher_keys)
               (extra_keys)
               (pending_account_registrations)(pending_miner_registrations)
               (ws_server)
               (ws_user)
               (ws_password)
               (labeled_keys)
               (blind_receipts)
)

FC_REFLECT( graphene::wallet::brain_key_info,
            (brain_priv_key)
               (wif_priv_key)
               (pub_key)
)

FC_REFLECT( graphene::wallet::exported_account_keys, (account_name)(encrypted_private_keys)(public_keys) )

FC_REFLECT( graphene::wallet::exported_keys, (password_checksum)(account_keys) )

FC_REFLECT( graphene::wallet::blind_receipt,
            (date)(from_key)(from_label)(to_key)(to_label)(amount)(memo)(control_authority)(data)(used)(conf) )

FC_REFLECT( graphene::wallet::approval_delta,
            (active_approvals_to_add)
               (active_approvals_to_remove)
               (owner_approvals_to_add)
               (owner_approvals_to_remove)
               (key_approvals_to_add)
               (key_approvals_to_remove)
)

FC_REFLECT_DERIVED( graphene::wallet::signed_block_with_info, (graphene::chain::signed_block),
                    (block_id)(signing_key)(transaction_ids) )


FC_REFLECT_DERIVED( graphene::wallet::transaction_detail_object_ex,
                    (graphene::chain::transaction_detail_object),(m_from_account_public_key)(m_to_account_public_key) )


FC_REFLECT( graphene::wallet::operation_detail,
            (memo)(description)(op) )

FC_API( graphene::wallet::wallet_api,
           (help)
           (gethelp)
           (info)
           (about)
           (is_new)
           (is_locked)
           (lock)(unlock)(set_password)
           (dump_private_keys)
           (list_my_accounts)
           (list_accounts)
           (search_accounts)
           (update_account_meta)
           (list_account_balances)
           (list_assets)
           (import_key)
           (import_accounts)
           (import_account_keys)
           (suggest_brain_key)
           (get_brain_key_info)
           (create_account_with_brain_key)
           (create_account_with_brain_key_noimport)
           (create_account_with_public_key)
           (transfer)
           (transfer2)
           (set_transfer_freeze_block)
           (get_transaction_id)
           (get_asset)
           (get_account)
           (get_account_id)
           (get_block)
           (get_blocks_starting_from)
           (get_account_balances)
           (get_account_count)
           (get_account_history)
           (search_account_history)
           (get_global_properties)
           (get_dynamic_global_properties)
           (get_object)
           (get_private_key)
           (load_wallet_file)
           (normalize_brain_key)
           (save_wallet_file)
           (serialize_transaction)
           (sign_transaction)
           (get_prototype_operation)
           (propose_fee_change)
           (approve_proposal)
           (list_miners)
           (get_miner)
           (create_miner)
           (update_miner)
           (get_vesting_balances)
           (withdraw_vesting)
           (vote_for_miner)
           (set_voting_proxy)
           (set_desired_miner_count)
           (dbg_push_blocks)
           (dbg_generate_blocks)
           (dbg_stream_json_objects)
           (dbg_update_object)
           (flood_network)
           (network_add_nodes)
           (network_get_connected_peers)
           (generate_encryption_key)
           (get_real_supply)
           (sign_buffer)
           (verify_signature)
           (head_block_time)
           (get_head_block)
           (get_nearest_block)
           (set_key_label)
           (get_key_label)
           (get_public_key)
           (get_blind_accounts)
           (get_my_blind_accounts)
           (get_blind_balances)
           (create_blind_account)
           (transfer_to_blind)
           (transfer_from_blind)
           (blind_transfer)
           (blind_history)
           (receive_blind_transfer)
           (transfer_from_confidential)
           (transfer_to_confidential)
           (get_confidential_transactions)
           (get_confidential_balance)
           )
