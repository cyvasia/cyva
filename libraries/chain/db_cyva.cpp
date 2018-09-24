/* (c) 2018 CYVA Net. For details refers to LICENSE.txt */
/*
 * Copyright (c) 2017 CYVA Net and contributors.
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


#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>

#include <algorithm>

namespace graphene { namespace chain {

void database::cyva_housekeeping()
{
}

bool database::is_reward_switch_time() const
{
   auto now = head_block_num();
   return ( now == CYVA_SPLIT_0 || now == CYVA_SPLIT_1 || now == CYVA_SPLIT_2 || now == CYVA_SPLIT_3 || now == CYVA_SPLIT_4 );
}

bool database::is_reward_switch_in_interval(uint64_t a, uint64_t b)const
{
   if(a>=b)
      return false;
   if (a <= CYVA_SPLIT_0 && b >= CYVA_SPLIT_0)
      return true;
   if (a <= CYVA_SPLIT_1 && b >= CYVA_SPLIT_1)
      return true;
   if (a <= CYVA_SPLIT_2 && b >= CYVA_SPLIT_2)
      return true;
   if (a <= CYVA_SPLIT_3 && b >= CYVA_SPLIT_3)
      return true;
   if (a <= CYVA_SPLIT_4 && b >= CYVA_SPLIT_4)
      return true;
   return false;
}

uint64_t database::get_next_reward_switch_block(uint64_t start)const
{
   if(start <= CYVA_SPLIT_0 )
      return CYVA_SPLIT_0;
   if(start <= CYVA_SPLIT_1 )
      return CYVA_SPLIT_1;
   if(start <= CYVA_SPLIT_2 )
      return CYVA_SPLIT_2;
   if(start <= CYVA_SPLIT_3 )
      return CYVA_SPLIT_3;
   if(start <= CYVA_SPLIT_4 )
      return CYVA_SPLIT_4;
   return 0;
}

share_type database::get_asset_per_block_by_block_num(uint32_t block_num)
{
   //this method is called AFTER the update of head_block_num in gpo or when user calls get_block.
   //If user calls get_block calculation of miner_reward needs to search backward for block_reward. 
   uint64_t block_reward;
   if (block_num < CYVA_SPLIT_0)
      block_reward = CYVA_BLOCK_REWARD_0;
   else if (block_num < CYVA_SPLIT_1)
      block_reward = CYVA_BLOCK_REWARD_1;
   else if (block_num < CYVA_SPLIT_2)
      block_reward = CYVA_BLOCK_REWARD_2;
   else if (block_num < CYVA_SPLIT_3)
      block_reward = CYVA_BLOCK_REWARD_3;
   else if (block_num < CYVA_SPLIT_4)
      block_reward = CYVA_BLOCK_REWARD_4;
   else
      block_reward = CYVA_BLOCK_REWARD_5;

   return block_reward;
}

share_type database::get_new_asset_per_block()
{
   //get age in blocks
   auto now = head_block_num();

   return get_asset_per_block_by_block_num(now);
}

share_type database::get_miner_budget(uint32_t blocks_to_maint)
{

   const global_property_object& gpo = get_global_properties();

   uint64_t next_switch = get_next_reward_switch_block( head_block_num() );
   if( head_block_num()+1 + blocks_to_maint >= next_switch )
   {
      uint64_t to_switch = next_switch - head_block_num() - 1;
      if( next_switch == CYVA_SPLIT_0 ){
         return get_new_asset_per_block() * to_switch + CYVA_BLOCK_REWARD_1 * ( blocks_to_maint - to_switch );
      }
      if( next_switch == CYVA_SPLIT_1 ) {
         return get_new_asset_per_block() * to_switch + CYVA_BLOCK_REWARD_2 * ( blocks_to_maint - to_switch );
      }
      if( next_switch == CYVA_SPLIT_2 ) {
         return get_new_asset_per_block() * to_switch + CYVA_BLOCK_REWARD_3 * ( blocks_to_maint - to_switch );
      }
      if( next_switch == CYVA_SPLIT_3 ) {
         return get_new_asset_per_block() * to_switch + CYVA_BLOCK_REWARD_4 * ( blocks_to_maint - to_switch );
      }
      if( next_switch == CYVA_SPLIT_4 ) {
         return get_new_asset_per_block() * to_switch + CYVA_BLOCK_REWARD_5 * ( blocks_to_maint - to_switch );
      }
      return get_new_asset_per_block() * to_switch + get_new_asset_per_block() / 2 * ( blocks_to_maint - to_switch );
   }

   return blocks_to_maint * get_new_asset_per_block();
}

real_supply database::get_real_supply()const
{
   //walk through account_balances, vesting_balances and escrows in content and buying objects
   real_supply total;
   const auto& abidx = get_index_type<account_balance_index>().indices().get<by_id>();
   auto abitr = abidx.begin();
   while( abitr != abidx.end() ){
      total.account_balances += abitr->balance;
      ++abitr;
   }

   const auto& vbidx = get_index_type<vesting_balance_index>().indices().get<by_id>();
   auto vbitr = vbidx.begin();
   while( vbitr != vbidx.end() ){
      total.vesting_balances += vbitr->balance.amount;
      ++vbitr;
   }

   return total;
}

}
}
