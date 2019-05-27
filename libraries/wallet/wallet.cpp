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
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <utility>
#include <string>
#include <list>
#include <map>

#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/sort.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <boost/range/combine.hpp>

#include <fc/git_revision.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/network/http/connection.hpp>

#include <graphene/app/api.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/utilities/git_revision.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/string_escape.hpp>
#include <graphene/utilities/words.hpp>
#include <graphene/wallet/wallet.hpp>
#include <graphene/wallet/api_documentation.hpp>
#include <graphene/wallet/reflect_util.hpp>
#include <graphene/debug_miner/debug_api.hpp>


#include <fc/smart_ref_impl.hpp>

#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
#endif

#define BRAIN_KEY_WORD_COUNT 16

CryptoPP::AutoSeededRandomPool randomGenerator;




namespace graphene { namespace wallet {

namespace detail {

// this class helps to gather seeding statistics. Tracks seeders currently in use.

struct operation_result_printer
{
public:
   operation_result_printer( const wallet_api_impl& w )
      : _wallet(w) {}
   const wallet_api_impl& _wallet;
   typedef std::string result_type;

   std::string operator()(const void_result& x) const;
   std::string operator()(const object_id_type& oid);
   std::string operator()(const asset& a);
};

// BLOCK  TRX  OP  VOP
struct operation_printer
{
private:
   ostream& out;
   const wallet_api_impl& wallet;
   operation_result result;

   std::string fee(const asset& a) const;

public:
   operation_printer( ostream& out, const wallet_api_impl& wallet, const operation_result& r = operation_result() )
      : out(out),
        wallet(wallet),
        result(r)
   {}
   typedef std::string result_type;

   template<typename T>
   std::string operator()(const T& op)const;

   std::string operator()(const transfer_operation& op)const;
   std::string operator()(const account_create_operation& op)const;
   std::string operator()(const account_update_operation& op)const;
};


template<class T>
optional<T> maybe_id( const string& name_or_id )
{
   if( std::isdigit( name_or_id.front() ) )
   {
      try
      {
         return fc::variant(name_or_id).as<T>();
      }
      catch (const fc::exception&)
      {
      }
   }
   return optional<T>();
}

fc::ecc::private_key derive_private_key( const std::string& prefix_string,
                                         int sequence_number )
{
   std::string sequence_string = std::to_string(sequence_number);
   fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
   fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
   return derived_key;
}

string normalize_brain_key( string s )
{
   size_t i = 0, n = s.length();
   std::string result;
   char c;
   result.reserve( n );

   bool preceded_by_whitespace = false;
   bool non_empty = false;
   while( i < n )
   {
      c = s[i++];
      switch( c )
      {
      case ' ':  case '\t': case '\r': case '\n': case '\v': case '\f':
         preceded_by_whitespace = true;
         continue;

      case 'a': c = 'A'; break;
      case 'b': c = 'B'; break;
      case 'c': c = 'C'; break;
      case 'd': c = 'D'; break;
      case 'e': c = 'E'; break;
      case 'f': c = 'F'; break;
      case 'g': c = 'G'; break;
      case 'h': c = 'H'; break;
      case 'i': c = 'I'; break;
      case 'j': c = 'J'; break;
      case 'k': c = 'K'; break;
      case 'l': c = 'L'; break;
      case 'm': c = 'M'; break;
      case 'n': c = 'N'; break;
      case 'o': c = 'O'; break;
      case 'p': c = 'P'; break;
      case 'q': c = 'Q'; break;
      case 'r': c = 'R'; break;
      case 's': c = 'S'; break;
      case 't': c = 'T'; break;
      case 'u': c = 'U'; break;
      case 'v': c = 'V'; break;
      case 'w': c = 'W'; break;
      case 'x': c = 'X'; break;
      case 'y': c = 'Y'; break;
      case 'z': c = 'Z'; break;

      default:
         break;
      }
      if( preceded_by_whitespace && non_empty )
         result.push_back(' ');
      result.push_back(c);
      preceded_by_whitespace = false;
      non_empty = true;
   }
   return result;
}

struct op_prototype_visitor
{
   typedef void result_type;

   int t = 0;
   flat_map< std::string, operation >& name2op;

   op_prototype_visitor(
      int _t,
      flat_map< std::string, operation >& _prototype_ops
      ):t(_t), name2op(_prototype_ops) {}

   template<typename Type>
   result_type operator()( const Type& op )const
   {
      string name = fc::get_typename<Type>::name();
      size_t p = name.rfind(':');
      if( p != string::npos )
         name = name.substr( p+1 );
      name2op[ name ] = Type();
   }
};

class wallet_api_impl
{
public:
   api_documentation method_documentation;
private:
   void claim_registered_account(const account_object& account)
   {
      auto it = _wallet.pending_account_registrations.find( account.name );
      FC_ASSERT( it != _wallet.pending_account_registrations.end() );
      for (const std::string& wif_key : it->second)
         if( !import_key( account.name, wif_key ) )
         {
            // somebody else beat our pending registration, there is
            //    nothing we can do except log it and move on
            elog( "account ${name} registered by someone else first!",
                  ("name", account.name) );
            // might as well remove it from pending regs,
            //    because there is now no way this registration
            //    can become valid (even in the extremely rare
            //    possibility of migrating to a fork where the
            //    name is available, the user can always
            //    manually re-register)
         }
      _wallet.pending_account_registrations.erase( it );
   }

   // after a miner registration succeeds, this saves the private key in the wallet permanently
   //
   void claim_registered_miner(const std::string& miner_name)
   {
      auto iter = _wallet.pending_miner_registrations.find(miner_name);
      FC_ASSERT(iter != _wallet.pending_miner_registrations.end());
      std::string wif_key = iter->second;

      // get the list key id this key is registered with in the chain
      fc::optional<fc::ecc::private_key> miner_private_key = wif_to_key(wif_key);
      FC_ASSERT(miner_private_key);

      auto pub_key = miner_private_key->get_public_key();
      _keys[pub_key] = wif_key;
      _wallet.pending_miner_registrations.erase(iter);
   }


   fc::mutex _resync_mutex;
   void resync()
   {
      fc::scoped_lock<fc::mutex> lock(_resync_mutex);
      // this method is used to update wallet_data annotations
      //   e.g. wallet has been restarted and was not notified
      //   of events while it was down
      //
      // everything that is done "incremental style" when a push
      //   notification is received, should also be done here
      //   "batch style" by querying the blockchain

      if( !_wallet.pending_account_registrations.empty() )
      {
         // make a vector of the account names pending registration
         std::vector<string> pending_account_names = boost::copy_range<std::vector<string> >(boost::adaptors::keys(_wallet.pending_account_registrations));

         // look those up on the blockchain
         std::vector<fc::optional<graphene::chain::account_object >>
               pending_account_objects = _remote_db->lookup_account_names( pending_account_names );

         // if any of them exist, claim them
         for( const fc::optional<graphene::chain::account_object>& optional_account : pending_account_objects )
            if( optional_account )
               claim_registered_account(*optional_account);
      }

      if (!_wallet.pending_miner_registrations.empty())
      {
         // make a vector of the owner accounts for miners pending registration
         std::vector<string> pending_miner_names = boost::copy_range<std::vector<string> >(boost::adaptors::keys(_wallet.pending_miner_registrations));

         // look up the owners on the blockchain
         std::vector<fc::optional<graphene::chain::account_object>> owner_account_objects = _remote_db->lookup_account_names(pending_miner_names);

         // if any of them have registered miners, claim them
         for( const fc::optional<graphene::chain::account_object>& optional_account : owner_account_objects )
            if (optional_account)
            {
               fc::optional<miner_object> miner_obj = _remote_db->get_miner_by_account(optional_account->id);
               if (miner_obj)
                  claim_registered_miner(optional_account->name);
            }
      }

   }

   void enable_umask_protection()
   {
#ifdef __unix__
      _old_umask = umask( S_IRWXG | S_IRWXO );
#endif
   }

   void disable_umask_protection()
   {
#ifdef __unix__
      umask( _old_umask );
#endif
   }

   void init_prototype_ops()
   {
      operation op;
      for( int t=0; t<op.count(); t++ )
      {
         op.set_which( t );
         op.visit( op_prototype_visitor(t, _prototype_ops) );
      }
      return;
   }

   map<transaction_handle_type, signed_transaction> _builder_transactions;

   // if the user executes the same command twice in quick succession,
   // we might generate the same transaction id, and cause the second
   // transaction to be rejected.  This can be avoided by altering the
   // second transaction slightly (bumping up the expiration time by
   // a second).  Keep track of recent transaction ids we've generated
   // so we can know if we need to do this
   struct recently_generated_transaction_record
   {
      fc::time_point_sec generation_time;
      graphene::chain::transaction_id_type transaction_id;
   };
   struct timestamp_index{};
   typedef boost::multi_index_container<recently_generated_transaction_record,
                                        boost::multi_index::indexed_by<boost::multi_index::hashed_unique<boost::multi_index::member<recently_generated_transaction_record,
                                                                                                                                    graphene::chain::transaction_id_type,
                                                                                                                                    &recently_generated_transaction_record::transaction_id>,
                                                                                                         std::hash<graphene::chain::transaction_id_type> >,
                                                                       boost::multi_index::ordered_non_unique<boost::multi_index::tag<timestamp_index>,
                                                                                                              boost::multi_index::member<recently_generated_transaction_record, fc::time_point_sec, &recently_generated_transaction_record::generation_time> > > > recently_generated_transaction_set_type;
   recently_generated_transaction_set_type _recently_generated_transactions;

public:
   wallet_api& self;
   wallet_api_impl( wallet_api& s, const wallet_data& initial_data, fc::api<login_api> rapi )
      : self(s),
        _chain_id(initial_data.chain_id),
        _remote_api(rapi),
        _remote_db(rapi->database()),
        _remote_net_broadcast(rapi->network_broadcast()),
        _remote_hist(rapi->history())
   {
      chain_id_type remote_chain_id = _remote_db->get_chain_id();
      if( remote_chain_id != _chain_id && _chain_id != chain_id_type ("0000000000000000000000000000000000000000000000000000000000000000") )
      {
         FC_THROW( "Remote server gave us an unexpected chain_id",
            ("remote_chain_id", remote_chain_id)
            ("chain_id", _chain_id) );
      }
      init_prototype_ops();

      _remote_db->set_block_applied_callback( [this](const variant& block_id )
      {
         on_block_applied( block_id );
      } );

      _wallet.chain_id = remote_chain_id;
      _chain_id = _wallet.chain_id;
      _wallet.ws_server = initial_data.ws_server;
      _wallet.ws_user = initial_data.ws_user;
      _wallet.ws_password = initial_data.ws_password;
   }
   virtual ~wallet_api_impl()
   {
      try
      {
         _remote_db->cancel_all_subscriptions();
      }
      catch (const fc::exception& e)
      {
         // Right now the wallet_api has no way of knowing if the connection to the
         // miner has already disconnected (via the miner node exiting first).
         // If it has exited, cancel_all_subscriptsions() will throw and there's
         // nothing we can do about it.
         // dlog("Caught exception ${e} while canceling database subscriptions", ("e", e));
      }
   }

   void encrypt_keys()
   {
      if( !is_locked() )
      {
         plain_keys data;
         data.keys = _keys;
         data.checksum = _checksum;
         auto plain_txt = fc::raw::pack(data);
         _wallet.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
      }
   }

   void on_block_applied( const variant& block_id )
   {
      fc::async([this]{resync();}, "Resync after block");
   }

