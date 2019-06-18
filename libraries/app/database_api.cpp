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

#include <functional>
 
#include <graphene/app/database_api.hpp>
#include <graphene/chain/get_config.hpp>
#include <graphene/utilities/key_conversion.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>

#include <fc/crypto/hex.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <cyva/encrypt/custodyutils.hpp>
#include <cyva/encrypt/encryptionutils.hpp>

#include <cctype>

#include <cfenv>
#include <iostream>

#define GET_REQUIRED_FEES_MAX_RECURSION 4

namespace {
      CryptoPP::AutoSeededRandomPool randomGenerator;
}

namespace {
   
   template <bool is_ascending>
   struct return_one {
      
      template <class T1, class T2>
      static auto choose(const T1& t1, const T2& t2) -> typename std::conditional<is_ascending, T1, T2 >::type {
         return t1;
      };
   };
   
   template <>
   struct return_one<false> {
      
      template <class T1, class T2>
      static auto choose(const T1& t1, const T2& t2) -> typename std::conditional<false, T1, T2 >::type {
         return t2;
      };
   };
}

namespace graphene { namespace app {

   class database_api_impl;
   
   
   class database_api_impl : public std::enable_shared_from_this<database_api_impl>
   {
   public:
      database_api_impl( graphene::chain::database& db );
      ~database_api_impl();
      
      // Objects
      fc::variants get_objects(const vector<object_id_type>& ids)const;
      
      // Subscriptions
      void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );
      void cancel_all_subscriptions();
      
      // Blocks and transactions
      optional<block_header> get_block_header(uint32_t block_num)const;
      optional<signed_block> get_block(uint32_t block_num)const;
      processed_transaction get_transaction( uint32_t block_num, uint32_t trx_in_block )const;
      fc::time_point_sec head_block_time()const;
      optional<signed_block> get_head_block()const;
      optional<signed_block> get_nearest_block(const string& time_iso_str) const;
      miner_reward_input get_time_to_maint_by_block_time(fc::time_point_sec block_time) const;
      
      // Globals
      chain_property_object get_chain_properties()const;
      global_property_object get_global_properties()const;
      fc::variant_object get_config()const;
      chain_id_type get_chain_id()const;
      dynamic_global_property_object get_dynamic_global_properties()const;
      
      // Keys
      vector<vector<account_id_type>> get_key_references( vector<public_key_type> key )const;
      
