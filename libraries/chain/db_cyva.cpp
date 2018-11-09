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

std::array<uint64_t, 72> CYVA_SPLIT = {1052000, 2104000, 3156000, 4208000, 5260000, 6312000, 7364000, 8416000, 9468000, 10520000, 11572000, 12624000, 13676000, 14728000, 15780000, 16832000, 17884000, 18936000, 19988000, 21040000, 22092000, 23144000, 24196000, 25248000, 26300000, 27352000, 28404000, 29456000, 30508000, 31560000, 32612000, 33664000, 34716000, 35768000, 36820000, 37872000, 38924000, 39976000, 41028000, 42080000, 43132000, 44184000, 45236000, 46288000, 47340000, 48392000, 49444000, 50496000, 51548000, 52600000, 53652000, 54704000, 55756000, 56808000, 57860000, 58912000, 59964000, 61016000, 62068000, 63120000, 64172000, 65224000, 66276000, 67328000, 68380000, 69432000, 70484000, 71536000, 72588000, 73640000, 74692000, 75744000};
std::array<uint64_t, 72> CYVA_BLOCK_REWARD = {3802281369, 3041825095, 2433460076, 1946768061, 1557414449, 1245931559, 996745247, 797396198, 637916920, 510333460, 408266730, 326613308, 261290589, 209032414, 167225856, 133780608, 107024430, 85619487, 68495532, 54796388, 43837072, 35069582, 28055608, 22444487, 17955513, 14364354, 11491445, 9193156, 7354468, 5886312, 4709030, 3767205, 3013688, 2410932, 1928707, 1542966, 1234316, 987452, 789924, 631939, 505513, 404373, 323479, 258745, 206939, 165494, 132319, 105798, 84601, 67681, 54087, 43251, 34601, 27662, 22053, 17586, 14068, 11217, 8935, 7129, 5703, 4563, 3612, 2852, 2281, 1806, 1426, 1141, 856, 665, 475, 0};

void database::cyva_housekeeping()
{
}

bool database::is_reward_switch_time() const
{
    auto now = head_block_num();
    if (CYVA_SPLIT.end() != std::find(CYVA_SPLIT.begin(), CYVA_SPLIT.end(), now))
        return true;
    else
        return false;
}

bool database::is_reward_switch_in_interval(uint64_t a, uint64_t b)const
{
    if( a >= b )
        return false;
    auto it_a = std::lower_bound(CYVA_SPLIT.begin(), CYVA_SPLIT.end(), a);
    auto it_b = std::upper_bound(CYVA_SPLIT.begin(), CYVA_SPLIT.end(), b);
    if (std::distance(it_a, it_b) > 0)
        return true;
    return false;
}


uint64_t database::get_next_reward_switch_block(uint64_t start)const
{
    auto it = std::lower_bound(CYVA_SPLIT.begin(), CYVA_SPLIT.end(), start);
    if ( it != CYVA_SPLIT.end())
        return *it;
    return 0;
}



share_type database::get_asset_per_block_by_block_num(uint32_t block_num)
{
    //this method is called AFTER the update of head_block_num in gpo or when user calls get_block.
    //If user calls get_block calculation of miner_reward needs to search backward for block_reward.
    uint64_t block_reward = CYVA_BLOCK_REWARD.back();
    auto it = std::upper_bound(CYVA_SPLIT.begin(), CYVA_SPLIT.end(), block_num);
    if( CYVA_SPLIT.end() != it )
    {
        auto block_reward_i = std::distance(CYVA_SPLIT.begin(), it);
        block_reward = CYVA_BLOCK_REWARD[block_reward_i];
    }

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
        auto it = std::find(CYVA_SPLIT.begin(), CYVA_SPLIT.end(), next_switch);
        if( CYVA_SPLIT.end() != it ){
            auto block_reward_i = std::distance(CYVA_SPLIT.begin(), it);
            auto block_reward = CYVA_BLOCK_REWARD[block_reward_i];
            return get_new_asset_per_block() * to_switch + block_reward * ( blocks_to_maint - to_switch );
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