   bool copy_wallet_file( string destination_filename )
   {
      fc::path src_path = get_wallet_filename();
      if( !fc::exists( src_path ) )
         return false;
      fc::path dest_path = destination_filename + _wallet_filename_extension;
      int suffix = 0;
      while( fc::exists(dest_path) )
      {
         ++suffix;
         dest_path = destination_filename + "-" + to_string( suffix ) + _wallet_filename_extension;
      }
      wlog( "backing up wallet ${src} to ${dest}",
            ("src", src_path)
            ("dest", dest_path) );

      fc::path dest_parent = fc::absolute(dest_path).parent_path();
      try
      {
         enable_umask_protection();
         if( !fc::exists( dest_parent ) )
            fc::create_directories( dest_parent );
         fc::copy( src_path, dest_path );
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
      return true;
   }

   bool is_locked()const
   {
      return _checksum == fc::sha512();
   }

   template<typename T>
   T get_object(object_id<T::space_id, T::type_id, T> id)const
   {
      auto ob = _remote_db->get_objects({id}).front();
      return ob.template as<T>();
   }

   void set_operation_fees( signed_transaction& tx, const fee_schedule& s  ) const
   {
      for( auto& op : tx.operations )
         s.set_fee(op);
   }

   variant info() const
   {
      auto chain_props = get_chain_properties();
      auto global_props = get_global_properties();
      auto dynamic_props = get_dynamic_global_properties();
      fc::mutable_variant_object result;
      result["head_block_num"] = dynamic_props.head_block_number;
      result["head_block_id"] = dynamic_props.head_block_id;
      result["head_block_age"] = fc::get_approximate_relative_time_string(dynamic_props.time,
                                                                          time_point_sec(time_point::now()),
                                                                          " old");
      result["next_maintenance_time"] = fc::get_approximate_relative_time_string(dynamic_props.next_maintenance_time);
      result["chain_id"] = chain_props.chain_id;
      result["participation"] = (100*dynamic_props.recent_slots_filled.popcount()) / 128.0;
      result["active_miners"] = global_props.active_miners;
      return result;
   }

   variant_object about() const
   {
      string client_version( graphene::utilities::git_revision_description );
      const size_t pos = client_version.find( '/' );
      if( pos != string::npos && client_version.size() > pos )
         client_version = client_version.substr( pos + 1 );

      fc::mutable_variant_object result;
      //result["blockchain_name"]        = BLOCKCHAIN_NAME;
      //result["blockchain_description"] = BTS_BLOCKCHAIN_DESCRIPTION;
      result["client_version"]           = client_version;
      result["graphene_revision"]        = graphene::utilities::git_revision_sha;
      result["graphene_revision_age"]    = fc::get_approximate_relative_time_string( fc::time_point_sec( graphene::utilities::git_revision_unix_timestamp ) );
      result["fc_revision"]              = fc::git_revision_sha;
      result["fc_revision_age"]          = fc::get_approximate_relative_time_string( fc::time_point_sec( fc::git_revision_unix_timestamp ) );
      result["compile_date"]             = "compiled on " __DATE__ " at " __TIME__;
      result["boost_version"]            = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
      result["openssl_version"]          = OPENSSL_VERSION_TEXT;

      std::string bitness = boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit";
#if defined(__APPLE__)
      std::string os = "osx";
#elif defined(__linux__)
      std::string os = "linux";
#elif defined(_MSC_VER)
      std::string os = "win32";
#else
      std::string os = "other";
#endif
      result["build"] = os + " " + bitness;

      return result;
   }

   chain_property_object get_chain_properties() const
   {
      return _remote_db->get_chain_properties();
   }
   global_property_object get_global_properties() const
   {
      return _remote_db->get_global_properties();
   }
   dynamic_global_property_object get_dynamic_global_properties() const
   {
      return _remote_db->get_dynamic_global_properties();
   }
   account_object get_account(account_id_type id) const
   {
      //if( _wallet.my_accounts.get<by_id>().count(id) )
      //   return *_wallet.my_accounts.get<by_id>().find(id);
      auto rec = _remote_db->get_accounts({id}).front();
      FC_ASSERT(rec);
      return *rec;
   }
   account_object get_account(string account_name_or_id) const
   {
      FC_ASSERT( account_name_or_id.size() > 0 );

      if( auto id = maybe_id<account_id_type>(account_name_or_id) )
      {
         // It's an ID
         return get_account(*id);
      } else {
         // It's a name
         if( _wallet.my_accounts.get<by_name>().count(account_name_or_id) )
         {
            auto local_account = *_wallet.my_accounts.get<by_name>().find(account_name_or_id);
            auto blockchain_account = _remote_db->lookup_account_names({account_name_or_id}).front();
            FC_ASSERT( blockchain_account );
            if (local_account.id != blockchain_account->id)
               elog("my account id ${id} different from blockchain id ${id2}", ("id", local_account.id)("id2", blockchain_account->id));
            if (local_account.name != blockchain_account->name)
               elog("my account name ${id} different from blockchain name ${id2}", ("id", local_account.name)("id2", blockchain_account->name));

            //return *_wallet.my_accounts.get<by_name>().find(account_name_or_id);
            return *blockchain_account;
         }
         auto rec = _remote_db->lookup_account_names({account_name_or_id}).front();
         FC_ASSERT( rec && rec->name == account_name_or_id );
         return *rec;
      }
   }
   account_id_type get_account_id(string account_name_or_id) const
   {
      return get_account(account_name_or_id).get_id();
   }
   optional<asset_object> find_asset(asset_id_type id)const
   {
      auto rec = _remote_db->get_assets({id}).front();
      if( rec )
         _asset_cache[id] = *rec;
      return rec;
   }
   optional<asset_object> find_asset(string asset_symbol_or_id)const
   {
      FC_ASSERT( asset_symbol_or_id.size() > 0 );

      if( auto id = maybe_id<asset_id_type>(asset_symbol_or_id) )
      {
         // It's an ID
         return find_asset(*id);
      } else {
         // It's a symbol
         auto rec = _remote_db->lookup_asset_symbols({asset_symbol_or_id}).front();
         if( rec )
         {
            if( rec->symbol != asset_symbol_or_id )
               return optional<asset_object>();

            _asset_cache[rec->get_id()] = *rec;
         }
         return rec;
      }
   }
   asset_object get_asset(asset_id_type id)const
   {
      auto opt = find_asset(id);
      FC_ASSERT(opt);
      return *opt;
   }

   asset_object get_asset(string asset_symbol_or_id)const
   {
      auto opt = find_asset(asset_symbol_or_id);
      FC_ASSERT(opt);
      return *opt;
   }

   asset_id_type get_asset_id(string asset_symbol_or_id) const
   {
      FC_ASSERT( asset_symbol_or_id.size() > 0 );
      vector<optional<asset_object>> opt_asset;
      if( std::isdigit( asset_symbol_or_id.front() ) )
         return fc::variant(asset_symbol_or_id).as<asset_id_type>();
      opt_asset = _remote_db->lookup_asset_symbols( {asset_symbol_or_id} );
      FC_ASSERT( (opt_asset.size() > 0) && (opt_asset[0].valid()) );
      return opt_asset[0]->id;
   }

   string                            get_wallet_filename() const
   {
      return _wallet_filename;
   }

   fc::ecc::private_key              get_private_key(const public_key_type& id)const
   {
      auto it = _keys.find(id);
      FC_ASSERT( it != _keys.end() );

      fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
      FC_ASSERT( privkey );
      return *privkey;
   }

   fc::ecc::private_key get_private_key_for_account(const account_object& account)const
   {
      vector<public_key_type> active_keys = account.owner.get_keys();
      if (active_keys.size() != 1)
         FC_THROW("Expecting a simple authority with one active key");
      return get_private_key(active_keys.front());
   }

   // imports the private key into the wallet, and associate it in some way (?) with the
   // given account name.
   // @returns true if the key matches a current active/owner/memo key for the named
   //          account, false otherwise (but it is stored either way)
   bool import_key(string account_name_or_id, string wif_key)
   {
      fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
      if (!optional_private_key)
         FC_THROW("Invalid private key");
      graphene::chain::public_key_type wif_pub_key = optional_private_key->get_public_key();

      account_object account = get_account( account_name_or_id );


      // make a list of all current public keys for the named account
      flat_set<public_key_type> all_keys_for_account;
      std::vector<public_key_type> owner_keys = account.owner.get_keys();

      if( std::find( owner_keys.begin(), owner_keys.end(), wif_pub_key ) !=owner_keys.end() )
      {
         //we have the owner keys
         int active_key_index = find_first_unused_derived_key_index( *optional_private_key );
         fc::ecc::private_key active_privkey = derive_private_key( wif_key, active_key_index);

         int memo_key_index = find_first_unused_derived_key_index(active_privkey);
         fc::ecc::private_key memo_privkey = derive_private_key( key_to_wif(active_privkey), memo_key_index);

         graphene::chain::public_key_type active_pubkey = active_privkey.get_public_key();
         graphene::chain::public_key_type memo_pubkey = memo_privkey.get_public_key();
         _keys[active_pubkey] = key_to_wif( active_privkey );
         _keys[memo_pubkey] = key_to_wif( memo_privkey );

         _wallet.extra_keys[account.id].insert( active_pubkey );
         _wallet.extra_keys[account.id].insert( memo_pubkey );

      }

      std::copy(owner_keys.begin(), owner_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));

      _keys[wif_pub_key] = wif_key;

      _wallet.update_account(account);

      _wallet.extra_keys[account.id].insert(wif_pub_key);

      return all_keys_for_account.find(wif_pub_key) != all_keys_for_account.end();
   }

   bool load_wallet_file(string wallet_filename = "")
   {
      // TODO:  Merge imported wallet with existing wallet,
      //        instead of replacing it
      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;

      if( ! fc::exists( wallet_filename ) )
         return false;

      _wallet = fc::json::from_file( wallet_filename ).as< wallet_data >();
      if( _wallet.chain_id != _chain_id )
         FC_THROW( "Wallet chain ID does not match",
            ("wallet.chain_id", _wallet.chain_id)
            ("chain_id", _chain_id) );

      size_t account_pagination = 100;
      vector< account_id_type > account_ids_to_send;
      size_t n = _wallet.my_accounts.size();
      account_ids_to_send.reserve( std::min( account_pagination, n ) );
      auto it = _wallet.my_accounts.begin();

      for( size_t start=0; start<n; start+=account_pagination )
      {
         size_t end = std::min( start+account_pagination, n );
         assert( end > start );
         account_ids_to_send.clear();
         std::vector< account_object > old_accounts;
         for( size_t i=start; i<end; i++ )
         {
            assert( it != _wallet.my_accounts.end() );
            old_accounts.push_back( *it );
            account_ids_to_send.push_back( old_accounts.back().id );
            ++it;
         }
         std::vector< optional< account_object > > accounts = _remote_db->get_accounts(account_ids_to_send);
         // server response should be same length as request
         FC_ASSERT( accounts.size() == account_ids_to_send.size() );
         size_t i = 0;
         for( const optional< account_object >& acct : accounts )
         {
            account_object& old_acct = old_accounts[i];
            if( !acct.valid() )
            {
               elog( "Could not find account ${id} : \"${name}\" does not exist on the chain!", ("id", old_acct.id)("name", old_acct.name) );
               i++;
               continue;
            }
            // this check makes sure the server didn't send results
            // in a different order, or accounts we didn't request
            FC_ASSERT( acct->id == old_acct.id );
            if( fc::json::to_string(*acct) != fc::json::to_string(old_acct) )
            {
               wlog( "Account ${id} : \"${name}\" updated on chain", ("id", acct->id)("name", acct->name) );
            }
            _wallet.update_account( *acct );
            i++;
         }
      }

      return true;
   }
   void save_wallet_file(string wallet_filename = "")
   {
      //
      // Serialize in memory, then save to disk
      //
      // This approach lessens the risk of a partially written wallet
      // if exceptions are thrown in serialization
      //

      encrypt_keys();

      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;

      wlog( "saving wallet to file ${fn}", ("fn", wallet_filename) );

      string data = fc::json::to_pretty_string( _wallet );
      try
      {
         enable_umask_protection();
         //
         // Parentheses on the following declaration fails to compile,
         // due to the Most Vexing Parse.  Thanks, C++
         //
         // http://en.wikipedia.org/wiki/Most_vexing_parse
         //
         fc::ofstream outfile{ fc::path( wallet_filename ) };
         outfile.write( data.c_str(), data.length() );
         outfile.flush();
         outfile.close();
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
   }


   // This function generates derived keys starting with index 0 and keeps incrementing
   // the index until it finds a key that isn't registered in the block chain.  To be
   // safer, it continues checking for a few more keys to make sure there wasn't a short gap
   // caused by a failed registration or the like.
   int find_first_unused_derived_key_index(const fc::ecc::private_key& parent_key)
   {
      int first_unused_index = 0;
      int number_of_consecutive_unused_keys = 0;
      for (int key_index = 0; ; ++key_index)
      {
         fc::ecc::private_key derived_private_key = derive_private_key(key_to_wif(parent_key), key_index);
         graphene::chain::public_key_type derived_public_key = derived_private_key.get_public_key();
         if( _keys.find(derived_public_key) == _keys.end() )
         {
            if (number_of_consecutive_unused_keys)
            {
               ++number_of_consecutive_unused_keys;
               if (number_of_consecutive_unused_keys > 5)
                  return first_unused_index;
            }
            else
            {
               first_unused_index = key_index;
               number_of_consecutive_unused_keys = 1;
            }
         }
         else
         {
            // key_index is used
            first_unused_index = 0;
            number_of_consecutive_unused_keys = 0;
         }
      }
   }

   signed_transaction create_account_with_public_key(graphene::chain::public_key_type const& owner_pubkey,
                                                     string const& registrar_account)
   { try {
         account_create_operation account_create_op;

         // TODO:  process when pay_from_account is ID

         account_object registrar_account_object = get_account( registrar_account );

         account_id_type registrar_account_id = registrar_account_object.id;

         account_create_op.registrar = registrar_account_id;
         account_create_op.owner = authority(1, owner_pubkey, 1);

         //account_create_op.active = authority(1, owner_pubkey, 1);
         //account_create_op.options.memo_key = owner_pubkey;


         // current_fee_schedule()
         // find_account(pay_from_account)

         // account_create_op.fee = account_create_op.calculate_fee(db.current_fee_schedule());

         signed_transaction tx;

         tx.operations.push_back( account_create_op );

         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);

         vector<public_key_type> paying_keys = registrar_account_object.owner.get_keys();

         auto dyn_props = get_dynamic_global_properties();
         tx.set_reference_block( dyn_props.head_block_id );
         tx.set_expiration( dyn_props.time + fc::seconds(30) );
         tx.validate();

         for( public_key_type& key : paying_keys )
         {
            auto it = _keys.find(key);
            if( it != _keys.end() )
            {
               fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
               FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
               tx.sign( *privkey, _chain_id );
            }
         }

         return tx;
   } FC_CAPTURE_AND_RETHROW( (registrar_account) ) }


   signed_transaction create_account_with_private_key(fc::ecc::private_key owner_privkey,
                                                      string const& registrar_account,
                                                      bool import,
                                                      bool broadcast = false,
                                                      bool save_wallet = true)
   { try {
         int active_key_index = find_first_unused_derived_key_index(owner_privkey);
         fc::ecc::private_key active_privkey = derive_private_key( key_to_wif(owner_privkey), active_key_index);

         //int memo_key_index = find_first_unused_derived_key_index(active_privkey);
         //fc::ecc::private_key memo_privkey = derive_private_key( key_to_wif(active_privkey), memo_key_index);

         graphene::chain::public_key_type owner_pubkey = owner_privkey.get_public_key();
         //graphene::chain::public_key_type active_pubkey = active_privkey.get_public_key();
         //graphene::chain::public_key_type memo_pubkey = memo_privkey.get_public_key();

         std::string account_name(owner_pubkey);
         signed_transaction tx = create_account_with_public_key(owner_pubkey,
                                                                registrar_account);

         if (import)
         {
            _wallet.pending_account_registrations[account_name].push_back(key_to_wif( owner_privkey ));
         }
         if( save_wallet )
            save_wallet_file();
         if( broadcast )
            _remote_net_broadcast->broadcast_transaction( tx );
         return tx;
   } FC_CAPTURE_AND_RETHROW( (registrar_account)(broadcast) ) }

   signed_transaction create_account_with_brain_key(string const& brain_key,
                                                    string const& registrar_account,
                                                    bool import,
                                                    bool broadcast = false,
                                                    bool save_wallet = true)
   { try {

      FC_ASSERT( !self.is_locked() );
      string normalized_brain_key = normalize_brain_key( brain_key );
      // TODO:  scan blockchain for accounts that exist with same brain key
      fc::ecc::private_key owner_privkey = derive_private_key( normalized_brain_key, 0 );
      return create_account_with_private_key(owner_privkey, registrar_account, import, broadcast, save_wallet);
   } FC_CAPTURE_AND_RETHROW( (registrar_account) ) }

   vector< vesting_balance_object_with_info > get_vesting_balances( string account_name )
   { try {
      fc::optional<vesting_balance_id_type> vbid = maybe_id<vesting_balance_id_type>( account_name );
      std::vector<vesting_balance_object_with_info> result;
      fc::time_point_sec now = _remote_db->get_dynamic_global_properties().time;

      if( vbid )
      {
         result.emplace_back( get_object<vesting_balance_object>(*vbid), now );
         return result;
      }

      // try casting to avoid a round-trip if we were given an account ID
      fc::optional<account_id_type> acct_id = maybe_id<account_id_type>( account_name );
      if( !acct_id )
         acct_id = get_account( account_name ).id;

      vector< vesting_balance_object > vbos = _remote_db->get_vesting_balances( *acct_id );
      if( vbos.size() == 0 )
         return result;

      for( const vesting_balance_object& vbo : vbos )
         result.emplace_back( vbo, now );

      return result;
   } FC_CAPTURE_AND_RETHROW( (account_name) )
   }

