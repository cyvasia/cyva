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

#define RFU_OP(N) \
namespace graphene { namespace chain {                                                      \
struct rfu_operation_ ## N : public base_operation                                          \
{                                                                                           \
    struct fee_parameters_type { uint64_t fee = std::numeric_limits<uint64_t>::max(); };    \
    asset            fee;                                                                   \
    account_id_type fee_payer() const { return account_id_type{}; }                         \
    void            validate() const { FC_ASSERT( false ); }                                \
};  }}                                                                                      \
    FC_REFLECT( graphene::chain::rfu_operation_ ## N::fee_parameters_type, (fee) )          \
    FC_REFLECT( graphene::chain::rfu_operation_ ## N, (fee) )                               \


//RFU_OP(1)
//RFU_OP(2)
//RFU_OP(3)
//RFU_OP(4)
//RFU_OP(5)
RFU_OP(6)
RFU_OP(7)
RFU_OP(8)
RFU_OP(9)
RFU_OP(10)
RFU_OP(11)
RFU_OP(12)
RFU_OP(13)
RFU_OP(14)
RFU_OP(15)
RFU_OP(16)
RFU_OP(17)
RFU_OP(18)
RFU_OP(19)
RFU_OP(20)
RFU_OP(21)
RFU_OP(22)
RFU_OP(23)
RFU_OP(24)
RFU_OP(25)
RFU_OP(26)
RFU_OP(27)
RFU_OP(28)
RFU_OP(29)
RFU_OP(30)
RFU_OP(31)
RFU_OP(32)
RFU_OP(33)
RFU_OP(34)
RFU_OP(35)
RFU_OP(36)
RFU_OP(37)
RFU_OP(38)
RFU_OP(39)
RFU_OP(40)
RFU_OP(41)
RFU_OP(42)
RFU_OP(43)
RFU_OP(44)
RFU_OP(45)
RFU_OP(46)
RFU_OP(47)
RFU_OP(48)
RFU_OP(49)
RFU_OP(50)
RFU_OP(51)
RFU_OP(52)
RFU_OP(53)
RFU_OP(54)
RFU_OP(55)
RFU_OP(56)
RFU_OP(57)
RFU_OP(58)
RFU_OP(59)
RFU_OP(60)
RFU_OP(61)
RFU_OP(62)
RFU_OP(63)
RFU_OP(64)

#undef RFU_OP
/*
namespace graphene { namespace chain {
struct rfu_operation : public base_operation
{
    struct fee_parameters_type { uint64_t fee = std::numeric_limits<uint64_t>::max(); };
    asset            fee;
    account_id_type fee_payer() const { return account_id_type{}; }
    void            validate() const { FC_ASSERT( false ); }
};  }}
    FC_REFLECT( graphene::chain::rfu_operation::fee_parameters_type, (fee) )
    FC_REFLECT( graphene::chain::rfu_operation, (fee) )
*/

