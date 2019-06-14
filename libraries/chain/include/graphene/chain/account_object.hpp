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
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {
   class database;

   /**
    * @class account_statistics_object
    * @ingroup object
    * @ingroup implementation
    *
    * This object contains regularly updated statistical data about an account. It is provided for the purpose of
    * separating the account data that changes frequently from the account data that is mostly static, which will
    * minimize the amount of data that must be backed up as part of the undo history everytime a transfer is made.
    */
   class account_statistics_object : public graphene::db::abstract_object<account_statistics_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_account_statistics_object_type;

         account_id_type  owner;

         /**
          * Keep the most recent operation as a root pointer to a linked list of the transaction history.
          */
         account_transaction_history_id_type most_recent_op;
         uint32_t                            total_ops = 0;

         /**
          * When calculating votes it is necessary to know how much is stored in orders (and thus unavailable for
          * transfers). Rather than maintaining an index of [asset,owner,order_id] we will simply maintain the running
          * total here and update it every time an order is created or modified.
          */
         share_type total_core_in_orders;


         /**
          * Tracks the fees paid by this account which have not been disseminated to the various parties that receive
          * them yet (registrar, referrer, lifetime referrer, network, etc). This is used as an optimization to avoid
          * doing massive amounts of uint128 arithmetic on each and every operation.
          *
          * These fees will be paid out as vesting cash-back, and this counter will reset during the maintenance
          * interval.
          */
         share_type pending_fees;
         /**
          * Same as @ref pending_fees, except these fees will be paid out as pre-vested cash-back (immediately
          * available for withdrawal) rather than requiring the normal vesting period.
          */
         share_type pending_vested_fees;
   };

   /**
    * @brief Tracks the balance of a single account/asset pair
    * @ingroup object
    *
    * This object is indexed on owner and asset_type so that black swan
    * events in asset_type can be processed quickly.
    */
   class account_balance_object : public graphene::db::abstract_object<account_balance_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_account_balance_object_type;

         account_id_type   owner;
         asset_id_type     asset_type;
         share_type        balance;
         uint64_t          vote_power = 0;
         vote_id_type      casted_vote = {vote_id_type::miner, std::numeric_limits<uint32_t>::max()};

         asset get_balance()const { return asset(balance, asset_type); }
         void  adjust_balance(const asset& delta);
   };

   enum class e_gender
   {
      male,
      female,
      unspecified
   };

   class account_details
   {
   public:
      string first_name;
      string last_name;
      string image_hash;
      e_gender gender;
   };

   template <typename basic_type, typename Derived>
   class PropertyBase
   {
   public:
      using value_type = basic_type;

      void load(string const& str_synopsis)
      {
         m_arrValues.clear();
         fc::variant variant_synopsis;
         bool bFallBack = false;   // fallback to support synopsis that was used simply as a string
         try
         {
            if (false == str_synopsis.empty())
               variant_synopsis = fc::json::from_string(str_synopsis);
         }
         catch(...)
         {
            if (is_fallback(0))
               bFallBack = true;
         }

         fc::variant variant_value;

         if (variant_synopsis.is_object())
         {
            string str_name = Derived::name();

            if (variant_synopsis.get_object().contains(str_name.c_str()))
               variant_value = variant_synopsis[str_name.c_str()];
         }

         if (variant_value.is_array())
         {
            for (size_t iIndex = 0; iIndex < variant_value.size(); ++iIndex)
            {
               fc::variant const& variant_item = variant_value[iIndex];
               string str_value = variant_item.as_string();
               value_type item;
               PropertyBase::convert_from_string(str_value, item);
               m_arrValues.push_back(item);
            }
         }
         else if (false == variant_value.is_null())
         {
            string str_value = variant_value.as_string();
            value_type item;
            PropertyBase::convert_from_string(str_value, item);
            m_arrValues.push_back(item);
         }

         if (bFallBack &&
             m_arrValues.empty())
         {
            value_type item;
            PropertyBase::convert_from_string(str_synopsis, item);
            m_arrValues.push_back(item);
         }

         if (is_default(0) &&
             m_arrValues.empty())
         {
            string str_value;
            value_type item;
            PropertyBase::convert_from_string(str_value, item);
            m_arrValues.push_back(item);
         }

         if (is_unique(0) &&
             1 < m_arrValues.size())
         {
            m_arrValues.resize(1);
         }
      }
      void save(string& str_synopsis) const
      {
         vector<value_type> arrValues = m_arrValues;

         if (is_unique(0) &&
             1 < arrValues.size())
            arrValues.resize(1);

         if (is_default(0) &&
             arrValues.empty())
         {
            string str_value;
            value_type item;
            PropertyBase::convert_from_string(str_value, item);
            arrValues.push_back(item);
         }

         if (arrValues.empty())
            return;

         variant variant_value;
         if (1 == arrValues.size())
         {
            value_type const& item = arrValues[0];
            string str_value;
            PropertyBase::convert_to_string(item, str_value);
            variant_value = str_value;
         }
         else
         {
            fc::variants arr_variant_value;
            for (auto const& item : arrValues)
            {
               string str_value;
               PropertyBase::convert_to_string(item, str_value);
               arr_variant_value.emplace_back(str_value);
            }
            variant_value = arr_variant_value;
         }

         fc::variant variant_synopsis;
         if (false == str_synopsis.empty())
            variant_synopsis = fc::json::from_string(str_synopsis);

         if (false == variant_synopsis.is_null() &&
             false == variant_synopsis.is_object())
            variant_synopsis.clear();

         if (variant_synopsis.is_null())
         {
            fc::mutable_variant_object mutable_variant_obj;
            variant_synopsis = mutable_variant_obj;
         }

         fc::variant_object& variant_obj = variant_synopsis.get_object();
         fc::mutable_variant_object mutable_variant_obj(variant_obj);

         string str_name = Derived::name();
         mutable_variant_obj.set(str_name, variant_value);
         variant_obj = mutable_variant_obj;

         str_synopsis = fc::json::to_string(variant_obj);
      }

      template <typename U>
      static void convert_from_string(string const& str_value, U& converted_value)
      {
         Derived::convert_from_string(str_value, converted_value);
      }
      template <typename U>
      static void convert_to_string(U const& value, string& str_converted_value)
      {
         Derived::convert_to_string(value, str_converted_value);
      }

      static void convert_from_string(string const& str_value, string& converted_value)
      {
         converted_value = str_value;
      }
      static void convert_to_string(string const& str_value, string& converted_value)
      {
         converted_value = str_value;
      }

      template <typename U = Derived>
      static bool is_default(...)
      {
         return false;
      }

      template <typename U = Derived>
      static bool is_default(typename U::meta_default)
      {
         return true;
      }

      template <typename U = Derived>
      static bool is_unique(...)
      {
         return false;
      }

      template <typename U = Derived>
      static bool is_unique(typename U::meta_unique)
      {
         return true;
      }
      template <typename U = Derived>
      static bool is_fallback(...)
      {
         return false;
      }

      template <typename U = Derived>
      static bool is_fallback(typename U::meta_fallback)
      {
         return true;
      }

   public:
      vector<value_type> m_arrValues;
   };

   class PropertyManager
   {
   public:
      PropertyManager(string const& str_synopsis = string())
         : m_str_synopsis(str_synopsis)
      {
      }

      template <typename P>
      typename P::value_type get() const
      {
         P property;
         property.load(m_str_synopsis);

         if (1 != property.m_arrValues.size())
            throw std::runtime_error("PropertyManager expects ideal conditions for now");

         return property.m_arrValues.front();
      }

      template <typename P>
      vector<typename P::value_type> get_values() const
      {
         P property;
         property.load(m_str_synopsis);
         return property.m_arrValues;
      }

      template <typename P>
      void set(typename P::value_type const& value)
      {
         P property;
         property.m_arrValues.clear();
         property.m_arrValues.push_back(value);
         property.save(m_str_synopsis);
      }

      template <typename P>
      void set_values(vector<typename P::value_type> const& values)
      {
         P property;
         property.m_arrValues.clear();
         if (property.is_unique(0))
             property.m_arrValues.push_back(values.front());
         else
             property.m_arrValues = values;
         property.save(m_str_synopsis);
      }

      string m_str_synopsis;
   };

   class AccountObjectFirstName : public PropertyBase<string, AccountObjectFirstName>
   {
   public:
      using meta_unique = bool;
      static string name()
      {
         return "first_name";
      }
   };
   class AccountObjectLastName : public PropertyBase<string, AccountObjectLastName>
   {
   public:
      using meta_unique = bool;
      static string name()
      {
         return "last_name";
      }
   };
   class AccountObjectImageHash : public PropertyBase<string, AccountObjectImageHash>
   {
   public:
      using meta_unique = bool;
      static string name()
      {
         return "image_hash";
      }
   };
   class AccountObjectGender : public PropertyBase<e_gender, AccountObjectGender>
   {
   public:
      using meta_unique = bool;
      static string name()
      {
         return "gender";
      }

      static void convert_from_string(string const& str_value, e_gender& who)
      {
         if (str_value == "male")
            who = e_gender::male;
         else if (str_value == "female")
            who = e_gender::female;
         else
            who = e_gender::unspecified;
      }
      static void convert_to_string(e_gender const& who, string& str_value)
      {
         switch (who)
         {
            case e_gender::male:
               str_value = "male";
               break;
            case e_gender::female:
               str_value = "female";
               break;
            default:
               str_value = "unspecified";
         }
      }
   };

   /**
    * @brief This class represents an account on the object graph
    * @ingroup object
    * @ingroup protocol
    *
    * Accounts are the primary unit of authority on the graphene system. Users must have an account in order to use
    * assets, trade in the markets, vote for committee_members, etc.
    */
   class account_object : public graphene::db::abstract_object<account_object>
   {
      public:
         static const uint8_t space_id = protocol_ids;
         static const uint8_t type_id  = account_object_type;

         ///The account that paid the fee to register this account. Receives a percentage of referral rewards.
         account_id_type registrar;

         /// The account's name. This name must be unique among all account names on the graph. May not be empty.
         string name;

         authority owner;

         account_options options;

         /// The reference implementation records the account's statistics in a separate object. This field contains the
         /// ID of that object.
         account_statistics_id_type statistics;

         /**
          * Vesting balance which receives cashback_reward deposits.
          */
         optional<vesting_balance_id_type> cashback_vb;

         /**
          * This flag is set when the top_n logic sets both authorities,
          * and gets reset when authority is set.
          */
         uint8_t top_n_control_flags = 0;
         static const uint8_t top_n_control_owner  = 1;
         static const uint8_t top_n_control_active = 2;

         template<typename DB>
         const vesting_balance_object& cashback_balance(const DB& db)const
         {
            FC_ASSERT(cashback_vb);
            return db.get(*cashback_vb);
         }
         account_id_type get_id()const { return id; }
         vote_id_type get_voted_miner() const {
             for (auto && v : options.votes)
                 if(v.type() == vote_id_type::miner)
                     return v;
             return vote_id_type(vote_id_type::miner, std::numeric_limits<uint32_t>::max());
         }

   };

   /**
    *  @brief This secondary index will allow a reverse lookup of all accounts that a particular key or account
    *  is an potential signing authority.
    */
   class account_member_index : public secondary_index
   {
      public:
         virtual void object_inserted( const object& obj ) override;
         virtual void object_removed( const object& obj ) override;
         virtual void about_to_modify( const object& before ) override;
         virtual void object_modified( const object& after  ) override;


         /** given an account or key, map it to the set of accounts that reference it in an active or owner authority */
         map< account_id_type, set<account_id_type> > account_to_account_memberships;
         map< public_key_type, set<account_id_type> > account_to_key_memberships;
         /** some accounts use address authorities in the genesis block */
         

      protected:
         set<account_id_type>  get_account_members( const account_object& a )const;
         set<public_key_type>  get_key_members( const account_object& a )const;
   

         set<account_id_type>  before_account_members;
         set<public_key_type>  before_key_members;
   };


   struct by_account_asset;
   struct by_asset_balance;
   struct by_voted_miner;
   struct by_owner;
   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      account_balance_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_owner>, member< account_balance_object, account_id_type, &account_balance_object::owner> >,
         ordered_unique< tag<by_account_asset>,
            composite_key<
               account_balance_object,
               member<account_balance_object, account_id_type, &account_balance_object::owner>,
               member<account_balance_object, asset_id_type, &account_balance_object::asset_type>
            >
         >,
         ordered_unique< tag<by_asset_balance>,
            composite_key<
               account_balance_object,
               member<account_balance_object, asset_id_type, &account_balance_object::asset_type>,
               member<account_balance_object, share_type, &account_balance_object::balance>,
               member<account_balance_object, account_id_type, &account_balance_object::owner>
            >,
            composite_key_compare<
               std::less< asset_id_type >,
               std::greater< share_type >,
               std::less< account_id_type >
            >
         >
      >
   > account_balance_object_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<account_balance_object, account_balance_object_multi_index_type> account_balance_index;


   struct by_name;

   template <typename TAG, typename _t_object>
   struct key_extractor;

   template <>
   struct key_extractor<by_id, account_object>
   {
      static object_id_type get(account_object const& ob)
      {
         return ob.id;
      }
   };

   template <>
   struct key_extractor<by_name, account_object>
   {
      static std::string get(account_object const& ob)
      {
         return ob.name;
      }
   };

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      account_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_name>, member<account_object, std::string, &account_object::name>  >,
         ordered_non_unique<tag<by_voted_miner>, const_mem_fun<account_object, vote_id_type, &account_object::get_voted_miner> >
      >
   > account_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<account_object, account_multi_index_type> account_index;

}}

FC_REFLECT_DERIVED( graphene::chain::account_object,
                    (graphene::db::object),
                    (registrar)
                    (name)(owner)(options)(statistics)
                    (cashback_vb)(top_n_control_flags)
                    )

FC_REFLECT_DERIVED( graphene::chain::account_balance_object,
                    (graphene::db::object),
                    (owner)(asset_type)(balance)(vote_power) )

FC_REFLECT_DERIVED( graphene::chain::account_statistics_object,
                    (graphene::db::object),
                    (owner)
                    (most_recent_op)
                    (total_ops)
                    (total_core_in_orders)
                    (pending_fees)
                    (pending_vested_fees)
                  )

FC_REFLECT_ENUM( graphene::chain::e_gender,
                 (male)
                 (female)
                 (unspecified))

FC_REFLECT( graphene::chain::account_details,
            (first_name)
            (last_name)
            (image_hash)
            (gender))

