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
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

#define RFU_EVAL(N) \
namespace graphene { namespace chain {                                                            \
struct rfu_evaluator_ ## N : public evaluator<rfu_evaluator_ ## N>                                \
{                                                                                                 \
    using operation_type = rfu_operation_ ## N;                                                   \
    void_result do_evaluate( const operation_type& op )                                           \
    {                                                                                             \
          try {  FC_ASSERT(false); return void_result(); }  FC_CAPTURE_AND_RETHROW( (op) )        \
    }                                                                                             \
    void_result do_apply( const operation_type&, const transaction_id_type& )                     \
    {                                                                                             \
        FC_ASSERT(false);                                                                         \
        return void_result();                                                                     \
    }                                                                                             \
};  }}                                                                                            \


//RFU_EVAL(1)
//RFU_EVAL(2)
//RFU_EVAL(3)
//RFU_EVAL(4)
//RFU_EVAL(5)
RFU_EVAL(6)
RFU_EVAL(7)
RFU_EVAL(8)
RFU_EVAL(9)
RFU_EVAL(10)
RFU_EVAL(11)
RFU_EVAL(12)
RFU_EVAL(13)
RFU_EVAL(14)
RFU_EVAL(15)
RFU_EVAL(16)
RFU_EVAL(17)
RFU_EVAL(18)
RFU_EVAL(19)
RFU_EVAL(20)
RFU_EVAL(21)
RFU_EVAL(22)
RFU_EVAL(23)
RFU_EVAL(24)
RFU_EVAL(25)
RFU_EVAL(26)
RFU_EVAL(27)
RFU_EVAL(28)
RFU_EVAL(29)
RFU_EVAL(30)
RFU_EVAL(31)
RFU_EVAL(32)
RFU_EVAL(33)
RFU_EVAL(34)
RFU_EVAL(35)
RFU_EVAL(36)
RFU_EVAL(37)
RFU_EVAL(38)
RFU_EVAL(39)
RFU_EVAL(40)
RFU_EVAL(41)
RFU_EVAL(42)
RFU_EVAL(43)
RFU_EVAL(44)
RFU_EVAL(45)
RFU_EVAL(46)
RFU_EVAL(47)
RFU_EVAL(48)
RFU_EVAL(49)
RFU_EVAL(50)
RFU_EVAL(51)
RFU_EVAL(52)
RFU_EVAL(53)
RFU_EVAL(54)
RFU_EVAL(55)
RFU_EVAL(56)
RFU_EVAL(57)
RFU_EVAL(58)
RFU_EVAL(59)
RFU_EVAL(60)
RFU_EVAL(61)
RFU_EVAL(62)
RFU_EVAL(63)
RFU_EVAL(64)

#undef RFU_EVAL
/*
namespace graphene { namespace chain {

   class rfu_evaluator : public evaluator<rfu_evaluator>
   {
   public:
      using operation_type = rfu_operation;
      void_result do_evaluate( const operation_type& o );
      void_result do_apply( const operation_type& o );
   };

} } // graphene::chain
*/