   void cook_transaction(signed_transaction & tx)
   {
       auto dyn_props = get_dynamic_global_properties();
       tx.set_reference_block( dyn_props.head_block_id );

       // first, some bookkeeping, expire old items from _recently_generated_transactions
       // since transactions include the head block id, we just need the index for keeping transactions unique
       // when there are multiple transactions in the same block.  choose a time period that should be at
       // least one block long, even in the worst case.  2 minutes ought to be plenty.
       fc::time_point_sec oldest_transaction_ids_to_track(dyn_props.time - fc::minutes(2));
       auto oldest_transaction_record_iter = _recently_generated_transactions.get<timestamp_index>().lower_bound(oldest_transaction_ids_to_track);
       auto begin_iter = _recently_generated_transactions.get<timestamp_index>().begin();
       _recently_generated_transactions.get<timestamp_index>().erase(begin_iter, oldest_transaction_record_iter);

       uint32_t expiration_time_offset = 0;
       for (;;)
       {
           tx.set_expiration( dyn_props.time + fc::seconds(30 + expiration_time_offset) );

           graphene::chain::transaction_id_type this_transaction_id = tx.id();
           auto iter = _recently_generated_transactions.find(this_transaction_id);
           if (iter == _recently_generated_transactions.end())
           {
               // we haven't generated this transaction before, the usual case
               recently_generated_transaction_record this_transaction_record;
               this_transaction_record.generation_time = dyn_props.time;
               this_transaction_record.transaction_id = this_transaction_id;
               _recently_generated_transactions.insert(this_transaction_record);
               break;
           }

           // else we've generated a dupe, increment expiration time and re-sign it
           ++expiration_time_offset;
       }
   }

   signed_transaction serve_cooked_transaction(signed_transaction const & tx)
   {
       try
       {
           elog("about to broadcast tx: ${t}", ("t", tx));
           _remote_net_broadcast->broadcast_transaction( tx );
       }
       catch (const fc::exception& e)
       {
           elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()) );
           throw;
       }
       return tx;
   }


   signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false)
   {
      flat_set<account_id_type> req_active_approvals;
      flat_set<account_id_type> req_owner_approvals;
      vector<authority>         other_auths;

      tx.get_required_authorities( req_active_approvals, req_owner_approvals, other_auths );

      for( const auto& auth : other_auths )
         for( const auto& a : auth.account_auths )
            req_active_approvals.insert(a.first);

      // std::merge lets us de-duplicate account_id's that occur in both
      //   sets, and dump them into a vector (as required by remote_db api)
      //   at the same time
      vector<account_id_type> v_approving_account_ids;
      std::merge(req_active_approvals.begin(), req_active_approvals.end(),
                 req_owner_approvals.begin() , req_owner_approvals.end(),
                 std::back_inserter(v_approving_account_ids));

      /// TODO: fetch the accounts specified via other_auths as well.

      vector< optional<account_object> > approving_account_objects =
            _remote_db->get_accounts( v_approving_account_ids );

      /// TODO: recursively check one layer deeper in the authority tree for keys

      FC_ASSERT( approving_account_objects.size() == v_approving_account_ids.size() );

      flat_map<account_id_type, account_object*> approving_account_lut;
      size_t i = 0;
      for( optional<account_object>& approving_acct : approving_account_objects )
      {
         if( !approving_acct.valid() )
         {
            wlog( "operation_get_required_auths said approval of non-existing account ${id} was needed",
                  ("id", v_approving_account_ids[i]) );
            i++;
            continue;
         }
         approving_account_lut[ approving_acct->id ] = &(*approving_acct);
         i++;
      }

      flat_set<public_key_type> approving_key_set;
      for( account_id_type& acct_id : req_active_approvals )
      {
         const auto it = approving_account_lut.find( acct_id );
         if( it == approving_account_lut.end() )
            continue;
         const account_object* acct = it->second;
         vector<public_key_type> v_approving_keys = acct->owner.get_keys();
         for( const public_key_type& approving_key : v_approving_keys )
            approving_key_set.insert( approving_key );
      }
      for( account_id_type& acct_id : req_owner_approvals )
      {
         const auto it = approving_account_lut.find( acct_id );
         if( it == approving_account_lut.end() )
            continue;
         const account_object* acct = it->second;
         vector<public_key_type> v_approving_keys = acct->owner.get_keys();
         for( const public_key_type& approving_key : v_approving_keys )
            approving_key_set.insert( approving_key );
      }
      for( const authority& a : other_auths )
      {
         for( const auto& k : a.key_auths )
            approving_key_set.insert( k.first );
      }

      auto dyn_props = get_dynamic_global_properties();
      tx.set_reference_block( dyn_props.head_block_id );

      // first, some bookkeeping, expire old items from _recently_generated_transactions
      // since transactions include the head block id, we just need the index for keeping transactions unique
      // when there are multiple transactions in the same block.  choose a time period that should be at
      // least one block long, even in the worst case.  2 minutes ought to be plenty.
      fc::time_point_sec oldest_transaction_ids_to_track(dyn_props.time - fc::minutes(2));
      auto oldest_transaction_record_iter = _recently_generated_transactions.get<timestamp_index>().lower_bound(oldest_transaction_ids_to_track);
      auto begin_iter = _recently_generated_transactions.get<timestamp_index>().begin();
      _recently_generated_transactions.get<timestamp_index>().erase(begin_iter, oldest_transaction_record_iter);

      uint32_t expiration_time_offset = 0;
      for (;;)
      {
         tx.set_expiration( dyn_props.time + fc::seconds(30 + expiration_time_offset) );
         tx.signatures.clear();

         for( public_key_type& key : approving_key_set )
         {
            auto it = _keys.find(key);
            if( it != _keys.end() )
            {
               fc::optional<fc::ecc::private_key> privkey = wif_to_key( it->second );
               FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
               tx.sign( *privkey, _chain_id );
            }
            /// TODO: if transaction has enough signatures to be "valid" don't add any more,
            /// there are cases where the wallet may have more keys than strictly necessary and
            /// the transaction will be rejected if the transaction validates without requiring
            /// all signatures provided
         }

         graphene::chain::transaction_id_type this_transaction_id = tx.id();
         auto iter = _recently_generated_transactions.find(this_transaction_id);
         if (iter == _recently_generated_transactions.end())
         {
            // we haven't generated this transaction before, the usual case
            recently_generated_transaction_record this_transaction_record;
            this_transaction_record.generation_time = dyn_props.time;
            this_transaction_record.transaction_id = this_transaction_id;
            _recently_generated_transactions.insert(this_transaction_record);
            break;
         }

         // else we've generated a dupe, increment expiration time and re-sign it
         ++expiration_time_offset;
      }

      if( broadcast )
      {
         try
         {
            elog("about to broadcast tx: ${t}", ("t", tx));
            _remote_net_broadcast->broadcast_transaction( tx );
         }
         catch (const fc::exception& e)
         {
            elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()) );
            throw;
         }
      }

      return tx;
   }


   signed_transaction transfer(string from, string to, string amount,
                               string asset_symbol, string memo, bool broadcast = false)
   { try {
      FC_ASSERT( !self.is_locked() );
      fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
      FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

      account_object from_account = get_account(from);
      public_key_type to_key(to);
      account_id_type from_id = from_account.id;

      transfer_operation xfer_op;

      xfer_op.from = from_id;
      xfer_op.to = to_key;
      xfer_op.amount = asset_obj->amount_from_string(amount);

      auto from_keys = from_account.owner.get_keys();
      FC_ASSERT(from_keys.empty() == false);

      if( memo.size() )
         {
            xfer_op.memo = memo_data();
            xfer_op.memo->from = from_keys.front();
            xfer_op.memo->to = to_key;
            xfer_op.memo->set_message(get_private_key(from_keys.front()),
                                      to_key, memo);
         }

      signed_transaction tx;
      tx.operations.push_back(xfer_op);
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   } FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(asset_symbol)(memo)(broadcast) ) }

   signed_transaction set_transfer_freeze_block(string const& announcer,
                                                uint64_t block_num,
                                                bool broadcast = false)
   { try {
      FC_ASSERT( !self.is_locked() );

      account_object announcer_account = get_account(announcer);

      set_transfer_freeze_block_operation freeze_op;

      freeze_op.announcer = announcer_account.id;
      freeze_op.block_num = block_num;

      signed_transaction tx;
      tx.operations.push_back(freeze_op);
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   } FC_CAPTURE_AND_RETHROW( (announcer)(block_num)(broadcast) ) }

   std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const
   {
      std::map<string,std::function<string(fc::variant,const fc::variants&)> > m;
      m["help"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      m["gethelp"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };
/*
      m["list_account_balances"] = [this](variant result, const fc::variants& a)
      {
         auto r = result.as<vector<asset>>();
         vector<asset_object> asset_recs;
         std::transform(r.begin(), r.end(), std::back_inserter(asset_recs), [this](const asset& a) {
            return get_asset(a.asset_id);
         });

         std::stringstream ss;
         for( unsigned i = 0; i < asset_recs.size(); ++i )
            ss << asset_recs[i].amount_to_pretty_string(r[i]) << "\n";

         return ss.str();
      };
*/
      m["get_order_book"] = [this](variant result, const fc::variants& a)
      {
         auto orders = result.as<order_book>();
         auto bids = orders.bids;
         auto asks = orders.asks;
         std::stringstream ss;
         std::stringstream sum_stream;
         sum_stream << "Sum(" << orders.base << ')';
         double bid_sum = 0;
         double ask_sum = 0;
         const int spacing = 20;

         auto prettify_num = [&]( double n )
         {
            //ss << n;
            if (abs( round( n ) - n ) < 0.00000000001 )
            {
               //ss << setiosflags( !ios::fixed ) << (int) n;     // doesn't compile on Linux with gcc
               ss << (int) n;
            }
            else if (n - floor(n) < 0.000001)
            {
               ss << setiosflags( ios::fixed ) << setprecision(10) << n;
            }
            else
            {
               ss << setiosflags( ios::fixed ) << setprecision(6) << n;
            }
         };

         ss << setprecision( 8 ) << setiosflags( ios::fixed ) << setiosflags( ios::left );

         ss << ' ' << setw( (spacing * 4) + 6 ) << "BUY ORDERS" << "SELL ORDERS\n"
            << ' ' << setw( spacing + 1 ) << "Price" << setw( spacing ) << orders.quote << ' ' << setw( spacing )
            << orders.base << ' ' << setw( spacing ) << sum_stream.str()
            << "   " << setw( spacing + 1 ) << "Price" << setw( spacing ) << orders.quote << ' ' << setw( spacing )
            << orders.base << ' ' << setw( spacing ) << sum_stream.str()
            << "\n"
            << "|=====================================================================================\n";

         for (int i = 0; i < bids.size() || i < asks.size() ; i++)
         {
            if ( i < bids.size() )
            {
                bid_sum += bids[i].base;
                ss << ' ' << setw( spacing );
                prettify_num( bids[i].price );
                ss << ' ' << setw( spacing );
                prettify_num( bids[i].quote );
                ss << ' ' << setw( spacing );
                prettify_num( bids[i].base );
                ss << ' ' << setw( spacing );
                prettify_num( bid_sum );
                ss << ' ';
            }
            else
            {
                ss << setw( (spacing * 4) + 5 ) << ' ';
            }

            ss << '|';

            if ( i < asks.size() )
            {
               ask_sum += asks[i].base;
               ss << ' ' << setw( spacing );
               prettify_num( asks[i].price );
               ss << ' ' << setw( spacing );
               prettify_num( asks[i].quote );
               ss << ' ' << setw( spacing );
               prettify_num( asks[i].base );
               ss << ' ' << setw( spacing );
               prettify_num( ask_sum );
            }

            ss << '\n';
         }

         ss << endl
            << "Buy Total:  " << bid_sum << ' ' << orders.base << endl
            << "Sell Total: " << ask_sum << ' ' << orders.base << endl;

         return ss.str();
      };

      return m;
   }

   signed_transaction propose_fee_change(
      const string& proposing_account,
      fc::time_point_sec expiration_time,
      const variant_object& changed_fees,
      bool broadcast = false)
   {
      const chain_parameters& current_params = get_global_properties().parameters;
      const fee_schedule_type& current_fees = *(current_params.current_fees);

      flat_map< int, fee_parameters > fee_map;
      fee_map.reserve( current_fees.parameters.size() );
      for( const fee_parameters& op_fee : current_fees.parameters )
         fee_map[ op_fee.which() ] = op_fee;
      uint32_t scale = current_fees.scale;

      for( const auto& item : changed_fees )
      {
         const string& key = item.key();
         if( key == "scale" )
         {
            int64_t _scale = item.value().as_int64();
            FC_ASSERT( _scale >= 0 );
            FC_ASSERT( _scale <= std::numeric_limits<uint32_t>::max() );
            scale = uint32_t( _scale );
            continue;
         }
         // is key a number?
         auto is_numeric = [&]() -> bool
         {
            size_t n = key.size();
            for( size_t i=0; i<n; i++ )
            {
               if( !isdigit( key[i] ) )
                  return false;
            }
            return true;
         };

         int which;
         if( is_numeric() )
            which = std::stoi( key );
         else
         {
            const auto& n2w = _operation_which_map.name_to_which;
            auto it = n2w.find( key );
            FC_ASSERT( it != n2w.end(), "unknown operation" );
            which = it->second;
         }

         fee_parameters fp = from_which_variant< fee_parameters >( which, item.value() );
         fee_map[ which ] = fp;
      }

      fee_schedule_type new_fees;

      for( const std::pair< int, fee_parameters >& item : fee_map )
         new_fees.parameters.insert( item.second );
      new_fees.scale = scale;

      chain_parameters new_params = current_params;
      new_params.current_fees = new_fees;

      miner_update_global_parameters_operation update_op;
      update_op.new_parameters = new_params;

      proposal_create_operation prop_op;

      prop_op.expiration_time = expiration_time;
      prop_op.review_period_seconds = current_params.miner_proposal_review_period;
      prop_op.fee_paying_account = get_account(proposing_account).id;

      prop_op.proposed_ops.emplace_back( update_op );
      current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

      signed_transaction tx;
      tx.operations.push_back(prop_op);
      set_operation_fees(tx, current_params.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   }

   signed_transaction approve_proposal(
      const string& fee_paying_account,
      const string& proposal_id,
      const approval_delta& delta,
      bool broadcast = false)
   {
      proposal_update_operation update_op;

      update_op.fee_paying_account = get_account(fee_paying_account).id;
      update_op.proposal = fc::variant(proposal_id).as<proposal_id_type>();
      // make sure the proposal exists
      get_object( update_op.proposal );

      for( const std::string& name : delta.active_approvals_to_add )
         update_op.active_approvals_to_add.insert( get_account( name ).id );
      for( const std::string& name : delta.active_approvals_to_remove )
         update_op.active_approvals_to_remove.insert( get_account( name ).id );
      for( const std::string& name : delta.owner_approvals_to_add )
         update_op.owner_approvals_to_add.insert( get_account( name ).id );
      for( const std::string& name : delta.owner_approvals_to_remove )
         update_op.owner_approvals_to_remove.insert( get_account( name ).id );
      for( const std::string& k : delta.key_approvals_to_add )
         update_op.key_approvals_to_add.insert( public_key_type( k ) );
      for( const std::string& k : delta.key_approvals_to_remove )
         update_op.key_approvals_to_remove.insert( public_key_type( k ) );

      signed_transaction tx;
      tx.operations.push_back(update_op);
      set_operation_fees(tx, get_global_properties().parameters.current_fees);
      tx.validate();
      return sign_transaction(tx, broadcast);
   }

   void dbg_push_blocks( const std::string& src_filename, uint32_t count )
   {
      use_debug_api();
      (*_remote_debug)->debug_push_blocks( src_filename, count );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void dbg_generate_blocks( const std::string& debug_wif_key, uint32_t count )
   {
      use_debug_api();
      (*_remote_debug)->debug_generate_blocks( debug_wif_key, count );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void dbg_stream_json_objects( const std::string& filename )
   {
      use_debug_api();
      (*_remote_debug)->debug_stream_json_objects( filename );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void dbg_update_object( const fc::variant_object& update )
   {
      use_debug_api();
      (*_remote_debug)->debug_update_object( update );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void use_network_node_api()
   {
      if( _remote_net_node )
         return;
      try
      {
         _remote_net_node = _remote_api->network_node();
      }
      catch( const fc::exception& e )
      {
         std::cerr << "\nCouldn't get network node API.  You probably are not configured\n"
         "to access the network API on the cyvad you are\n"
         "connecting to.  Please follow the instructions in README.md to set up an apiaccess file.\n"
         "\n";
         throw(e);
      }
   }

   void use_debug_api()
   {
      if( _remote_debug )
         return;
      try
      {
        _remote_debug = _remote_api->debug();
      }
      catch( const fc::exception& e )
      {
         std::cerr << "\nCouldn't get debug node API.  You probably are not configured\n"
         "to access the debug API on the node you are connecting to.\n"
         "\n"
         "To fix this problem:\n"
         "- Please ensure you are running debug_node, not cyvad.\n"
         "- Please follow the instructions in README.md to set up an apiaccess file.\n"
         "\n";
      }
   }

   void network_add_nodes( const vector<string>& nodes )
   {
      use_network_node_api();
      for( const string& node_address : nodes )
      {
         (*_remote_net_node)->add_node( fc::ip::endpoint::from_string( node_address ) );
      }
   }

   vector< variant > network_get_connected_peers()
   {
      use_network_node_api();
      const auto peers = (*_remote_net_node)->get_connected_peers();
      vector< variant > result;
      result.reserve( peers.size() );
      for( const auto& peer : peers )
      {
         variant v;
         fc::to_variant( peer, v );
         result.push_back( v );
      }
      return result;
   }

   void flood_network(string prefix, uint32_t number_of_transactions)
   {
      try
      {
         const account_object& master = *_wallet.my_accounts.get<by_name>().lower_bound("cyva");
         int number_of_accounts = number_of_transactions / 2;
         number_of_transactions -= number_of_accounts;
         //auto key = derive_private_key("floodshill", 0);

         fc::time_point start = fc::time_point::now();
         for( int i = 0; i < number_of_accounts; ++i )
         {
            std::ostringstream brain_key;
            brain_key << "brain key for account " << prefix << i;

            graphene::chain::account_details details;
            signed_transaction trx = create_account_with_brain_key(brain_key.str(), prefix + fc::to_string(i),
                                                                   /* import = */ true,
                                                                   /* broadcast = */ true,
                                                                   /* save wallet = */ false);
         }
         fc::time_point end = fc::time_point::now();
         ilog("Created ${n} accounts in ${time} milliseconds",
              ("n", number_of_accounts)("time", (end - start).count() / 1000));

         start = fc::time_point::now();
         for( int i = 0; i < number_of_accounts; ++i )
         {
            signed_transaction trx = transfer(master.name, prefix + fc::to_string(i), "0.1", "CVA", "", true);
            trx = transfer(master.name, prefix + fc::to_string(i), "0.09", "CVA", "", true);
         }
         end = fc::time_point::now();
         ilog("Transferred to ${n} accounts in ${time} milliseconds",
              ("n", number_of_accounts*2)("time", (end - start).count() / 1000));


      }
      catch (...)
      {
         throw;
      }

   }

   operation get_prototype_operation( string operation_name )
   {
      auto it = _prototype_ops.find( operation_name );
      if( it == _prototype_ops.end() )
         FC_THROW("Unsupported operation: \"${operation_name}\"", ("operation_name", operation_name));
      return it->second;
   }

   fc::time_point_sec head_block_time() const
   {
      return _remote_db->head_block_time();
   }

   optional<signed_block> get_head_block()const
   {
      return _remote_db->get_head_block();
   }

   optional<signed_block> get_nearest_block(const string& time_iso)const
   {
      return _remote_db->get_nearest_block(time_iso);
   }

   string                  _wallet_filename;
   wallet_data             _wallet;

   map<public_key_type,string> _keys;
   fc::sha512                  _checksum;

   chain_id_type           _chain_id;
   fc::api<login_api>      _remote_api;
   fc::api<database_api>   _remote_db;
   fc::api<network_broadcast_api>   _remote_net_broadcast;
   fc::api<history_api>    _remote_hist;
   optional< fc::api<network_node_api> > _remote_net_node;
   optional< fc::api<graphene::debug_miner::debug_api> > _remote_debug;

   flat_map<string, operation> _prototype_ops;

   static_variant_map _operation_which_map = create_static_variant_map< operation >();

#ifdef __unix__
   mode_t                  _old_umask;
#endif
   const string _wallet_filename_extension = ".wallet";

   mutable map<asset_id_type, asset_object> _asset_cache;

};

   std::string operation_printer::fee(const asset& a)const {
      out << "   (Fee: " << wallet.get_asset(a.asset_id).amount_to_pretty_string(a) << ")";
      return "";
   }

   template<typename T>
   std::string operation_printer::operator()(const T& op)const
   {
      //balance_accumulator acc;
      //op.get_balance_delta( acc, result );
      auto a = wallet.get_asset( op.fee.asset_id );
      auto payer = wallet.get_account( op.fee_payer() );

      string op_name = fc::get_typename<T>::name();
      if( op_name.find_last_of(':') != string::npos )
         op_name.erase(0, op_name.find_last_of(':')+1);
      out << op_name <<" ";
     // out << "balance delta: " << fc::json::to_string(acc.balance) <<"   ";
      out << payer.name << " fee: " << a.amount_to_pretty_string( op.fee );
      operation_result_printer rprinter(wallet);
      std::string str_result = result.visit(rprinter);
      if( str_result != "" )
      {
         out << "   result: " << str_result;
      }
      return "";
   }

   string operation_printer::operator()(const transfer_operation& op) const
   {
      const auto& from_account = wallet.get_account(op.from);

      out << "Transfer " << wallet.get_asset(op.amount.asset_id).amount_to_pretty_string(op.amount)
          << " from " << from_account.name << " to " << std::string(op.to);
      std::string memo;
      if( op.memo )
      {
         if( wallet.is_locked() )
         {
            out << " -- Unlock wallet to see memo.";
         } else {
            try {

               FC_ASSERT(wallet._keys.count(op.memo->to) || wallet._keys.count(op.memo->from), "Memo is encrypted to a key ${to} or ${from} not in this wallet.", ("to", op.memo->to)("from",op.memo->from));
               if( wallet._keys.count(op.memo->to) ) {
                  vector<fc::ecc::private_key> keys_to_try_to;
                  auto my_memo_key = wif_to_key(wallet._keys.at(op.memo->to));

                  FC_ASSERT(my_memo_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
                  keys_to_try_to.push_back(*my_memo_key);
                  auto key_auths = authority(1, op.to, 1).key_auths;
                  for( auto k: key_auths ) {
                     auto my_key = wif_to_key(wallet._keys.at(k.first));
                     if(my_key)
                        keys_to_try_to.push_back(*my_key);
                  }
                  /*for( auto k: to_account.owner.key_auths ) {
                     auto my_key = wif_to_key(wallet._keys.at(k.first));
                     if(my_key)
                        keys_to_try_to.push_back(*my_key);
                  }*/

                  bool found = false;
                  for( auto k : keys_to_try_to ){
                     try{
                           memo = op.memo->get_message(k, op.memo->from);
                           out << " -- Memo: " << memo;
                           found = true;
                           break;

                     }catch(...){}
                  }
                  if(!found){
                     vector<public_key_type> keys_to_try_from;
                     for( auto k : from_account.owner.key_auths ){
                        keys_to_try_from.push_back(k.first);
                     }
                     for( auto k : keys_to_try_from ){
                        try{
                           memo = op.memo->get_message(*my_memo_key, k);
                           out << " -- Memo: " << memo;
                           found = true;
                           break;
                        }catch(...){}
                     }
                  }
                  FC_ASSERT(found);

               } else {
                  vector<fc::ecc::private_key> keys_to_try_from;
                  auto my_memo_key = wif_to_key(wallet._keys.at(op.memo->from));

                  FC_ASSERT(my_memo_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
                  keys_to_try_from.push_back(*my_memo_key);
                  for( auto k: from_account.owner.key_auths ) {
                     auto my_key = wif_to_key(wallet._keys.at(k.first));
                     if(my_key)
                        keys_to_try_from.push_back(*my_key);
                  }

                  bool found = false;
                  for( auto k : keys_to_try_from ){
                     try{
                        memo = op.memo->get_message(k, op.memo->to);
                        out << " -- Memo: " << memo;
                        found = true;
                        break;
                     }catch(...){}
                  }
                  if(!found) {
                      auto key_auths = authority(1, op.to, 1).key_auths;
                     vector<public_key_type> keys_to_try_to;
                     for( auto k : key_auths ) {
                        keys_to_try_to.push_back(k.first);
                     }
                     /*for( auto k : to_account.owner.key_auths ) {
                        keys_to_try_to.push_back(k.first);
                     }*/
                     for( auto k : keys_to_try_to ) {
                        try {
                           memo = op.memo->get_message(*my_memo_key, k);
                           out << " -- Memo: " << memo;
                           found = true;
                           break;

                        } catch( ... ) {}
                     }
                  }
               }
            } catch (const fc::exception& e) {
               out << " -- could not decrypt memo";
               elog("Error when decrypting memo: ${e}", ("e", e.to_detail_string()));
            }
         }
      }
      fee(op.fee);
      return memo;
   }


   std::string operation_printer::operator()(const account_create_operation& op) const
   {
       auto keys = op.owner.get_keys();
       FC_ASSERT(keys.empty() == false);

       std::string to_name(keys.front());
      out << "Create Account '" << to_name << "'";
      return fee(op.fee);
   }

   std::string operation_printer::operator()(const account_update_operation& op) const
   {
      out << "Update Account '" << wallet.get_account(op.account).name << "'";
      return fee(op.fee);
   }


   std::string operation_result_printer::operator()(const void_result& x) const
   {
      return "";
   }

   std::string operation_result_printer::operator()(const object_id_type& oid)
   {
      return std::string(oid);
   }

   std::string operation_result_printer::operator()(const asset& a)
   {
      return _wallet.get_asset(a.asset_id).amount_to_pretty_string(a);
   }


   }}}





   namespace graphene { namespace wallet {

   wallet_api::wallet_api(const wallet_data& initial_data, fc::api<login_api> rapi)
      : my(new detail::wallet_api_impl(*this, initial_data, rapi))
   {
   }

   wallet_api::~wallet_api()
   {
   }

   bool wallet_api::copy_wallet_file(string destination_filename)
   {
      return my->copy_wallet_file(destination_filename);
   }

   optional<signed_block_with_info> wallet_api::get_block(uint32_t num) const
   {
      optional<signed_block_with_info> result = my->_remote_db->get_block(num);

      return result;
   }

   vector<pair<uint32_t, signed_block_with_info>> wallet_api::get_blocks_starting_from(uint32_t block_num, uint32_t count, bool skip_empty)const
   {
       vector<pair<uint32_t, signed_block_with_info>> result;
       result.reserve(std::min(count, uint32_t(10000)));

       size_t index = block_num;
       while (true)
       {
          auto op_block = get_block(index);
          ++index;
          if (false == op_block.valid() ||
             result.size() == count)
              break;
          if (op_block->transactions.empty() &&
             skip_empty)
              continue;

          result.push_back(std::make_pair(index - 1, *op_block));
       }

       return result;
   }

   vector<asset> wallet_api::get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const
   {
       return my->_remote_db->get_account_balances(id, assets);
   }

   uint64_t wallet_api::get_account_count() const
   {
      return my->_remote_db->get_account_count();
   }

   vector<account_object> wallet_api::list_my_accounts()
   {
      return vector<account_object>(my->_wallet.my_accounts.begin(), my->_wallet.my_accounts.end());
   }

   map<string,account_id_type> wallet_api::list_accounts(const string& lowerbound, uint32_t limit)
   {
       return my->_remote_db->lookup_accounts(lowerbound, limit);
   }

   vector<account_object> wallet_api::search_accounts(const string& term, const string& order, const string& id, uint32_t limit)
   {
      return my->_remote_db->search_accounts(term, order, object_id_type(id), limit);
   }

   signed_transaction wallet_api::update_account_meta(string const& account_id, bool broadcast)
   {
      account_update_operation account_update_op;
      account_update_op.account = get_account(account_id).get_id();

      signed_transaction tx;
      tx.operations.push_back( account_update_op );
      my->set_operation_fees( tx, my->_remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   }

   vector<asset> wallet_api::list_account_balances(const string& id)
   {
      if( auto real_id = detail::maybe_id<account_id_type>(id) )
         return my->_remote_db->get_account_balances(*real_id, flat_set<asset_id_type>());
      return my->_remote_db->get_account_balances(get_account(id).id, flat_set<asset_id_type>());
   }

   vector<asset_object> wallet_api::list_assets(const string& lowerbound, uint32_t limit)const
   {
      return my->_remote_db->list_assets( lowerbound, limit );
   }

   vector<operation_detail> wallet_api::get_account_history(string name, int limit)const
   {
      vector<operation_detail> result;
      auto account_id = get_account(name).get_id();

      while( limit > 0 )
      {
         operation_history_id_type start;
         if( result.size() )
         {
            start = result.back().op.id;
            start = start + 1;
         }


         vector<operation_history_object> current = my->_remote_hist->get_account_history(account_id, "", operation_history_id_type(), std::min(100,limit), start);
         for( auto& o : current ) {
            std::stringstream ss;
            auto memo = o.op.visit(detail::operation_printer(ss, *my, o.result));
            result.push_back( operation_detail{ memo, ss.str(), o } );
         }
         if( current.size() < std::min(100,limit) )
            break;
         limit -= current.size();
      }

      return result;
   }

   vector<transaction_detail_object_ex> wallet_api::search_account_history(string const& account_name,
                                                                           string const& order,
                                                                           string const& id,
                                                                           int limit) const
      {
         vector<transaction_detail_object_ex> result_ex;
         try
         {
            vector<transaction_detail_object> result;
            result = my->_remote_db->search_account_history(account_name, order, id, limit);

            for (auto& item : result)
            {
               auto const& memo = item.m_transaction_encrypted_memo;
               if (memo)
               {
                  item.m_str_description += " - ";
                  auto it = my->_keys.find(memo->to);
                  if (it == my->_keys.end())
                     // memo is encrypted for someone else
                     item.m_str_description += "{encrypted}";
                  else
                  {
                     // here the memo is encrypted for me
                     // so I can decrypt it
                     string mykey = it->second;
                     auto wtok = *wif_to_key(mykey);
                     string str_memo =
                        memo->get_message(wtok, memo->from);

                     item.m_str_description += str_memo;
                  }
               }

               transaction_detail_object_ex item_ex = item;
               item_ex.m_from_account_public_key = get_account(string(object_id_type(item_ex.m_from_account))).name;
               item_ex.m_to_account_public_key = get_account(string(object_id_type(item_ex.m_to_account))).name;
               result_ex.push_back(item_ex);
            }
         }
         catch(...){}

         return result_ex;
      }

   brain_key_info wallet_api::suggest_brain_key()const
   {
      brain_key_info result;
      // create a private key for secure entropy
      fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
      fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
      fc::bigint entropy1( sha_entropy1.data(), sha_entropy1.data_size() );
      fc::bigint entropy2( sha_entropy2.data(), sha_entropy2.data_size() );
      fc::bigint entropy(entropy1);
      entropy <<= 8*sha_entropy1.data_size();
      entropy += entropy2;
      string brain_key = "";

      for( int i=0; i<BRAIN_KEY_WORD_COUNT; i++ )
      {
         fc::bigint choice = entropy % graphene::words::word_list_size;
         entropy /= graphene::words::word_list_size;
         if( i > 0 )
            brain_key += " ";
         brain_key += graphene::words::word_list[ choice.to_int64() ];
      }

      brain_key = normalize_brain_key(brain_key);
      fc::ecc::private_key priv_key = derive_private_key( brain_key, 0 );
      result.brain_priv_key = brain_key;
      result.wif_priv_key = key_to_wif( priv_key );
      result.pub_key = priv_key.get_public_key();
      return result;
   }

   brain_key_info wallet_api::get_brain_key_info(string const& brain_key) const
   {
      brain_key_info result;

      string str_brain_key = normalize_brain_key(brain_key);
      fc::ecc::private_key priv_key = derive_private_key( str_brain_key, 0 );
      result.brain_priv_key = str_brain_key;
      result.wif_priv_key = key_to_wif( priv_key );
      result.pub_key = priv_key.get_public_key();
      return result;
   }

   DInteger wallet_api::generate_encryption_key() const
   {
      CryptoPP::Integer secret(randomGenerator, 256);
      return secret;
   }

   string wallet_api::serialize_transaction( signed_transaction tx )const
   {
      return fc::to_hex(fc::raw::pack(tx));
   }

   variant wallet_api::get_object( object_id_type id ) const
   {
      return my->_remote_db->get_objects({id});
   }

   string wallet_api::get_wallet_filename() const
   {
      return my->get_wallet_filename();
   }

   account_object wallet_api::get_account(string account_name_or_id) const
   {
      return my->get_account(account_name_or_id);
   }

   asset_object wallet_api::get_asset(string asset_name_or_id) const
   {
      auto a = my->find_asset(asset_name_or_id);
      FC_ASSERT(a);
      return *a;
   }

   account_id_type wallet_api::get_account_id(string account_name_or_id) const
   {
      return my->get_account_id(account_name_or_id);
   }

   asset_id_type wallet_api::get_asset_id(string asset_symbol_or_id) const
   {
      return my->get_asset_id(asset_symbol_or_id);
   }

   bool wallet_api::import_key(string account_name_or_id, string wif_key)
   {
      FC_ASSERT(!is_locked());
      // backup wallet
      fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
      if (!optional_private_key)
         FC_THROW("Invalid private key");
      string base58_public_key = optional_private_key->get_public_key().to_base58();
   //   copy_wallet_file( "before-import-key-" + base58_public_key );

      if( my->import_key(account_name_or_id, wif_key) )
      {
         save_wallet_file();
   //      copy_wallet_file( "after-import-key-" + base58_public_key );
         return true;
      }
      return false;
   }

   map<string, bool> wallet_api::import_accounts( string filename, string password )
   {
      FC_ASSERT( !is_locked() );
      FC_ASSERT( fc::exists( filename ) );

      const auto imported_keys = fc::json::from_file<exported_keys>( filename );

      const auto password_hash = fc::sha512::hash( password );
      FC_ASSERT( fc::sha512::hash( password_hash ) == imported_keys.password_checksum );

      map<string, bool> result;
      for( const auto& item : imported_keys.account_keys )
      {
          const auto import_this_account = [ & ]() -> bool
          {
              try
              {
                  const account_object account = get_account( item.account_name );
                  const auto& owner_keys = account.owner.get_keys();

                  for( const auto& public_key : item.public_keys )
                  {
                      if( std::find( owner_keys.begin(), owner_keys.end(), public_key ) != owner_keys.end() )
                          return true;
                  }
              }
              catch( ... )
              {
              }

              return false;
          };

          const auto should_proceed = import_this_account();
          result[ item.account_name ] = should_proceed;

          if( should_proceed )
          {
              uint32_t import_successes = 0;
              uint32_t import_failures = 0;
              // TODO: First check that all private keys match public keys
              for( const auto& encrypted_key : item.encrypted_private_keys )
              {
                  try
                  {
                     const auto plain_text = fc::aes_decrypt( password_hash, encrypted_key );
                     const auto private_key = fc::raw::unpack<private_key_type>( plain_text );

                     import_key( item.account_name, string( graphene::utilities::key_to_wif( private_key ) ) );
                     ++import_successes;
                  }
                  catch( const fc::exception& e )
                  {
                     elog( "Couldn't import key due to exception ${e}", ("e", e.to_detail_string()) );
                     ++import_failures;
                  }
              }
              ilog( "successfully imported ${n} keys for account ${name}", ("n", import_successes)("name", item.account_name) );
              if( import_failures > 0 )
                 elog( "failed to import ${n} keys for account ${name}", ("n", import_failures)("name", item.account_name) );
          }
      }

      return result;
   }

   bool wallet_api::import_account_keys( string filename, string password, string src_account_name, string dest_account_name )
   {
      FC_ASSERT( !is_locked() );
      FC_ASSERT( fc::exists( filename ) );

      bool is_my_account = false;
      const auto accounts = list_my_accounts();
      for( const auto& account : accounts )
      {
          if( account.name == dest_account_name )
          {
              is_my_account = true;
              break;
          }
      }
      FC_ASSERT( is_my_account );

      const auto imported_keys = fc::json::from_file<exported_keys>( filename );

      const auto password_hash = fc::sha512::hash( password );
      FC_ASSERT( fc::sha512::hash( password_hash ) == imported_keys.password_checksum );

      bool found_account = false;
      for( const auto& item : imported_keys.account_keys )
      {
          if( item.account_name != src_account_name )
              continue;

          found_account = true;

          for( const auto& encrypted_key : item.encrypted_private_keys )
          {
              const auto plain_text = fc::aes_decrypt( password_hash, encrypted_key );
              const auto private_key = fc::raw::unpack<private_key_type>( plain_text );

              my->import_key( dest_account_name, string( graphene::utilities::key_to_wif( private_key ) ) );
          }

          return true;
      }
      save_wallet_file();

      FC_ASSERT( found_account );

      return false;
   }

   string wallet_api::normalize_brain_key(string s) const
   {
      return detail::normalize_brain_key( s );
   }

   variant wallet_api::info()
   {
      return my->info();
   }

   variant_object wallet_api::about() const
   {
       return my->about();
   }

   fc::ecc::private_key wallet_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
   {
      return detail::derive_private_key( prefix_string, sequence_number );
   }

   signed_transaction wallet_api::create_account_with_brain_key(string const& brain_key,
                                                                string const& registrar_account,
                                                                bool broadcast/* = false*/)
   {
      return my->create_account_with_brain_key(
               brain_key, registrar_account, true,
               broadcast
               );
   }

   signed_transaction wallet_api::create_account_with_brain_key_noimport(string const& brain_key,
                                                                         string const& registrar_account,
                                                                         bool broadcast/* = false*/)
   {
      return my->create_account_with_brain_key(
                                               brain_key, registrar_account, false,
                                               broadcast
                                               );
   }

   signed_transaction wallet_api::create_account_with_public_key(public_key_type const& pubkey,
                                                                 string const& registrar_account,
                                                                 bool broadcast/* = false*/)
   {
       string account_name;
       try
       {
           account_name = string(pubkey);
           FC_ASSERT( !my->self.is_locked() );

           signed_transaction tx = my->create_account_with_public_key(pubkey,
                                                                      registrar_account);

           bool save_wallet = true;   // not sure if this is mandatory, just copying existing logic
           if( save_wallet )
               my->save_wallet_file();
           if( broadcast )
               my->_remote_net_broadcast->broadcast_transaction( tx );
           return tx;
       } FC_CAPTURE_AND_RETHROW( (registrar_account) )
   }

   signed_transaction wallet_api::transfer(string from, string to, string amount,
                                           string asset_symbol, string memo, bool broadcast /* = false */)
   {
      return my->transfer(from, to, amount, asset_symbol, memo, broadcast);
   }

   signed_transaction wallet_api::set_transfer_freeze_block(string const& announcer,
                                                            uint64_t block_num,
                                                            bool broadcast /* = false */)
   {
      return my->set_transfer_freeze_block(announcer, block_num, broadcast);
   }


   vector< vesting_balance_object_with_info > wallet_api::get_vesting_balances( string account_name )
   {
      return my->get_vesting_balances( account_name );
   }


   void wallet_api::set_wallet_filename(string wallet_filename)
   {
      my->_wallet_filename = wallet_filename;
   }

   signed_transaction wallet_api::sign_transaction(signed_transaction tx, bool broadcast /* = false */)
   { try {
      return my->sign_transaction( tx, broadcast);
   } FC_CAPTURE_AND_RETHROW( (tx) ) }

   operation wallet_api::get_prototype_operation(string operation_name)
   {
      return my->get_prototype_operation( operation_name );
   }

   void wallet_api::dbg_push_blocks( std::string src_filename, uint32_t count )
   {
      my->dbg_push_blocks( src_filename, count );
   }

   void wallet_api::dbg_generate_blocks( std::string debug_wif_key, uint32_t count )
   {
      my->dbg_generate_blocks( debug_wif_key, count );
   }

   void wallet_api::dbg_stream_json_objects( const std::string& filename )
   {
      my->dbg_stream_json_objects( filename );
   }

   void wallet_api::dbg_update_object( fc::variant_object update )
   {
      my->dbg_update_object( update );
   }

   void wallet_api::network_add_nodes( const vector<string>& nodes )
   {
      my->network_add_nodes( nodes );
   }

   vector< variant > wallet_api::network_get_connected_peers()
   {
      return my->network_get_connected_peers();
   }

   void wallet_api::flood_network(string prefix, uint32_t number_of_transactions)
   {
      FC_ASSERT(!is_locked());
      my->flood_network(prefix, number_of_transactions);
   }

   signed_transaction wallet_api::propose_fee_change(
      const string& proposing_account,
      fc::time_point_sec expiration_time,
      const variant_object& changed_fees,
      bool broadcast /* = false */
      )
   {
      return my->propose_fee_change( proposing_account, expiration_time, changed_fees, broadcast );
   }

   signed_transaction wallet_api::approve_proposal(
      const string& fee_paying_account,
      const string& proposal_id,
      const approval_delta& delta,
      bool broadcast /* = false */
      )
   {
      return my->approve_proposal( fee_paying_account, proposal_id, delta, broadcast );
   }

   global_property_object wallet_api::get_global_properties() const
   {
      return my->get_global_properties();
   }

   dynamic_global_property_object wallet_api::get_dynamic_global_properties() const
   {
      return my->get_dynamic_global_properties();
   }

   string wallet_api::help()const
   {
      std::vector<std::string> method_names = my->method_documentation.get_method_names();
      std::stringstream ss;
      for (const std::string method_name : method_names)
      {
         try
         {
            ss << my->method_documentation.get_brief_description(method_name);
         }
         catch (const fc::key_not_found_exception&)
         {
            ss << method_name << " (no help available)\n";
         }
      }
      return ss.str();
   }

   string wallet_api::gethelp(const string& method)const
   {
      fc::api<wallet_api> tmp;
      std::stringstream ss;
      ss << "\n";

      if( method == "import_key" )
      {
         ss << "usage: import_key ACCOUNT_NAME_OR_ID  WIF_PRIVATE_KEY\n\n";
         ss << "example: import_key \"1.2.11\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
         ss << "example: import_key \"usera\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
      }
      else if( method == "transfer" )
      {
         ss << "usage: transfer FROM TO AMOUNT SYMBOL \"memo\" BROADCAST\n\n";
         ss << "example: transfer \"1.2.11\" \"1.2.4\" 1000.03 CVA \"memo\" true\n";
         ss << "example: transfer \"usera\" \"userb\" 1000.123 CVA \"memo\" true\n";
      }
      else
      {
         std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
         if (!doxygenHelpString.empty())
            ss << doxygenHelpString;
         else
            ss << "No help defined for method " << method << "\n";
      }

      return ss.str();
   }

   bool wallet_api::load_wallet_file( string wallet_filename )
   {
      return my->load_wallet_file( wallet_filename );
   }

   void wallet_api::save_wallet_file( string wallet_filename )
   {
      my->save_wallet_file( wallet_filename );
   }

   std::map<string,std::function<string(fc::variant,const fc::variants&)> >
   wallet_api::get_result_formatters() const
   {
      return my->get_result_formatters();
   }

   bool wallet_api::is_locked()const
   {
      return my->is_locked();
   }
   bool wallet_api::is_new()const
   {
      return my->_wallet.cipher_keys.size() == 0;
   }

   void wallet_api::encrypt_keys()
   {
      my->encrypt_keys();
   }

   void wallet_api::lock()
   { try {
      FC_ASSERT( !is_locked() );
      encrypt_keys();
      for( auto & key : my->_keys )
         key.second = key_to_wif(fc::ecc::private_key());
      my->_keys.clear();
      my->_checksum = fc::sha512();
      my->self.lock_changed(true);
   } FC_CAPTURE_AND_RETHROW() }

   void wallet_api::unlock(string password)
   { try {
      FC_ASSERT(password.size() > 0);
      auto pw = fc::sha512::hash(password.c_str(), password.size());
      vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
      auto pk = fc::raw::unpack<plain_keys>(decrypted);
      FC_ASSERT(pk.checksum == pw);
      my->_keys = std::move(pk.keys);
      my->_checksum = pk.checksum;
      my->self.lock_changed(false);
   } FC_CAPTURE_AND_RETHROW() }

   void wallet_api::set_password( string password )
   {
      if( !is_new() )
         FC_ASSERT( !is_locked(), "The wallet must be unlocked before the password can be set" );
      my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
      lock();
   }

   map<string,miner_id_type> wallet_api::list_miners(const string& lowerbound, uint32_t limit)
   {
      return my->_remote_db->lookup_miner_accounts(lowerbound, limit);
   }

   miner_object wallet_api::get_miner(string owner_account)
   {
      try
      {
         fc::optional<miner_id_type> miner_id = detail::maybe_id<miner_id_type>(owner_account);
         if (miner_id)
         {
            std::vector<miner_id_type> ids_to_get;
            ids_to_get.push_back(*miner_id);
            std::vector<fc::optional<miner_object>> miner_objects = my->_remote_db->get_miners(ids_to_get);
            if (miner_objects.front())
               return *miner_objects.front();
            FC_THROW("No miner is registered for id ${id}", ("id", owner_account));
         }
         else
         {
            // then maybe it's the owner account
            try
            {
               account_id_type owner_account_id = get_account_id(owner_account);
               fc::optional<miner_object> miner = my->_remote_db->get_miner_by_account(owner_account_id);
               if (miner)
                  return *miner;
               else
                  FC_THROW("No miner is registered for account ${account}", ("account", owner_account));
            }
            catch (const fc::exception&)
            {
               FC_THROW("No account or miner named ${account}", ("account", owner_account));
            }
         }
      }
      FC_CAPTURE_AND_RETHROW( (owner_account) )
   }

   signed_transaction wallet_api::create_miner(string owner_account,
                                               string url,
                                               bool broadcast /* = false */)
   { try {
         account_object miner_account = get_account(owner_account);
         fc::ecc::private_key active_private_key = my->get_private_key_for_account(miner_account);
         int miner_key_index = my->find_first_unused_derived_key_index(active_private_key);
         fc::ecc::private_key miner_private_key = detail::derive_private_key(key_to_wif(active_private_key), miner_key_index);
         graphene::chain::public_key_type miner_public_key = miner_private_key.get_public_key();

         miner_create_operation miner_create_op;
         miner_create_op.miner_account = miner_account.id;
         miner_create_op.block_signing_key = miner_public_key;
         miner_create_op.url = url;

         if (my->_remote_db->get_miner_by_account(miner_create_op.miner_account))
            FC_THROW("Account ${owner_account} is already a miner", ("owner_account", owner_account));

         signed_transaction tx;
         tx.operations.push_back( miner_create_op );
         my->set_operation_fees( tx, my->_remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         my->_wallet.pending_miner_registrations[owner_account] = key_to_wif(miner_private_key);

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (owner_account)(broadcast) ) }

   signed_transaction wallet_api::update_miner(
         string miner_name,
         string url,
         string block_signing_key,
         bool broadcast /* = false */)
   { try {
         miner_object miner = get_miner(miner_name);
         account_object miner_account = my->get_account( miner.miner_account );
         fc::ecc::private_key active_private_key = my->get_private_key_for_account(miner_account);

         miner_update_operation miner_update_op;
         miner_update_op.miner = miner.id;
         miner_update_op.miner_account = miner_account.id;
         if( url != "" )
            miner_update_op.new_url = url;
         if( block_signing_key != "" )
            miner_update_op.new_signing_key = public_key_type( block_signing_key );

         signed_transaction tx;
         tx.operations.push_back( miner_update_op );
         my->set_operation_fees( tx, my->_remote_db->get_global_properties().parameters.current_fees );
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (miner_name)(url)(block_signing_key)(broadcast) ) }

   signed_transaction wallet_api::withdraw_vesting(
         string miner_name,
         string amount,
         string asset_symbol,
         bool broadcast /* = false */)
   { try {
         asset_object asset_obj = get_asset( asset_symbol );
         fc::optional<vesting_balance_id_type> vbid = detail::maybe_id<vesting_balance_id_type>(miner_name);
         if( !vbid )
         {
            miner_object wit = get_miner( miner_name );
            FC_ASSERT( wit.pay_vb );
            vbid = wit.pay_vb;
         }

         vesting_balance_object vbo = my->get_object< vesting_balance_object >( *vbid );
         vesting_balance_withdraw_operation vesting_balance_withdraw_op;

         vesting_balance_withdraw_op.vesting_balance = *vbid;
         vesting_balance_withdraw_op.owner = vbo.owner;
         vesting_balance_withdraw_op.amount = asset_obj.amount_from_string(amount);

         signed_transaction tx;
         tx.operations.push_back( vesting_balance_withdraw_op );
         my->set_operation_fees( tx, my->_remote_db->get_global_properties().parameters.current_fees );
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (miner_name)(amount) )
      }

   signed_transaction wallet_api::vote_for_miner(string voting_account,
                                                 string miner,
                                                 bool approve,
                                                 bool broadcast /* = false */)
   { try {
         account_object voting_account_object = get_account(voting_account);
         account_id_type miner_owner_account_id = get_account_id(miner);
         fc::optional<miner_object> miner_obj = my->_remote_db->get_miner_by_account(miner_owner_account_id);
         if (!miner_obj)
            FC_THROW("Account ${miner} is not registered as a miner", ("miner", miner));
         if (approve)
         {
            auto insert_result = voting_account_object.options.votes.insert(miner_obj->vote_id);
            if (!insert_result.second)
               FC_THROW("Account ${account} was already voting for miner ${miner}", ("account", voting_account)("miner", miner));
         }
         else
         {
            unsigned votes_removed = voting_account_object.options.votes.erase(miner_obj->vote_id);
            if (!votes_removed)
               FC_THROW("Account ${account} is already not voting for miner ${miner}", ("account", voting_account)("miner", miner));
         }
         account_update_operation account_update_op;
         account_update_op.account = voting_account_object.id;
         account_update_op.new_options = voting_account_object.options;

         signed_transaction tx;
         tx.operations.push_back( account_update_op );
         my->set_operation_fees( tx, my->_remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (voting_account)(miner)(approve)(broadcast) ) }

   signed_transaction wallet_api::set_voting_proxy(string account_to_modify,
                                                   optional<string> voting_account,
                                                   bool broadcast /* = false */)
   { try {
         account_object account_object_to_modify = my->get_account(account_to_modify);
         if (voting_account)
         {
            account_id_type new_voting_account_id = get_account_id(*voting_account);
            if (account_object_to_modify.options.voting_account == new_voting_account_id)
               FC_THROW("Voting proxy for ${account} is already set to ${voter}", ("account", account_to_modify)("voter", *voting_account));
            account_object_to_modify.options.voting_account = new_voting_account_id;
         }
         else
         {
            if (account_object_to_modify.options.voting_account == GRAPHENE_PROXY_TO_SELF_ACCOUNT)
               FC_THROW("Account ${account} is already voting for itself", ("account", account_to_modify));
            account_object_to_modify.options.voting_account = GRAPHENE_PROXY_TO_SELF_ACCOUNT;
         }

         account_update_operation account_update_op;
         account_update_op.account = account_object_to_modify.id;
         account_update_op.new_options = account_object_to_modify.options;

         signed_transaction tx;
         tx.operations.push_back( account_update_op );
         my->set_operation_fees( tx, my->_remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (account_to_modify)(voting_account)(broadcast) ) }

   signed_transaction wallet_api::set_desired_miner_count(string account_to_modify,
                                                          uint16_t desired_number_of_miners,
                                                          bool broadcast /* = false */)
   { try {
         account_object account_object_to_modify = get_account(account_to_modify);

         if (account_object_to_modify.options.num_miner == desired_number_of_miners)
            FC_THROW("Account ${account} is already voting for ${miners} miners",
                     ("account", account_to_modify)("miners", desired_number_of_miners));
         account_object_to_modify.options.num_miner = desired_number_of_miners;

         account_update_operation account_update_op;
         account_update_op.account = account_object_to_modify.id;
         account_update_op.new_options = account_object_to_modify.options;

         signed_transaction tx;
         tx.operations.push_back( account_update_op );
         my->set_operation_fees( tx, my->_remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (account_to_modify)(desired_number_of_miners)(broadcast) ) }

   map<public_key_type, string> wallet_api::dump_private_keys()
   {
      FC_ASSERT(!is_locked());
      return my->_keys;
   }


   string wallet_api::get_private_key( public_key_type pubkey )const
   {
      return key_to_wif( my->get_private_key( pubkey ) );
   }

   signed_block_with_info::signed_block_with_info( const signed_block& block )
      : signed_block( block )
   {
      block_id = id();
      signing_key = signee();
      transaction_ids.reserve( transactions.size() );
      for( const processed_transaction& tx : transactions )
         transaction_ids.push_back( tx.id() );
   }

   vesting_balance_object_with_info::vesting_balance_object_with_info( const vesting_balance_object& vbo, fc::time_point_sec now )
      : vesting_balance_object( vbo )
   {
      allowed_withdraw = get_allowed_withdraw( now );
      allowed_withdraw_time = now;
   }

   real_supply wallet_api::get_real_supply()const
   {
      return my->_remote_db->get_real_supply();
   }

   std::string wallet_api::sign_buffer(std::string const& str_buffer,
                                       std::string const& str_brainkey) const
   {
      if (str_buffer.empty() ||
          str_brainkey.empty())
         throw std::runtime_error("You need buffer and brainkey to sign");

      string normalized_brain_key = normalize_brain_key( str_brainkey );
      fc::ecc::private_key privkey = derive_private_key( normalized_brain_key, 0 );

      fc::sha256 digest(str_buffer);

      auto sign = privkey.sign_compact(digest);

      return fc::to_hex((const char*)sign.begin(), sign.size());
   }

   bool wallet_api::verify_signature(std::string const& str_buffer,
                                     std::string const& str_publickey,
                                     std::string const& str_signature) const
   {
      try
      {
      if (str_buffer.empty() ||
          str_publickey.empty() ||
          str_signature.empty())
         throw std::runtime_error("You need buffer, public key and signature to verify");

      fc::ecc::compact_signature signature;
      fc::from_hex(str_signature, (char*)signature.begin(), signature.size());
      fc::sha256 digest(str_buffer);

      fc::ecc::public_key pub_key(signature, digest);
      public_key_type provided_key(str_publickey);

      if (provided_key == pub_key)
         return true;
      else
         return false;
      }
      catch(...){}

      return false;
   }

   fc::time_point_sec wallet_api::head_block_time() const
   {
       return my->head_block_time();
   }

   optional<signed_block> wallet_api::get_head_block()const
   {
       return my->get_head_block();
   }

   optional<signed_block> wallet_api::get_nearest_block(const string& time_iso)const
   {
       return my->get_nearest_block(time_iso);
   }

   public_key_type  wallet_api::get_public_key( string label )const
   {
      try { return fc::variant(label).as<public_key_type>(); } catch ( ... ){}

      auto key_itr   = my->_wallet.labeled_keys.get<by_label>().find(label);
      if( key_itr != my->_wallet.labeled_keys.get<by_label>().end() )
         return key_itr->key;
      return public_key_type();
   }

   /* BLIND TRANSFERS */
   bool wallet_api::set_key_label( public_key_type key, string label )
   {
      auto result = my->_wallet.labeled_keys.insert( key_label{label,key} );
      if( result.second  ) return true;

      auto key_itr   = my->_wallet.labeled_keys.get<by_key>().find(key);
      auto label_itr = my->_wallet.labeled_keys.get<by_label>().find(label);
      if( label_itr == my->_wallet.labeled_keys.get<by_label>().end() )
      {
         if( key_itr != my->_wallet.labeled_keys.get<by_key>().end() )
            return my->_wallet.labeled_keys.get<by_key>().modify( key_itr, [&]( key_label& obj ){ obj.label = label; } );
      }
      return false;
   }

   string wallet_api::get_key_label( public_key_type key )const
   {
      auto key_itr   = my->_wallet.labeled_keys.get<by_key>().find(key);
      if( key_itr != my->_wallet.labeled_keys.get<by_key>().end() )
         return key_itr->label;
      return string();
   }

   map<string,public_key_type> wallet_api::get_blind_accounts()const
   {
      map<string,public_key_type> result;
      for( const auto& item : my->_wallet.labeled_keys )
         result[item.label] = item.key;
      return result;
   }
   map<string,public_key_type> wallet_api::get_my_blind_accounts()const
   {
      FC_ASSERT( !is_locked() );
      map<string,public_key_type> result;
      for( const auto& item : my->_wallet.labeled_keys )
      {
         if( my->_keys.find(item.key) != my->_keys.end() )
            result[item.label] = item.key;
      }
      return result;
   }

   public_key_type    wallet_api::create_blind_account( string label, string brain_key  )
   {
      FC_ASSERT( !is_locked() );

      auto label_itr = my->_wallet.labeled_keys.get<by_label>().find(label);
      if( label_itr !=  my->_wallet.labeled_keys.get<by_label>().end() )
         FC_ASSERT( !"Key with label already exists" );
      brain_key = fc::trim_and_normalize_spaces( brain_key );
      auto secret = fc::sha256::hash( brain_key.c_str(), brain_key.size() );
      auto priv_key = fc::ecc::private_key::regenerate( secret );
      public_key_type pub_key  = priv_key.get_public_key();

      FC_ASSERT( set_key_label( pub_key, label ) );

      my->_keys[pub_key] = graphene::utilities::key_to_wif( priv_key );

      save_wallet_file();
      return pub_key;
   }

   vector<asset>   wallet_api::get_blind_balances( string key_or_label )
   {
      vector<asset> result;
      map<asset_id_type, share_type> balances;

      vector<commitment_type> used;

      auto pub_key = get_public_key( key_or_label );
      auto& to_asset_used_idx = my->_wallet.blind_receipts.get<by_to_asset_used>();
      auto start = to_asset_used_idx.lower_bound( std::make_tuple(pub_key,asset_id_type(0),false)  );
      auto end = to_asset_used_idx.lower_bound( std::make_tuple(pub_key,asset_id_type(uint32_t(0xffffffff)),true)  );
      while( start != end )
      {
         if( !start->used  )
         {
            auto answer = my->_remote_db->get_blinded_balances( {start->commitment()} );
            if( answer.size() )
               balances[start->amount.asset_id] += start->amount.amount;
            else
               used.push_back( start->commitment() );
         }
         ++start;
      }
      for( const auto& u : used )
      {
         auto itr = my->_wallet.blind_receipts.get<by_commitment>().find( u );
         my->_wallet.blind_receipts.modify( itr, []( blind_receipt& r ){ r.used = true; } );
      }
      for( auto item : balances )
         result.push_back( asset( item.second, item.first ) );
      return result;
   }

   blind_confirmation wallet_api::transfer_from_blind( string from_blind_account_key_or_label,
                                                       string to_account_id_or_name,
                                                       string amount_in,
                                                       string symbol,
                                                       bool broadcast )
   { try {
      transfer_from_blind_operation from_blind;


      auto fees  = my->_remote_db->get_global_properties().parameters.current_fees;
      fc::optional<asset_object> asset_obj = get_asset(symbol);
      FC_ASSERT(asset_obj.valid(), "Could not find asset matching ${asset}", ("asset", symbol));
      auto amount = asset_obj->amount_from_string(amount_in);

      from_blind.fee  = fees->calculate_fee( from_blind );

      auto blind_in = asset_obj->amount_to_string( from_blind.fee + amount );


      auto conf = blind_transfer_help( from_blind_account_key_or_label,
                                  from_blind_account_key_or_label,
                                  blind_in, symbol, false, true/*to_temp*/ );
      FC_ASSERT( conf.outputs.size() > 0 );

      auto to_account = my->get_account( to_account_id_or_name );
      from_blind.to = to_account.id;
      from_blind.amount = amount;
      from_blind.blinding_factor = conf.outputs.back().decrypted_memo.blinding_factor;
      from_blind.inputs.push_back( {conf.outputs.back().decrypted_memo.commitment, authority() } );
      from_blind.fee  = fees->calculate_fee( from_blind );

      idump( (from_blind) );
      conf.trx.operations.push_back(from_blind);
      ilog( "about to validate" );
      conf.trx.validate();

      if( broadcast && conf.outputs.size() == 2 ) {

          // Save the change
          blind_confirmation::output conf_output;
          blind_confirmation::output change_output = conf.outputs[0];

          // The wallet must have a private key for confirmation.to, this is used to decrypt the memo
          public_key_type from_key = get_public_key(from_blind_account_key_or_label);
          conf_output.confirmation.to = from_key;
          conf_output.confirmation.one_time_key = change_output.confirmation.one_time_key;
          conf_output.confirmation.encrypted_memo = change_output.confirmation.encrypted_memo;
          conf_output.confirmation_receipt = conf_output.confirmation;
          //try {
          receive_blind_transfer( conf_output.confirmation_receipt, from_blind_account_key_or_label, "@"+to_account.name );
          //} catch ( ... ){}
      }

      ilog( "about to broadcast" );
      conf.trx = sign_transaction( conf.trx, broadcast );

      return conf;
   } FC_CAPTURE_AND_RETHROW( (from_blind_account_key_or_label)(to_account_id_or_name)(amount_in)(symbol) ) }

   blind_confirmation wallet_api::blind_transfer( string from_key_or_label,
                                                  string to_key_or_label,
                                                  string amount_in,
                                                  string symbol,
                                                  bool broadcast )
   {
      return blind_transfer_help( from_key_or_label, to_key_or_label, amount_in, symbol, broadcast, false );
   }
   blind_confirmation wallet_api::blind_transfer_help( string from_key_or_label,
                                                  string to_key_or_label,
                                                  string amount_in,
                                                  string symbol,
                                                  bool broadcast,
                                                  bool to_temp )
   {
      blind_confirmation confirm;
      try {

      FC_ASSERT( !is_locked() );
      public_key_type from_key = get_public_key(from_key_or_label);
      public_key_type to_key   = get_public_key(to_key_or_label);

      fc::optional<asset_object> asset_obj = get_asset(symbol);
      FC_ASSERT(asset_obj.valid(), "Could not find asset matching ${asset}", ("asset", symbol));

      blind_transfer_operation blind_tr;
      blind_tr.outputs.resize(2);

      auto fees  = my->_remote_db->get_global_properties().parameters.current_fees;

      auto amount = asset_obj->amount_from_string(amount_in);

      asset total_amount = asset_obj->amount(0);

      vector<fc::sha256> blinding_factors;

      //auto from_priv_key = my->get_private_key( from_key );

      blind_tr.fee  = fees->calculate_fee( blind_tr );

      vector<commitment_type> used;

      auto& to_asset_used_idx = my->_wallet.blind_receipts.get<by_to_asset_used>();
      auto start = to_asset_used_idx.lower_bound( std::make_tuple(from_key,amount.asset_id,false)  );
      auto end = to_asset_used_idx.lower_bound( std::make_tuple(from_key,amount.asset_id,true)  );
      while( start != end )
      {
         auto result = my->_remote_db->get_blinded_balances( {start->commitment() } );
         if( result.size() == 0 )
         {
            used.push_back( start->commitment() );
         }
         else
         {
            blind_tr.inputs.push_back({start->commitment(), start->control_authority});
            blinding_factors.push_back( start->data.blinding_factor );
            total_amount += start->amount;

            if( total_amount >= amount + blind_tr.fee )
               break;
         }
         ++start;
      }
      for( const auto& u : used )
      {
         auto itr = my->_wallet.blind_receipts.get<by_commitment>().find( u );
         my->_wallet.blind_receipts.modify( itr, []( blind_receipt& r ){ r.used = true; } );
      }

      FC_ASSERT( total_amount >= amount+blind_tr.fee, "Insufficent Balance", ("available",total_amount)("amount",amount)("fee",blind_tr.fee) );

      auto one_time_key = fc::ecc::private_key::generate();
      auto secret       = one_time_key.get_shared_secret( to_key );
      auto child        = fc::sha256::hash( secret );
      auto nonce        = fc::sha256::hash( one_time_key.get_secret() );
      auto blind_factor = fc::sha256::hash( child );

      auto from_secret  = one_time_key.get_shared_secret( from_key );
      auto from_child   = fc::sha256::hash( from_secret );
      auto from_nonce   = fc::sha256::hash( nonce );

      auto change = total_amount - amount - blind_tr.fee;
      fc::sha256 change_blind_factor;
      fc::sha256 to_blind_factor;
      if( change.amount > 0 )
      {
         idump(("to_blind_factor")(blind_factor) );
         blinding_factors.push_back( blind_factor );
         change_blind_factor = fc::ecc::blind_sum( blinding_factors, blinding_factors.size() - 1 );
         wdump(("change_blind_factor")(change_blind_factor) );
      }
      else // change == 0
      {
         blind_tr.outputs.resize(1);
         blind_factor = fc::ecc::blind_sum( blinding_factors, blinding_factors.size() );
         idump(("to_sum_blind_factor")(blind_factor) );
         blinding_factors.push_back( blind_factor );
         idump(("nochange to_blind_factor")(blind_factor) );
      }
      fc::ecc::public_key from_pub_key = from_key;
      fc::ecc::public_key to_pub_key = to_key;

      blind_output to_out;
      to_out.owner       = to_temp ? authority() : authority( 1, public_key_type( to_pub_key.child( child ) ), 1 );
      to_out.commitment  = fc::ecc::blind( blind_factor, amount.amount.value );
      idump(("to_out.blind")(blind_factor)(to_out.commitment) );


      if( blind_tr.outputs.size() > 1 )
      {
         to_out.range_proof = fc::ecc::range_proof_sign( 0, to_out.commitment, blind_factor, nonce,  0, 0, amount.amount.value );

         blind_output change_out;
         change_out.owner       = authority( 1, public_key_type( from_pub_key.child( from_child ) ), 1 );
         change_out.commitment  = fc::ecc::blind( change_blind_factor, change.amount.value );
         change_out.range_proof = fc::ecc::range_proof_sign( 0, change_out.commitment, change_blind_factor, from_nonce,  0, 0, change.amount.value );
         blind_tr.outputs[1] = change_out;


         blind_confirmation::output conf_output;
         conf_output.label = from_key_or_label;
         conf_output.pub_key = from_key;
         conf_output.decrypted_memo.from = from_key;
         conf_output.decrypted_memo.amount = change;
         conf_output.decrypted_memo.blinding_factor = change_blind_factor;
         conf_output.decrypted_memo.commitment = change_out.commitment;
         conf_output.decrypted_memo.check   = from_secret._hash[0];
         conf_output.confirmation.one_time_key = one_time_key.get_public_key();
         conf_output.confirmation.to = from_key;
         conf_output.confirmation.encrypted_memo = fc::aes_encrypt( from_secret, fc::raw::pack( conf_output.decrypted_memo ) );
         conf_output.auth = change_out.owner;
         conf_output.confirmation_receipt = conf_output.confirmation;

         confirm.outputs.push_back( conf_output );
      }
      blind_tr.outputs[0] = to_out;

      blind_confirmation::output conf_output;
      conf_output.label = to_key_or_label;
      conf_output.pub_key = to_key;
      conf_output.decrypted_memo.from = from_key;
      conf_output.decrypted_memo.amount = amount;
      conf_output.decrypted_memo.blinding_factor = blind_factor;
      conf_output.decrypted_memo.commitment = to_out.commitment;
      conf_output.decrypted_memo.check   = secret._hash[0];
      conf_output.confirmation.one_time_key = one_time_key.get_public_key();
      conf_output.confirmation.to = to_key;
      conf_output.confirmation.encrypted_memo = fc::aes_encrypt( secret, fc::raw::pack( conf_output.decrypted_memo ) );
      conf_output.auth = to_out.owner;
      conf_output.confirmation_receipt = conf_output.confirmation;

      confirm.outputs.push_back( conf_output );

      /** commitments must be in sorted order */
      std::sort( blind_tr.outputs.begin(), blind_tr.outputs.end(),
                 [&]( const blind_output& a, const blind_output& b ){ return a.commitment < b.commitment; } );
      std::sort( blind_tr.inputs.begin(), blind_tr.inputs.end(),
                 [&]( const blind_input& a, const blind_input& b ){ return a.commitment < b.commitment; } );

      confirm.trx.operations.emplace_back( std::move(blind_tr) );
      ilog( "validate before" );
      confirm.trx.validate();
      confirm.trx = sign_transaction(confirm.trx, broadcast);

      if( broadcast )
      {
         for( const auto& out : confirm.outputs )
         {
            try { receive_blind_transfer( out.confirmation_receipt, from_key_or_label, "" ); } catch ( ... ){}
         }
      }

      return confirm;
   } FC_CAPTURE_AND_RETHROW( (from_key_or_label)(to_key_or_label)(amount_in)(symbol)(broadcast)(confirm) ) }



   /**
    *  Transfers a public balance from @from to one or more blinded balances using a
    *  stealth transfer.
    */
   blind_confirmation wallet_api::transfer_to_blind( string from_account_id_or_name,
                                                     string asset_symbol,
                                                     /** map from key or label to amount */
                                                     vector<pair<string, string>> to_amounts,
                                                     bool broadcast )
   { try {
      FC_ASSERT( !is_locked() );

      blind_confirmation confirm;
      account_object from_account = my->get_account(from_account_id_or_name);

      fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
      FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

      transfer_to_blind_operation bop;
      bop.from   = from_account.id;

      vector<fc::sha256> blinding_factors;

      asset total_amount = asset_obj->amount(0);

      for( auto item : to_amounts )
      {
         auto one_time_key = fc::ecc::private_key::generate();
         auto to_key       = get_public_key( item.first );
         auto secret       = one_time_key.get_shared_secret( to_key );
         auto child        = fc::sha256::hash( secret );
         auto nonce        = fc::sha256::hash( one_time_key.get_secret() );
         auto blind_factor = fc::sha256::hash( child );

         blinding_factors.push_back( blind_factor );

         auto amount = asset_obj->amount_from_string(item.second);
         total_amount += amount;


         fc::ecc::public_key to_pub_key = to_key;
         blind_output out;
         out.owner       = authority( 1, public_key_type( to_pub_key.child( child ) ), 1 );
         out.commitment  = fc::ecc::blind( blind_factor, amount.amount.value );
         if( to_amounts.size() > 1 )
            out.range_proof = fc::ecc::range_proof_sign( 0, out.commitment, blind_factor, nonce,  0, 0, amount.amount.value );


         blind_confirmation::output conf_output;
         conf_output.label = item.first;
         conf_output.pub_key = to_key;
         conf_output.decrypted_memo.amount = amount;
         conf_output.decrypted_memo.blinding_factor = blind_factor;
         conf_output.decrypted_memo.commitment = out.commitment;
         conf_output.decrypted_memo.check   = secret._hash[0];
         conf_output.confirmation.one_time_key = one_time_key.get_public_key();
         conf_output.confirmation.to = to_key;
         conf_output.confirmation.encrypted_memo = fc::aes_encrypt( secret, fc::raw::pack( conf_output.decrypted_memo ) );
         conf_output.confirmation_receipt = conf_output.confirmation;

         confirm.outputs.push_back( conf_output );

         bop.outputs.push_back(out);
      }
      bop.amount          = total_amount;
      bop.blinding_factor = fc::ecc::blind_sum( blinding_factors, blinding_factors.size() );

      /** commitments must be in sorted order */
      std::sort( bop.outputs.begin(), bop.outputs.end(),
                 [&]( const blind_output& a, const blind_output& b ){ return a.commitment < b.commitment; } );

      confirm.trx.operations.push_back( bop );
      my->set_operation_fees( confirm.trx, my->_remote_db->get_global_properties().parameters.current_fees);
      confirm.trx.validate();
      confirm.trx = sign_transaction(confirm.trx, broadcast);

      if( broadcast )
      {
         for( const auto& out : confirm.outputs )
         {
            try { receive_blind_transfer( out.confirmation_receipt, "@"+from_account.name, "from @"+from_account.name ); } catch ( ... ){}
         }
      }

      return confirm;
   } FC_CAPTURE_AND_RETHROW( (from_account_id_or_name)(asset_symbol)(to_amounts) ) }

   blind_receipt wallet_api::receive_blind_transfer( string confirmation_receipt, string opt_from, string opt_memo )
   {
      FC_ASSERT( !is_locked() );
      stealth_confirmation conf(confirmation_receipt);
      FC_ASSERT( conf.to );

      blind_receipt result;
      result.conf = conf;

      auto to_priv_key_itr = my->_keys.find( *conf.to );
      FC_ASSERT( to_priv_key_itr != my->_keys.end(), "No private key for receiver", ("conf",conf) );


      auto to_priv_key = wif_to_key( to_priv_key_itr->second );
      FC_ASSERT( to_priv_key );

      auto secret       = to_priv_key->get_shared_secret( conf.one_time_key );
      auto child        = fc::sha256::hash( secret );

      auto child_priv_key = to_priv_key->child( child );
      //auto blind_factor = fc::sha256::hash( child );

      auto plain_memo = fc::aes_decrypt( secret, conf.encrypted_memo );
      auto memo = fc::raw::unpack<stealth_confirmation::memo_data>( plain_memo );

      result.to_key   = *conf.to;
      result.to_label = get_key_label( result.to_key );
      if( memo.from )
      {
         result.from_key = *memo.from;
         result.from_label = get_key_label( result.from_key );
         if( result.from_label == string() )
         {
            result.from_label = opt_from;
            set_key_label( result.from_key, result.from_label );
         }
      }
      else
      {
         result.from_label = opt_from;
      }
      result.amount = memo.amount;
      result.memo = opt_memo;

      // confirm the amount matches the commitment (verify the blinding factor)
      auto commtiment_test = fc::ecc::blind( memo.blinding_factor, memo.amount.amount.value );
      FC_ASSERT( fc::ecc::verify_sum( {commtiment_test}, {memo.commitment}, 0 ) );

      blind_balance bal;
      bal.amount = memo.amount;
      bal.to     = *conf.to;
      if( memo.from ) bal.from   = *memo.from;
      bal.one_time_key = conf.one_time_key;
      bal.blinding_factor = memo.blinding_factor;
      bal.commitment = memo.commitment;
      bal.used = false;

      auto child_pubkey = child_priv_key.get_public_key();
      auto owner = authority(1, public_key_type(child_pubkey), 1);
      result.control_authority = owner;
      result.data = memo;

      auto child_key_itr = owner.key_auths.find( child_pubkey );
      if( child_key_itr != owner.key_auths.end() )
         my->_keys[child_key_itr->first] = key_to_wif( child_priv_key );

      // my->_wallet.blinded_balances[memo.amount.asset_id][bal.to].push_back( bal );

      result.date = fc::time_point::now();
      my->_wallet.blind_receipts.insert( result );
      my->_keys[child_pubkey] = key_to_wif( child_priv_key );

      save_wallet_file();

      return result;
   }

   vector<blind_receipt> wallet_api::blind_history( string key_or_account )
   {
      vector<blind_receipt> result;
      auto pub_key = get_public_key( key_or_account );

      if( pub_key == public_key_type() )
         return vector<blind_receipt>();

      for( auto& r : my->_wallet.blind_receipts )
      {
         if( r.from_key == pub_key || r.to_key == pub_key )
            result.push_back( r );
      }
      std::sort( result.begin(), result.end(), [&]( const blind_receipt& a, const blind_receipt& b ){ return a.date > b.date; } );
      return result;
   }

   static auto build_confidential_tx = [](const string &A_, const string &B_, asset val, bool generate_range_proof) {
       auto                tx_key_s = fc::ecc::private_key::generate( );
       auto                tx_key_p = tx_key_s.get_public_key( );
       auto                T_       = public_key_type(tx_key_p);
       fc::ecc::public_key A        = public_key_type(A_);
       fc::ecc::public_key B        = public_key_type(B_);

       auto nonce      = fc::sha256::hash(tx_key_s.get_secret( ));
       auto addr_blind = fc::sha256::hash(tx_key_s.get_shared_secret(A));
       auto P          = B.add(addr_blind);
       auto P_         = public_key_type(P);

       auto shared_secret = tx_key_s.get_shared_secret(B);
       auto blind_factor  = fc::sha256::hash(shared_secret);

       auto data       = fc::aes_encrypt(shared_secret, fc::raw::pack(val.amount.value));
       auto commitment = fc::ecc::blind(blind_factor, uint64_t(val.asset_id), val.amount.value);

       optional<vector<char>> commitment_range_proof;
       if(generate_range_proof)
           commitment_range_proof = fc::ecc::range_proof_sign(0, commitment, blind_factor, nonce, 0, 0, val.amount.value);
       return make_tuple(T_, P_, blind_factor, commitment, data, commitment_range_proof, tx_key_s);
   };

   vector<confidential_tx_object> wallet_api::get_confidential_transactions(const string &A, const string &B, bool unspent) const
   {
       FC_ASSERT( !is_locked() );
       auto Ap = public_key_type(A);
       auto As = my->_keys[Ap];
       return my->_remote_db->get_confidential_transactions(As, B, unspent);
   }

   asset wallet_api::get_confidential_balance(const string &A, const string &B, string asset_symbol) const
   {
       FC_ASSERT( !is_locked() );
       fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
       FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

       auto Ap = public_key_type(A);
       auto As = my->_keys[Ap];
       auto Bp = public_key_type(B);
       auto Bs = my->_keys[Bp];

       auto owner_private_a = *wif_to_key(As);
       auto owner_private_b = *wif_to_key(Bs);

       auto _atxs =  my->_remote_db->get_confidential_transactions(As, B, true);

       asset total_amount = asset_obj->amount(0);
       for( auto && tx : _atxs)
       {
           auto shared_secret = owner_private_b.get_shared_secret(tx.tx_key);
           asset amount = asset_obj->amount(0);
           fc::raw::unpack(fc::aes_decrypt(shared_secret, tx.data), amount.amount.value);
           if (amount.amount.value == 0)
               continue;

           total_amount += amount;
       }

       return total_amount;
   }

   /**
    *  Transfers a public balance from @from to one or more confidential balances using a
    *  confidential transfer.
    */
   signed_transaction wallet_api::transfer_to_confidential( string from_account_id_or_name,
                                      string asset_symbol,
                                      /** map from key or label to amount */
                                      vector<pair<string, string>> to_addresses,
                                      vector<string> to_amounts)
   { try {
      FC_ASSERT( !is_locked() );

      account_object from_account = my->get_account(from_account_id_or_name);

      fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
      FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

      transfer_to_confidential_operation op;
      op.from = from_account.id;

      vector<fc::sha256> blinding_factors;

      asset total_amount = asset_obj->amount(0);

      for( auto item : boost::combine(to_addresses, to_amounts) )
      {
          confidential_tx out;
          auto to_address = boost::get<0>(item);
          auto to_amount = boost::get<1>(item);
          auto amount = asset_obj->amount_from_string(to_amount);

          auto v = build_confidential_tx(std::get<0>(to_address), std::get<1>(to_address), amount, (to_amounts.size() > 1));

          out.tx_key = std::get<0>(v);
          out.owner = std::get<1>(v);
          out.commitment = std::get<3>(v);
          out.data = std::get<4>(v);

          total_amount += amount;

          if(std::get<5>(v))
              out.range_proof = *std::get<5>(v);
          blinding_factors.push_back(std::get<2>(v));

          op.outputs.push_back(out);
      }
      op.amount          = total_amount;
      op.blinding_factor = fc::ecc::blind_sum( blinding_factors, blinding_factors.size());

      /** commitments must be in sorted order */
      std::sort( op.outputs.begin(), op.outputs.end(), [&]( const confidential_tx& a, const confidential_tx& b )
      {
          return a.commitment < b.commitment;
      });

      signed_transaction trx;

      trx.operations.push_back(op);
      my->set_operation_fees(trx, my->_remote_db->get_global_properties().parameters.current_fees);
      trx.validate();

      return sign_transaction(trx, true);
   } FC_CAPTURE_AND_RETHROW( (from_account_id_or_name)(asset_symbol)(to_amounts) ) }

   /**
    *  Transfers a public balance from @from to one or more confidential balances using a
    *  confidential transfer.
    */
   signed_transaction wallet_api::transfer_from_confidential(string const &A, string const &B, string asset_symbol,
                                                             /** map from key or label to amount */
                                                             vector<pair<string, string>> to_addresses,
                                                             vector<string>               to_amounts)
   {
       try
       {
           FC_ASSERT(!is_locked( ));

           bool                                 enough_balance = false;
           transfer_from_confidential_operation op;
           fc::optional<asset_object>           asset_obj = get_asset(asset_symbol);
           FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
           auto cf       = my->_remote_db->get_global_properties( ).parameters.current_fees;
           auto base_fee = cf->calculate_fee(op);
           op.outputs.resize(1);
           auto per_out = cf->calculate_fee(op) - base_fee;
           op.outputs.clear( );

           asset total_amount_out = asset_obj->amount(0);
           asset total_amount_in  = asset_obj->amount(0);

           auto Ap = public_key_type(A);
           auto Bp = public_key_type(B);
           auto As = my->_keys[Ap];
           auto Bs = my->_keys[Bp];

           auto owner_private_a = *wif_to_key(As);
           auto owner_private_b = *wif_to_key(Bs);


           vector<fc::sha256>       blinding_factors_in, blinding_factors_out;
           vector<private_key_type> sks;


           for(auto &&a : to_amounts)
           {
               auto amount = asset_obj->amount_from_string(a);
               total_amount_out += amount;
           }

           auto _atxs = get_confidential_transactions(A, B, true);
           FC_ASSERT(_atxs.size( ) > 0, "No confidential transactions for ${A} ${B}", ("A", A)("B", B));

           for(auto &&tx : _atxs)
           {
               if(not tx.unspent)
                   continue;

               confidential_tx in;
               in.commitment  = tx.commitment;
               in.tx_key      = tx.tx_key;
               in.owner       = tx.owner;
               in.range_proof = tx.range_proof;
               in.data        = tx.data;

               auto shared_secret = owner_private_b.get_shared_secret(in.tx_key);
               auto blind_factor  = fc::sha256::hash(shared_secret);
               blinding_factors_in.push_back(blind_factor);

               asset amount = asset_obj->amount(0);

               fc::raw::unpack(fc::aes_decrypt(shared_secret, in.data), amount.amount.value);

               total_amount_in += amount;

               op.inputs.push_back(in);

               if(total_amount_in > total_amount_out + base_fee + (to_amounts.size( ) + 1) * per_out)
               {
                   auto _change = total_amount_in - total_amount_out - (base_fee + (to_amounts.size( ) + 1) * per_out);

                   to_addresses.push_back({A, B});
                   to_amounts.push_back(asset_obj->amount_to_string(_change));

                   enough_balance = true;
                   break;
               }
               else if(total_amount_in == total_amount_out + base_fee + to_amounts.size( ) * per_out)
               {
                   enough_balance = true;
                   break;
               }
           }

           FC_ASSERT(enough_balance, "Not enough balance");


           for(auto &&in : op.inputs)
           {
               auto S = fc::ecc::private_key::generate_from_seed(owner_private_b.get_secret( ),
                                                                 fc::sha256::hash(owner_private_a.get_shared_secret(in.tx_key)));
               sks.push_back(S);
           }

           auto ct_n = std::count_if(to_addresses.begin( ), to_addresses.end( ), [](pair<string, string> const &addr) { return not addr.second.empty( ); });
           for(auto item : boost::combine(to_addresses, to_amounts))
           {
               auto to_address = boost::get<0>(item);
               auto to_amount  = boost::get<1>(item);
               auto amount     = asset_obj->amount_from_string(to_amount);
               if(not to_address.second.empty( ))
               {
                   confidential_tx out;

                   auto v         = build_confidential_tx(to_address.first, to_address.second, amount, ct_n > 1);
                   out.tx_key     = std::get<0>(v);
                   out.owner      = std::get<1>(v);
                   out.commitment = std::get<3>(v);
                   if(std::get<5>(v))
                       out.range_proof = *std::get<5>(v);
                   out.data = std::get<4>(v);

                   blinding_factors_out.push_back(std::get<2>(v));
                   op.outputs.push_back(out);
               }
               else
               {
                   op.to.push_back(public_key_type(to_address.first));
                   op.amount.push_back(amount);
               }
           }
           /** commitments must be in sorted order */
           std::sort(op.outputs.begin( ), op.outputs.end( ), [&](const confidential_tx &a, const confidential_tx &b) {
               return a.commitment < b.commitment;
           });

           auto nn = blinding_factors_in.size( );
           std::copy(blinding_factors_out.begin( ), blinding_factors_out.end( ), std::back_inserter(blinding_factors_in));
           op.blinding_factor = fc::ecc::blind_sum(blinding_factors_in, nn);

           signed_transaction trx;
           trx.operations.push_back(op);

           my->set_operation_fees(trx, cf);
           trx.validate( );
           my->cook_transaction(trx);

           for(auto &&sk : sks)
               trx.sign(sk, my->get_chain_properties( ).chain_id);

           return my->serve_cooked_transaction(trx);
       }
       FC_CAPTURE_AND_RETHROW((A)(B)(asset_symbol)(to_amounts))
   }
} } // graphene::wallet



void fc::to_variant(const account_multi_index_type& accts, fc::variant& vo)
{
   vo = vector<account_object>(accts.begin(), accts.end());
}

void fc::from_variant(const fc::variant& var, account_multi_index_type& vo)
{
   const vector<account_object>& v = var.as<vector<account_object>>();
   vo = account_multi_index_type(v.begin(), v.end());
}