      // Accounts
      vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids)const;
      std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );
      optional<account_object> get_account_by_name( string name )const;
      vector<account_id_type> get_account_references( account_id_type account_id )const;
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;
      map<string,account_id_type> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;
      vector<account_object> search_accounts(const string& search_term, const string order, const object_id_type& id, uint32_t limit)const;
      vector<transaction_detail_object> search_account_history(std::string const& account_name,
                                                               std::string const& order,
                                                               std::string const& id,
                                                               int limit) const;
      uint64_t get_account_count()const;
      
      // Balances
      vector<asset> get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;
      vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const;
      vector<vesting_balance_object> get_vesting_balances( account_id_type account_id )const;
      
      // Assets
      vector<optional<asset_object>> get_assets(const vector<asset_id_type>& asset_ids)const;
      vector<asset_object>           list_assets(const string& lower_bound_symbol, uint32_t limit)const;
      vector<optional<asset_object>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;
      share_type get_new_asset_per_block() const;
      share_type get_asset_per_block_by_block_num(uint32_t block_num) const;

      // Miners
      vector<optional<miner_object>> get_miners(const vector<miner_id_type>& miner_ids)const;
      fc::optional<miner_object> get_miner_by_account(account_id_type account)const;
      map<string, miner_object> lookup_miner_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_miner_count()const;
      vector<account_balance_object> get_miner_voters(const string &miner_id)const;
      
      // Votes
      vector<variant> lookup_vote_ids( const vector<vote_id_type>& votes )const;
      
      // Authority / validation
      std::string get_transaction_hex(const signed_transaction& trx)const;
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      bool verify_authority( const signed_transaction& trx )const;
      bool verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;
      processed_transaction validate_transaction( const signed_transaction& trx )const;
      vector< fc::variant > get_required_fees( const vector<operation>& ops, asset_id_type id )const;
      
      // Proposed transactions
      vector<proposal_object> get_proposed_transactions( account_id_type id )const;
      
      // Blinded balances
      vector<blinded_balance_object> get_blinded_balances( const flat_set<commitment_type>& commitments )const;
      vector<confidential_tx_object> get_confidential_transactions(const fc::ecc::private_key &a, const fc::ecc::public_key &B, bool unspent)const;
      
      // CYVA

      //private:
      template<typename T>
      void subscribe_to_item( const T& i )const
      {
         auto vec = fc::raw::pack(i);
         if( !_subscribe_callback )
            return;
         
         if( !is_subscribed_to_item(i) )
         {
            idump((i));
            _subscribe_filter.insert( vec.data(), vec.size() );//(vecconst char*)&i, sizeof(i) );
         }
      }
      
      template<typename T>
      bool is_subscribed_to_item( const T& i )const
      {
         if( !_subscribe_callback )
            return false;
         return true;
         return _subscribe_filter.contains( i );
      }
      
      void broadcast_updates( const vector<variant>& updates );
      
      /** called every time a block is applied to report the objects that were changed */
      void on_objects_changed(const vector<object_id_type>& ids);
      void on_objects_removed(const vector<const object*>& objs);
      void on_applied_block();
      
      mutable fc::bloom_filter                               _subscribe_filter;
      std::function<void(const fc::variant&)> _subscribe_callback;
      std::function<void(const fc::variant&)> _pending_trx_callback;
      std::function<void(const fc::variant&)> _block_applied_callback;
      
      boost::signals2::scoped_connection                                                                                           _change_connection;
      boost::signals2::scoped_connection                                                                                           _removed_connection;
      boost::signals2::scoped_connection                                                                                           _applied_block_connection;
      boost::signals2::scoped_connection                                                                                           _pending_trx_connection;
      graphene::chain::database&                                                                                                   _db;
   };
   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Constructors                                                     //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   database_api::database_api( graphene::chain::database& db )
   : my( new database_api_impl( db ) ) {}
   
   database_api::~database_api() {}
   
   database_api_impl::database_api_impl( graphene::chain::database& db ):_db(db)
   {
      wlog("creating database api ${x}", ("x",int64_t(this)) );
      _change_connection = _db.changed_objects.connect([this](const vector<object_id_type>& ids) {
         on_objects_changed(ids);
      });
      _removed_connection = _db.removed_objects.connect([this](const vector<const object*>& objs) {
         on_objects_removed(objs);
      });
      _applied_block_connection = _db.applied_block.connect([this](const signed_block&){ on_applied_block(); });
      
      _pending_trx_connection = _db.on_pending_transaction.connect([this](const signed_transaction& trx ){
         if( _pending_trx_callback ) _pending_trx_callback( fc::variant(trx) );
      });
   }
   
   database_api_impl::~database_api_impl()
   {
      elog("freeing database api ${x}", ("x",int64_t(this)) );
   }
   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Objects                                                          //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   fc::variants database_api::get_objects(const vector<object_id_type>& ids)const
   {
      return my->get_objects( ids );
   }
   
   fc::variants database_api_impl::get_objects(const vector<object_id_type>& ids)const
   {
      if( _subscribe_callback )  {
         for( auto id : ids )
         {
            if( id.type() == operation_history_object_type && id.space() == protocol_ids ) continue;
            if( id.type() == impl_account_transaction_history_object_type && id.space() == implementation_ids ) continue;

            this->subscribe_to_item( id );
         }
      }
      else
      {
         elog( "getObjects without subscribe callback??" );
      }
      
      fc::variants result;
      result.reserve(ids.size());
      
      std::transform(ids.begin(), ids.end(), std::back_inserter(result),
                     [this](object_id_type id) -> fc::variant {
                        if(auto obj = _db.find_object(id))
                           return obj->to_variant();
                        return {};
                     });
      
      return result;
   }
   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Subscriptions                                                    //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   void database_api::set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter )
   {
      my->set_subscribe_callback( cb, clear_filter );
   }
   
   void database_api_impl::set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter )
   {
      edump((clear_filter));
      _subscribe_callback = cb;
      if( clear_filter || !cb )
      {
         static fc::bloom_parameters param;
         param.projected_element_count    = 10000;
         param.false_positive_probability = 1.0/10000;
         param.maximum_size = 1024*8*8*2;
         param.compute_optimal_parameters();
         _subscribe_filter = fc::bloom_filter(param);
      }
   }

   void database_api::set_pending_transaction_callback( std::function<void(const variant&)> cb )
   {
      my->set_pending_transaction_callback( cb );
   }
   
   void database_api_impl::set_pending_transaction_callback( std::function<void(const variant&)> cb )
   {
      _pending_trx_callback = cb;
   }
   
   void database_api::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
   {
      my->set_block_applied_callback( cb );
   }
   
   void database_api_impl::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
   {
      _block_applied_callback = cb;
   }
   
   void database_api::cancel_all_subscriptions()
   {
      my->cancel_all_subscriptions();
   }
   
   void database_api_impl::cancel_all_subscriptions()
   {
      set_subscribe_callback( std::function<void(const fc::variant&)>(), true);
   }
   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Blocks and transactions                                          //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   optional<block_header> database_api::get_block_header(uint32_t block_num)const
   {
      return my->get_block_header( block_num );
   }

   vector<pair<uint32_t, optional<signed_block_with_info>>> database_api::get_blocks(vector<uint32_t> blocks)const
   {
      vector<pair<uint32_t, optional<signed_block_with_info>>> result;
      result.resize(blocks.size());

      for (size_t index = 0; index < blocks.size(); ++index)
      {
         uint32_t block = blocks[index];
         result[index].first = block;
         result[index].second = get_block(block);
      }

      return result;
   }
   
   optional<block_header> database_api_impl::get_block_header(uint32_t block_num) const
   {
      auto result = _db.fetch_block_by_number(block_num);
      if(result)
         return *result;
      return {};
   }
   
   optional<signed_block_with_info> database_api::get_block(uint32_t block_num)const
   {
      if (auto result = my->get_block( block_num ))
          return *result;
      return {};
   }
   
   optional<signed_block> database_api_impl::get_block(uint32_t block_num)const
   {
      return _db.fetch_block_by_number(block_num);
   }
   
   processed_transaction database_api::get_transaction( uint32_t block_num, uint32_t trx_in_block )const
   {
      return my->get_transaction( block_num, trx_in_block );
   }

   fc::time_point_sec database_api::head_block_time() const
   {
      return my->head_block_time();
   }
   
   optional<signed_transaction> database_api::get_recent_transaction_by_id( const transaction_id_type& id )const
   {
      try {
         return my->_db.get_recent_transaction( id );
      } catch ( ... ) {
         return optional<signed_transaction>();
      }
   }

   map<string, signed_transaction> database_api::get_transactions_by_id(vector<string> ids) const
   {
       set<transaction_id_type>        id_set;
       map<string, signed_transaction> result;
       for(auto &&id : ids)
           id_set.insert(transaction_id_type(id));

       auto hbn = get_head_block( )->block_num( );
       for(size_t i = 0; i < hbn; ++i)
       {
           auto blk = get_block(i);
           if(blk)
               for(auto &&tx : blk->transactions)
               {
                   if(id_set.empty( ))
                       return result;

                   auto it = id_set.find(tx.id( ));
                   if(id_set.end( ) != it)
                   {
                       result[string(tx.id( ))] = signed_transaction(tx);
                       id_set.erase(it);
                   }
               }
       }
       return result;
   }

   processed_transaction database_api_impl::get_transaction(uint32_t block_num, uint32_t trx_num)const
   {
      auto opt_block = _db.fetch_block_by_number(block_num);
      FC_ASSERT( opt_block );
      FC_ASSERT( opt_block->transactions.size() > trx_num );
      return opt_block->transactions[trx_num];
   }

   fc::time_point_sec database_api_impl::head_block_time() const
   {
      return _db.head_block_time();
   }

   optional<signed_block> database_api::get_head_block() const
   {
       return my->get_head_block();
   }

   optional<signed_block> database_api::get_nearest_block(const string& time_iso_str) const
   {
       return my->get_nearest_block(time_iso_str);
   }
   optional<signed_block> database_api_impl::get_nearest_block(const string& time_iso_str) const
   {
       int64_t time = fc::time_point_sec::from_iso_string(time_iso_str).sec_since_epoch();

       int64_t block_num_hint_ = get_head_block()->block_num();
       block_num_hint_ -= (get_head_block()->timestamp.sec_since_epoch() - time) / GRAPHENE_DEFAULT_BLOCK_INTERVAL;

       if (block_num_hint_ > get_head_block()->block_num())
           return get_head_block();
       else if (block_num_hint_ < 1)
           return get_block(1);

       uint32_t block_num_hint = static_cast<uint32_t>(block_num_hint_), block_num_hint_prev = block_num_hint;

       int64_t time_a = _db.fetch_block_by_number(block_num_hint)->timestamp.sec_since_epoch(), time_b = time_a;

       do
       {
           block_num_hint_prev = block_num_hint;
           if (time_a>time)
               block_num_hint--;
           else if(time_a<time)
               block_num_hint++;

           time_b = time_a;
           time_a = _db.fetch_block_by_number(block_num_hint)->timestamp.sec_since_epoch();
       } while ( (time-time_a)*(time-time_b) > 0 );

       if ( std::abs<int64_t>(time-time_a) < std::abs<int64_t>(time-time_b) )
           return _db.fetch_block_by_number(block_num_hint);
       else
           return _db.fetch_block_by_number(block_num_hint_prev);
   }

   optional<signed_block> database_api_impl::get_head_block() const
   {
       return _db.fetch_block_by_number(_db.head_block_num());
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Globals                                                          //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   chain_property_object database_api::get_chain_properties()const
   {
      return my->get_chain_properties();
   }
   
   chain_property_object database_api_impl::get_chain_properties()const
   {
      return _db.get(chain_property_id_type());
   }
   
   global_property_object database_api::get_global_properties()const
   {
      return my->get_global_properties();
   }
   
   global_property_object database_api_impl::get_global_properties()const
   {
      return _db.get(global_property_id_type());
   }
   
   fc::variant_object database_api::get_config()const
   {
      return my->get_config();
   }
   
   fc::variant_object database_api_impl::get_config()const
   {
      return graphene::chain::get_config();
   }
   
   chain_id_type database_api::get_chain_id()const
   {
      return my->get_chain_id();
   }
   
   chain_id_type database_api_impl::get_chain_id()const
   {
      return _db.get_chain_id();
   }
   
   dynamic_global_property_object database_api::get_dynamic_global_properties()const
   {
      return my->get_dynamic_global_properties();
   }
   
   dynamic_global_property_object database_api_impl::get_dynamic_global_properties()const
   {
      return _db.get(dynamic_global_property_id_type());
   }
   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Keys                                                             //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   vector<vector<account_id_type>> database_api::get_key_references( vector<public_key_type> key )const
   {
      return my->get_key_references( key );
   }
   
   /**
    *  @return all accounts that refer to the key or account id in their owner or active authorities.
    */
   vector<vector<account_id_type>> database_api_impl::get_key_references( vector<public_key_type> keys )const
   {
      wdump( (keys) );
      vector< vector<account_id_type> > final_result;
      final_result.reserve(keys.size());
      
      for( auto& key : keys )
      {
         subscribe_to_item( key );
         
         const auto& idx = _db.get_index_type<account_index>();
         const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
         const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
         auto itr = refs.account_to_key_memberships.find(key);
         vector<account_id_type> result;
         
         if( itr != refs.account_to_key_memberships.end() )
         {
            result.reserve( itr->second.size() );
            for( auto item : itr->second ) result.push_back(item);
         }
         final_result.emplace_back( std::move(result) );
      }
      
      for( auto i : final_result )
         subscribe_to_item(i);
      
      return final_result;
   }
   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Accounts                                                         //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   vector<optional<account_object>> database_api::get_accounts(const vector<account_id_type>& account_ids)const
   {
      return my->get_accounts( account_ids );
   }
   
   vector<optional<account_object>> database_api_impl::get_accounts(const vector<account_id_type>& account_ids)const
   {
      vector<optional<account_object>> result; result.reserve(account_ids.size());
      std::transform(account_ids.begin(), account_ids.end(), std::back_inserter(result),
                     [this](account_id_type id) -> optional<account_object> {
                        if(auto o = _db.find(id))
                        {
                           subscribe_to_item( id );
                           return *o;
                        }
                        return {};
                     });
      return result;
   }
   
   std::map<string,full_account> database_api::get_full_accounts( const vector<string>& names_or_ids, bool subscribe )
   {
      return my->get_full_accounts( names_or_ids, subscribe );
   }
   
   std::map<std::string, full_account> database_api_impl::get_full_accounts( const vector<std::string>& names_or_ids, bool subscribe)
   {
      idump((names_or_ids));
      std::map<std::string, full_account> results;
      
      for (const std::string& account_name_or_id : names_or_ids)
      {
         const account_object* account = nullptr;
         if (std::isdigit(account_name_or_id[0]))
            account = _db.find(fc::variant(account_name_or_id).as<account_id_type>());
         else
         {
            const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
            auto itr = idx.find(account_name_or_id);
            if (itr != idx.end())
               account = &*itr;
         }
         if (account == nullptr)
            continue;
         
         if( subscribe )
         {
            ilog( "subscribe to ${id}", ("id",account->name) );
            subscribe_to_item( account->id );
         }
         
         // fc::mutable_variant_object full_account;
         full_account acnt;
         acnt.account = *account;
         acnt.statistics = account->statistics(_db);
         acnt.registrar_name = account->registrar(_db).name;
         acnt.votes = lookup_vote_ids( vector<vote_id_type>(account->options.votes.begin(),account->options.votes.end()) );
         
         // Add the account itself, its statistics object, cashback balance, and referral account names
         /*
          full_account("account", *account)("statistics", account->statistics(_db))
          ("registrar_name", account->registrar(_db).name)("referrer_name", account->referrer(_db).name)
          ("lifetime_referrer_name", account->lifetime_referrer(_db).name);
          */
         if (account->cashback_vb)
         {
            acnt.cashback_balance = account->cashback_balance(_db);
         }
         // Add the account's proposals
         const auto& proposal_idx = _db.get_index_type<proposal_index>();
         const auto& pidx = dynamic_cast<const primary_index<proposal_index>&>(proposal_idx);
         const auto& proposals_by_account = pidx.get_secondary_index<graphene::chain::required_approval_index>();
         auto  required_approvals_itr = proposals_by_account._account_to_proposals.find( account->id );
         if( required_approvals_itr != proposals_by_account._account_to_proposals.end() )
         {
            acnt.proposals.reserve( required_approvals_itr->second.size() );
            for( auto proposal_id : required_approvals_itr->second )
               acnt.proposals.push_back( proposal_id(_db) );
         }
         
         
         // Add the account's balances
         auto balance_range = _db.get_index_type<account_balance_index>().indices().get<by_account_asset>().equal_range(boost::make_tuple(account->id));
         //vector<account_balance_object> balances;
         std::for_each(balance_range.first, balance_range.second,
                       [&acnt](const account_balance_object& balance) {
                          acnt.balances.emplace_back(balance);
                       });
         
         // Add the account's vesting balances
         auto vesting_range = _db.get_index_type<vesting_balance_index>().indices().get<by_account>().equal_range(account->id);
         std::for_each(vesting_range.first, vesting_range.second,
                       [&acnt](const vesting_balance_object& balance) {
                          acnt.vesting_balances.emplace_back(balance);
                       });
         
         results[account_name_or_id] = acnt;
      }
      return results;
   }
   
   optional<account_object> database_api::get_account_by_name( string name )const
   {
      return my->get_account_by_name( name );
   }
   
   optional<account_object> database_api_impl::get_account_by_name( string name )const
   {
      const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = idx.find(name);
      if (itr != idx.end())
         return *itr;
      return optional<account_object>();
   }
   
   vector<account_id_type> database_api::get_account_references( account_id_type account_id )const
   {
      return my->get_account_references( account_id );
   }
   
   vector<account_id_type> database_api_impl::get_account_references( account_id_type account_id )const
   {
      const auto& idx = _db.get_index_type<account_index>();
      const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
      const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
      auto itr = refs.account_to_account_memberships.find(account_id);
      vector<account_id_type> result;
      
      if( itr != refs.account_to_account_memberships.end() )
      {
         result.reserve( itr->second.size() );
         for( auto item : itr->second ) result.push_back(item);
      }
      return result;
   }
   
   vector<optional<account_object>> database_api::lookup_account_names(const vector<string>& account_names)const
   {
      return my->lookup_account_names( account_names );
   }
   
   vector<optional<account_object>> database_api_impl::lookup_account_names(const vector<string>& account_names)const
   {
      const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
      vector<optional<account_object> > result;
      result.reserve(account_names.size());
      std::transform(account_names.begin(), account_names.end(), std::back_inserter(result),
                     [&accounts_by_name](const string& name) -> optional<account_object> {
                        auto itr = accounts_by_name.find(name);
                        return itr == accounts_by_name.end()? optional<account_object>() : *itr;
                     });
      return result;
   }
   
   
   vector<account_object> database_api::search_accounts(const string& search_term, const string order, const object_id_type& id, uint32_t limit) const {
      return my->search_accounts( search_term, order, id, limit );
   }

   vector<transaction_detail_object> database_api::search_account_history(string const& account_name,
                                                                          string const& order,
                                                                          string const& id,
                                                                          int limit) const
   {
      return my->search_account_history(account_name, order, id, limit);
   }


   map<string,account_id_type> database_api::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
   {
      return my->lookup_accounts( lower_bound_name, limit );
   }

   namespace
   {
      template <  typename _t_object_index,
      typename _t_object,
      typename _t_sort_tag,
      typename _t_iterator,
      bool is_ascending>
      void correct_iterator(graphene::chain::database& db,
                            const object_id_type& id,
                            _t_iterator& itr_begin)
      {
         const auto& idx_by_id = db.get_index_type<_t_object_index>().indices().template get<by_id>();
         auto itr_id = idx_by_id.find(id);

         const auto& idx_by_sort_tag = db.get_index_type<_t_object_index>().indices().template get<_t_sort_tag>();

         auto itr_find = idx_by_sort_tag.end();
         if (itr_id != idx_by_id.end())
            itr_find = idx_by_sort_tag.find(key_extractor<_t_sort_tag, _t_object>::get(*itr_id));

         // itr_find has the same keys as the object with id
         // scan to next items until exactly the object with id is found
         auto itr_scan = itr_find;
         while (itr_find != idx_by_sort_tag.end() &&
                itr_id != idx_by_id.end() &&
                ++itr_scan != idx_by_sort_tag.end() &&
                itr_find->id != itr_id->id &&
                key_extractor<_t_sort_tag, _t_object>::get(*itr_scan) == key_extractor<_t_sort_tag, _t_object>::get(*itr_id))
            itr_find = itr_scan;

         if (itr_find != idx_by_sort_tag.end())
         {
            itr_begin = return_one<is_ascending>::choose(itr_find, boost::reverse_iterator<decltype(itr_find)>(itr_find));
            if (false == is_ascending)
               --itr_begin;
         }
      }

      template <bool is_ascending, class sort_tag>
      void search_accounts_template(graphene::chain::database& db,
                                    const string& term,
                                    uint32_t count,
                                    const object_id_type& id,
                                    vector<account_object>& result)
      {
         const auto& idx_by_sort_tag = db.get_index_type<account_index>().indices().get<sort_tag>();

         auto itr_begin = return_one<is_ascending>::choose(idx_by_sort_tag.cbegin(), idx_by_sort_tag.crbegin());
         auto itr_end = return_one<is_ascending>::choose(idx_by_sort_tag.end(), idx_by_sort_tag.rend());

         correct_iterator<account_index, account_object, sort_tag, decltype(itr_begin), is_ascending>(db, id, itr_begin);

         while(count &&
               itr_begin != itr_end)
         {
            account_object const& element = *itr_begin;
            ++itr_begin;


            std::string account_id_str = fc::variant(element.get_id()).as<std::string>();
            std::string account_name = element.name;
            std::string search_term = term;

            boost::algorithm::to_lower(account_id_str);
            boost::algorithm::to_lower(account_name);
            boost::algorithm::to_lower(search_term);

            if (false == search_term.empty() &&
                account_name.find(search_term) == std::string::npos &&
                account_id_str.find(search_term) == std::string::npos)
               continue;
            
            result.emplace_back(element);
            --count;
         }
      }
   }
   
   vector<account_object> database_api_impl::search_accounts(const string& term, const string order, const object_id_type& id, uint32_t limit)const
   {
      FC_ASSERT( limit <= 1000 );
      vector<account_object> result;

      if (order == "+id")
         search_accounts_template<true, by_id>(_db, term, limit, id, result);
      else if (order == "-id")
         search_accounts_template<false, by_id>(_db, term, limit, id, result);
      else if (order == "-name")
         search_accounts_template<false, by_name>(_db, term, limit, id, result);
      else
         search_accounts_template<true, by_name>(_db, term, limit, id, result);


      return result;
   }

   namespace
   {
      template <bool is_ascending, class sort_tag>
      void search_account_history_template(graphene::chain::database& db,
                                           const account_id_type& account,
                                           uint32_t count,
                                           const object_id_type& id,
                                           vector<transaction_detail_object>& result)
      {
         const auto& idx_by_sort_tag = db.get_index_type<transaction_detail_index>().indices().get<sort_tag>();

         auto itr_begin = return_one<is_ascending>::choose(idx_by_sort_tag.cbegin(), idx_by_sort_tag.crbegin());
         auto itr_end = return_one<is_ascending>::choose(idx_by_sort_tag.end(), idx_by_sort_tag.rend());

         correct_iterator<transaction_detail_index, transaction_detail_object, sort_tag, decltype(itr_begin), is_ascending>(db, id, itr_begin);

         while(count &&
               itr_begin != itr_end)
         {
            transaction_detail_object element = *itr_begin;
            ++itr_begin;

            if (account != element.m_from_account &&
                account != element.m_to_account)
               continue;

            account_object from = element.m_from_account(db);
            account_object to = element.m_to_account(db);

            element.m_from_name = from.name;
            element.m_to_name = to.name;

            result.emplace_back(element);
            --count;
         }
      }
   }

   vector<transaction_detail_object> database_api_impl::search_account_history(std::string const& account_name,
                                                                               std::string const& order,
                                                                               std::string const& id,
                                                                               int limit) const
   {
      vector<transaction_detail_object> result;

      const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = accounts_by_name.find(account_name);
      if( itr != accounts_by_name.end() )
      {
      auto account = itr->get_id();
      if (order == "+type")
         search_account_history_template<true, by_operation_type>(_db, account, limit, object_id_type(id), result);
      else if (order == "-type")
         search_account_history_template<false, by_operation_type>(_db, account, limit, object_id_type(id), result);
      else if (order == "+to")
         search_account_history_template<true, by_to_account>(_db, account, limit, object_id_type(id), result);
      else if (order == "-to")
         search_account_history_template<false, by_to_account>(_db, account, limit, object_id_type(id), result);
      else if (order == "+from")
         search_account_history_template<true, by_from_account>(_db, account, limit, object_id_type(id), result);
      else if (order == "-from")
         search_account_history_template<false, by_from_account>(_db, account, limit, object_id_type(id), result);
      else if (order == "+price")
         search_account_history_template<true, by_transaction_amount>(_db, account, limit, object_id_type(id), result);
      else if (order == "-price")
         search_account_history_template<false, by_transaction_amount>(_db, account, limit, object_id_type(id), result);
      else if (order == "+fee")
         search_account_history_template<true, by_transaction_fee>(_db, account, limit, object_id_type(id), result);
      else if (order == "-fee")
         search_account_history_template<false, by_transaction_fee>(_db, account, limit, object_id_type(id), result);
      else if (order == "+description")
         search_account_history_template<true, by_description>(_db, account, limit, object_id_type(id), result);
      else if (order == "-description")
         search_account_history_template<false, by_description>(_db, account, limit, object_id_type(id), result);
      else if (order == "+time")
         search_account_history_template<true, by_time>(_db, account, limit, object_id_type(id), result);
      else// if (order == "-time")
         search_account_history_template<false, by_time>(_db, account, limit, object_id_type(id), result);
      }

      return result;
   }

   map<string,account_id_type> database_api_impl::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
   {
      FC_ASSERT( limit <= 1000 );
      const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
      map<string,account_id_type> result;
      
      for( auto itr = accounts_by_name.lower_bound(lower_bound_name);
          limit-- && itr != accounts_by_name.end();
          ++itr )
      {
         result.insert(make_pair(itr->name, itr->get_id()));
         if( limit == 1 )
            subscribe_to_item( itr->get_id() );
      }
      
      return result;
   }
   
   uint64_t database_api::get_account_count()const
   {
      return my->get_account_count();
   }
   
   uint64_t database_api_impl::get_account_count()const
   {
      return _db.get_index_type<account_index>().indices().size();
   }
   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Balances                                                         //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   vector<asset> database_api::get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const
   {
      return my->get_account_balances( id, assets );
   }
   
   vector<asset> database_api_impl::get_account_balances(account_id_type acnt, const flat_set<asset_id_type>& assets)const
   {
      vector<asset> result;
      if (assets.empty())
      {
         // if the caller passes in an empty list of assets, return balances for all assets the account owns
         const account_balance_index& balance_index = _db.get_index_type<account_balance_index>();
         auto range = balance_index.indices().get<by_account_asset>().equal_range(boost::make_tuple(acnt));
         for (const account_balance_object& balance : boost::make_iterator_range(range.first, range.second))
            result.push_back(asset(balance.get_balance()));
      }
      else
      {
         result.reserve(assets.size());
         
         std::transform(assets.begin(), assets.end(), std::back_inserter(result),
                        [this, acnt](asset_id_type id) { return _db.get_balance(acnt, id); });
      }
      
      return result;
   }
   
   vector<asset> database_api::get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const
   {
      return my->get_named_account_balances( name, assets );
   }
   
   vector<asset> database_api_impl::get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets) const
   {
      const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = accounts_by_name.find(name);
      if( itr != accounts_by_name.end() )
        return get_account_balances(itr->get_id(), assets);
      else
      {
          vector<asset> result;
          if (assets.empty())
             result.push_back(asset(0, asset_id_type()));
          else
          {
             result.reserve(assets.size());
             for (auto asset_item : assets)
                 result.push_back(asset(0, asset_item));
          }

          return result;
      }
   }
   
   vector<vesting_balance_object> database_api::get_vesting_balances( account_id_type account_id )const
   {
      return my->get_vesting_balances( account_id );
   }
   
   vector<vesting_balance_object> database_api_impl::get_vesting_balances( account_id_type account_id )const
   {
      try
      {
         vector<vesting_balance_object> result;
         auto vesting_range = _db.get_index_type<vesting_balance_index>().indices().get<by_account>().equal_range(account_id);
         std::for_each(vesting_range.first, vesting_range.second,
                       [&result](const vesting_balance_object& balance) {
                          result.emplace_back(balance);
                       });
         return result;
      }
      FC_CAPTURE_AND_RETHROW( (account_id) );
   }

   vector<vesting_balance_object_with_info> database_api::get_vesting_balances_with_info(account_id_type account_id) const
   {
       std::vector<vesting_balance_object_with_info> result;

       vector<vesting_balance_object> vbos = get_vesting_balances(account_id);
       if(vbos.size( ) == 0)
           return result;

       for(const vesting_balance_object &vbo : vbos)
           result.emplace_back(vbo, head_block_time( ));

       return result;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Assets                                                           //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   vector<optional<asset_object>> database_api::get_assets(const vector<asset_id_type>& asset_ids)const
   {
      return my->get_assets( asset_ids );
   }
   
   vector<optional<asset_object>> database_api_impl::get_assets(const vector<asset_id_type>& asset_ids)const
   {
      vector<optional<asset_object>> result; result.reserve(asset_ids.size());
      std::transform(asset_ids.begin(), asset_ids.end(), std::back_inserter(result),
                     [this](asset_id_type id) -> optional<asset_object> {
                        if(auto o = _db.find(id))
                        {
                           subscribe_to_item( id );
                           return *o;
                        }
                        return {};
                     });
      return result;
   }
   
   vector<asset_object> database_api::list_assets(const string& lower_bound_symbol, uint32_t limit)const
   {
      return my->list_assets( lower_bound_symbol, limit );
   }
   
   vector<asset_object> database_api_impl::list_assets(const string& lower_bound_symbol, uint32_t limit)const
   {
      FC_ASSERT( limit <= 100 );
      const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
      vector<asset_object> result;
      result.reserve(limit);
      
      auto itr = assets_by_symbol.lower_bound(lower_bound_symbol);
      
      if( lower_bound_symbol == "" )
         itr = assets_by_symbol.begin();
      
      while(limit-- && itr != assets_by_symbol.end())
         result.emplace_back(*itr++);
      
      return result;
   }
   
   vector<optional<asset_object>> database_api::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
   {
      return my->lookup_asset_symbols( symbols_or_ids );
   }
   
   vector<optional<asset_object>> database_api_impl::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
   {
      const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
      vector<optional<asset_object> > result;
      result.reserve(symbols_or_ids.size());
      std::transform(symbols_or_ids.begin(), symbols_or_ids.end(), std::back_inserter(result),
                     [this, &assets_by_symbol](const string& symbol_or_id) -> optional<asset_object> {
                        if( !symbol_or_id.empty() && std::isdigit(symbol_or_id[0]) )
                        {
                           auto ptr = _db.find(variant(symbol_or_id).as<asset_id_type>());
                           return ptr == nullptr? optional<asset_object>() : *ptr;
                        }
                        auto itr = assets_by_symbol.find(symbol_or_id);
                        return itr == assets_by_symbol.end()? optional<asset_object>() : *itr;
                     });
      return result;
   }
   

   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Miners                                                        //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   vector<optional<miner_object>> database_api::get_miners(const vector<miner_id_type>& miner_ids)const
   {
      return my->get_miners( miner_ids );
   }
   
   
   vector<optional<miner_object>> database_api_impl::get_miners(const vector<miner_id_type>& miner_ids)const
   {
      vector<optional<miner_object>> result; result.reserve(miner_ids.size());
      std::transform(miner_ids.begin(), miner_ids.end(), std::back_inserter(result),
                     [this](miner_id_type id) -> optional<miner_object> {
                        if(auto o = _db.find(id))
                           return *o;
                        return {};
                     });
      return result;
   }
   
   fc::optional<miner_object> database_api::get_miner_by_account(account_id_type account)const
   {
      return my->get_miner_by_account( account );
   }
   
   fc::optional<miner_object> database_api_impl::get_miner_by_account(account_id_type account) const
   {
      const auto& idx = _db.get_index_type<miner_index>().indices().get<by_account>();
      auto itr = idx.find(account);
      if( itr != idx.end() )
         return *itr;
      return {};
   }
   
   map<string, miner_object> database_api::lookup_miner_accounts(const string& lower_bound_name, uint32_t limit)const
   {
      return my->lookup_miner_accounts( lower_bound_name, limit );
   }
   
   map<string, miner_object> database_api_impl::lookup_miner_accounts(const string& lower_bound_name, uint32_t limit)const
   {
      FC_ASSERT( limit <= 1000 );
      const auto& miners_by_id = _db.get_index_type<miner_index>().indices().get<by_id>();
      
      // we want to order miners by account name, but that name is in the account object
      // so the miner_index doesn't have a quick way to access it.
      // get all the names and look them all up, sort them, then figure out what
      // records to return.  This could be optimized, but we expect the
      // number of miners to be few and the frequency of calls to be rare
      std::map<std::string, miner_object> miners_by_account_name;
      for (const miner_object& miner : miners_by_id)
         if (auto account_iter = _db.find(miner.miner_account))
            if (account_iter->name >= lower_bound_name) // we can ignore anything below lower_bound_name
               miners_by_account_name.insert(std::make_pair(account_iter->name, miner));
      
      auto end_iter = miners_by_account_name.begin();
      while (end_iter != miners_by_account_name.end() && limit--)
         ++end_iter;
      miners_by_account_name.erase(end_iter, miners_by_account_name.end());
      return miners_by_account_name;
   }
   
   uint64_t database_api::get_miner_count()const
   {
      return my->get_miner_count();
   }
   
   uint64_t database_api_impl::get_miner_count()const
   {
      return _db.get_index_type<miner_index>().indices().size();
   }

   vector<account_balance_object> database_api::get_miner_voters(const string &miner_id)const
   {
       return my->get_miner_voters(miner_id);
   }

   vector<account_balance_object> database_api_impl::get_miner_voters(const string &miner_id)const
   {
       vector<account_balance_object> result;
       auto const & mix = _db.get_index_type<miner_index>().indices();

       auto const & mit = mix.find(object_id_type(miner_id));
       if (mit == mix.end())
           return result;

       auto const & vid = mit->vote_id;
       auto const & abix = _db.get_index_type<account_balance_index>().indices().get<by_voted_miner>();

       auto _begin = abix.lower_bound(vid);
       auto _end = abix.upper_bound(vid);
       for(auto a = _begin; a != _end; ++a)
           result.push_back(*a);

       return result;
   }


   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Votes                                                            //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   vector<variant> database_api::lookup_vote_ids( const vector<vote_id_type>& votes )const
   {
      return my->lookup_vote_ids( votes );
   }
   
   vector<variant> database_api_impl::lookup_vote_ids( const vector<vote_id_type>& votes )const
   {
      FC_ASSERT( votes.size() < 1000, "Only 1000 votes can be queried at a time" );
      
      const auto& miner_idx = _db.get_index_type<miner_index>().indices().get<by_vote_id>();
      
      vector<variant> result;
      result.reserve( votes.size() );
      for( auto id : votes )
      {
         switch( id.type() )
         {
            case vote_id_type::miner:
            {
               auto itr = miner_idx.find( id );
               if( itr != miner_idx.end() )
                  result.emplace_back( variant( *itr ) );
               else
                  result.emplace_back( variant() );
               break;
            }
               
            case vote_id_type::VOTE_TYPE_COUNT: break; // supress unused enum value warnings
         }
      }
      return result;
   }
   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Authority / validation                                           //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   std::string database_api::get_transaction_hex(const signed_transaction& trx)const
   {
      return my->get_transaction_hex( trx );
   }
   
   std::string database_api_impl::get_transaction_hex(const signed_transaction& trx)const
   {
      return fc::to_hex(fc::raw::pack(trx));
   }
   
   set<public_key_type> database_api::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
   {
      return my->get_required_signatures( trx, available_keys );
   }
   
   set<public_key_type> database_api_impl::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
   {
      wdump((trx)(available_keys));
      auto result = trx.get_required_signatures( _db.get_chain_id(),
                                                available_keys,
                                                [&]( account_id_type id ){ return &id(_db).owner; },
                                                [&]( account_id_type id ){ return &id(_db).owner; },
                                                _db.get_global_properties().parameters.max_authority_depth );
      wdump((result));
      return result;
   }
   
   set<public_key_type> database_api::get_potential_signatures( const signed_transaction& trx )const
   {
      return my->get_potential_signatures( trx );
   }
   
   set<public_key_type> database_api_impl::get_potential_signatures( const signed_transaction& trx )const
   {
      wdump((trx));
      set<public_key_type> result;
      trx.get_required_signatures(
                                  _db.get_chain_id(),
                                  flat_set<public_key_type>(),
                                  [&]( account_id_type id )
                                  {
                                     const auto& auth = id(_db).owner;
                                     for( const auto& k : auth.get_keys() )
                                        result.insert(k);
                                     return &auth;
                                  },
                                  [&]( account_id_type id )
                                  {
                                     const auto& auth = id(_db).owner;
                                     for( const auto& k : auth.get_keys() )
                                        result.insert(k);
                                     return &auth;
                                  },
                                  _db.get_global_properties().parameters.max_authority_depth
                                  );
      
      wdump((result));
      return result;
   }
   
   bool database_api::verify_authority( const signed_transaction& trx )const
   {
      return my->verify_authority( trx );
   }
   
   bool database_api_impl::verify_authority( const signed_transaction& trx )const
   {
      trx.verify_authority( _db.get_chain_id(),
                           [&]( account_id_type id ){ return &id(_db).owner; },
                           [&]( account_id_type id ){ return &id(_db).owner; },
                           _db.get_global_properties().parameters.max_authority_depth );
      return true;
   }
   
   bool database_api::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const
   {
      return my->verify_account_authority( name_or_id, signers );
   }
   
   bool database_api_impl::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& keys )const
   {
      FC_ASSERT( name_or_id.size() > 0);
      const account_object* account = nullptr;
      if (std::isdigit(name_or_id[0]))
         account = _db.find(fc::variant(name_or_id).as<account_id_type>());
      else
      {
         const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
         auto itr = idx.find(name_or_id);
         if (itr != idx.end())
            account = &*itr;
      }
      FC_ASSERT( account, "no such account" );
      
      
      /// reuse trx.verify_authority by creating a dummy transfer
      signed_transaction trx;
      transfer_operation op;
      op.from = account->id;
      trx.operations.emplace_back(op);
      
      return verify_authority( trx );
   }
   
   processed_transaction database_api::validate_transaction( const signed_transaction& trx )const
   {
      return my->validate_transaction( trx );
   }
   
   processed_transaction database_api_impl::validate_transaction( const signed_transaction& trx )const
   {
      return _db.validate_transaction(trx);
   }
   
   vector< fc::variant > database_api::get_required_fees( const vector<operation>& ops, asset_id_type id )const
   {
      return my->get_required_fees( ops, id );
   }
   
   /**
    * Container method for mutually recursive functions used to
    * implement get_required_fees() with potentially nested proposals.
    */
   struct get_required_fees_helper
   {
      get_required_fees_helper(
                               const fee_schedule& _current_fee_schedule,
                               uint32_t _max_recursion
                               )
      : current_fee_schedule(_current_fee_schedule),
      max_recursion(_max_recursion)
      {}
      
      fc::variant set_op_fees( operation& op )
      {
         if( op.which() == operation::tag<proposal_create_operation>::value )
         {
            return set_proposal_create_op_fees( op );
         }
         else
         {
            price core_exchange_rate( asset(1, asset_id_type()), asset(1, asset_id_type()));
            asset fee = current_fee_schedule.set_fee( op, core_exchange_rate );
            fc::variant result;
            fc::to_variant( fee, result );
            return result;
         }
      }
      
      fc::variant set_proposal_create_op_fees( operation& proposal_create_op )
      {
         proposal_create_operation& op = proposal_create_op.get<proposal_create_operation>();
         std::pair< asset, fc::variants > result;
         for( op_wrapper& prop_op : op.proposed_ops )
         {
            FC_ASSERT( current_recursion < max_recursion );
            ++current_recursion;
            result.second.push_back( set_op_fees( prop_op.op ) );
            --current_recursion;
         }
         // we need to do this on the boxed version, which is why we use
         // two mutually recursive functions instead of a visitor
         price core_exchange_rate( asset(1, asset_id_type()), asset(1, asset_id_type()));
         result.first = current_fee_schedule.set_fee( proposal_create_op, core_exchange_rate );
         fc::variant vresult;
         fc::to_variant( result, vresult );
         return vresult;
      }
      
      const fee_schedule& current_fee_schedule;
      uint32_t max_recursion;
      uint32_t current_recursion = 0;
   };
   
   vector< fc::variant > database_api_impl::get_required_fees( const vector<operation>& ops, asset_id_type id )const
   {
      vector< operation > _ops = ops;
      //
      // we copy the ops because we need to mutate an operation to reliably
      // determine its fee, see #435
      //
      
      vector< fc::variant > result;
      result.reserve(ops.size());
      get_required_fees_helper helper(
                                      _db.current_fee_schedule(),
                                      GET_REQUIRED_FEES_MAX_RECURSION );
      for( operation& op : _ops )
      {
         result.push_back( helper.set_op_fees( op ) );
      }
      return result;
   }
   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Proposed transactions                                            //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   vector<proposal_object> database_api::get_proposed_transactions( account_id_type id )const
   {
      return my->get_proposed_transactions( id );
   }
   
   /** TODO: add secondary index that will accelerate this process */
   vector<proposal_object> database_api_impl::get_proposed_transactions( account_id_type id )const
   {
      const auto& idx = _db.get_index_type<proposal_index>();
      vector<proposal_object> result;
      
      idx.inspect_all_objects( [&](const object& obj){
         const proposal_object& p = static_cast<const proposal_object&>(obj);
         if( p.required_active_approvals.find( id ) != p.required_active_approvals.end() )
            result.push_back(p);
         else if ( p.required_owner_approvals.find( id ) != p.required_owner_approvals.end() )
            result.push_back(p);
         else if ( p.available_active_approvals.find( id ) != p.available_active_approvals.end() )
            result.push_back(p);
      });
      return result;
   }
   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // CYVA                                                           //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   real_supply database_api::get_real_supply()const
   {
      return my->_db.get_real_supply();
   }

   miner_reward_input database_api_impl::get_time_to_maint_by_block_time(fc::time_point_sec block_time) const
   {
      const auto& idx = _db.get_index_type<budget_record_index>().indices().get<by_time>();
      graphene::chain::miner_reward_input miner_reward_input;
      memset(&miner_reward_input, 0, sizeof(miner_reward_input));

      auto itr = idx.begin();
  
      fc::time_point_sec next_time = (fc::time_point_sec)0;
      fc::time_point_sec prev_time = (fc::time_point_sec)0;

      for (itr = itr; (itr != idx.end()) && (next_time == (fc::time_point_sec)0); ++itr)
      {
         budget_record_object bro = (*itr);
         if (itr->record.next_maintenance_time > block_time)
         {
            next_time = itr->record.next_maintenance_time;
            miner_reward_input.from_accumulated_fees = itr->record.from_accumulated_fees;
            miner_reward_input.block_interval = itr->record.block_interval;
         }
      }

      FC_ASSERT(next_time != (fc::time_point_sec)0);
      
      itr--;
      
      if (itr == idx.begin())
      {
         fc::optional<signed_block> first_block = get_block(1);
         prev_time = first_block->timestamp;
         miner_reward_input.time_to_maint = (next_time - prev_time).to_seconds();

         return miner_reward_input;
      }

      itr--;
      
      prev_time = (*itr).record.next_maintenance_time;
      miner_reward_input.time_to_maint = (next_time - prev_time).to_seconds();
      
      return miner_reward_input;
   }
   
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Blinded balances                                                 //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   vector<blinded_balance_object> database_api::get_blinded_balances( const flat_set<commitment_type>& commitments )const
   {
      return my->get_blinded_balances( commitments );
   }

   vector<blinded_balance_object> database_api_impl::get_blinded_balances( const flat_set<commitment_type>& commitments )const
   {
      vector<blinded_balance_object> result; result.reserve(commitments.size());
      const auto& bal_idx = _db.get_index_type<blinded_balance_index>();
      const auto& by_commitment_idx = bal_idx.indices().get<by_commitment>();
      for( const auto& c : commitments )
      {
         auto itr = by_commitment_idx.find( c );
         if( itr != by_commitment_idx.end() )
            result.push_back( *itr );
      }
      return result;
   }


   vector<confidential_tx_object> database_api::get_confidential_transactions(const string &a, const string &B, bool unspent) const
   {
       try
       {
           auto B_ = public_key_type(B);
           auto a_ = utilities::wif_to_key(a);
           if(a_)
               return my->get_confidential_transactions(*a_, B_, unspent);
       }
       catch(...)
       {
       }
       return {};
   }

   vector<confidential_tx_object> database_api_impl::get_confidential_transactions(fc::ecc::private_key const &a, const fc::ecc::public_key &B, bool unspent)const
   {
      vector<confidential_tx_object> result;
      const auto& bal_idx = _db.get_index_type<confidential_tx_index>();
      const auto& trxs = bal_idx.indices().get<by_unspent>();

      std::copy_if(trxs.lower_bound(unspent), trxs.end(), std::back_inserter(result), [&](confidential_tx_object const & t)
      {
          auto p = B.add(fc::sha256::hash(a.get_shared_secret(t.tx_key)));
          return public_key_type(p) == t.owner;
      });

      return result;
   }
   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Private methods                                                  //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////
   
   void database_api_impl::broadcast_updates( const vector<variant>& updates )
   {
      if( updates.size() ) {
         auto capture_this = shared_from_this();
         fc::async([capture_this,updates](){
            capture_this->_subscribe_callback( fc::variant(updates) );
         });
      }
   }
   
   void database_api_impl::on_objects_removed( const vector<const object*>& objs )
   {
      /// we need to ensure the database_api is not deleted for the life of the async operation
      if( _subscribe_callback )
      {
         vector<variant>    updates;
         updates.reserve(objs.size());
         
         for( auto obj : objs )
            updates.emplace_back( obj->id );
         broadcast_updates( updates );
      }

   }
   
   void database_api_impl::on_objects_changed(const vector<object_id_type>& ids)
   {
      vector<variant>    updates;
      
      for(auto id : ids)
      {
         const object* obj = nullptr;
         if( _subscribe_callback )
         {
            obj = _db.find_object( id );
            if( obj )
            {
               updates.emplace_back( obj->to_variant() );
            }
            else
            {
               updates.emplace_back(id); // send just the id to indicate removal
            }
         }
      }
      
      auto capture_this = shared_from_this();
      
      /// pushing the future back / popping the prior future if it is complete.
      /// if a connection hangs then this could get backed up and result in
      /// a failure to exit cleanly.
      fc::async([capture_this,this,updates](){
         if( _subscribe_callback ) _subscribe_callback( updates );
      });
   }
   
   /** note: this method cannot yield because it is called in the middle of
    * apply a block.
    */
   void database_api_impl::on_applied_block()
   {
      if (_block_applied_callback)
      {
         auto capture_this = shared_from_this();
         block_id_type block_id = _db.head_block_id();
         fc::async([this,capture_this,block_id](){
            _block_applied_callback(fc::variant(block_id));
         });
      }
   }

   share_type database_api::get_new_asset_per_block()const
   {
      return my->get_new_asset_per_block();
   }

   share_type database_api_impl::get_new_asset_per_block() const
   {
      return _db.get_new_asset_per_block();
   }

   share_type database_api::get_asset_per_block_by_block_num(uint32_t block_num) const
   {
      return my->get_asset_per_block_by_block_num(block_num);
   }

   share_type database_api_impl::get_asset_per_block_by_block_num(uint32_t block_num) const
   {
      return _db.get_asset_per_block_by_block_num(block_num);
   }

   miner_reward_input database_api::get_time_to_maint_by_block_time(fc::time_point_sec block_time) const
   {
      return my->get_time_to_maint_by_block_time(block_time);
   }
   
} } // graphene::app
