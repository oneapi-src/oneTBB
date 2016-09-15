/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

#ifndef __TBB_test_range_based_for_H
#define __TBB_test_range_based_for_H

#include <utility> //for std::pair
namespace range_based_for_support_tests{

    template<typename value_type, typename container, typename binary_op_type, typename init_value_type>
    inline init_value_type range_based_for_accumulate(container const& c, binary_op_type accumulator, init_value_type init )
    {
        init_value_type range_for_accumulated = init;
        #if __TBB_RANGE_BASED_FOR_PRESENT
        for (value_type  x : c) {
            range_for_accumulated = accumulator(range_for_accumulated, x);
        }
        #else
        for (typename container::const_iterator x =c.begin(); x != c.end(); ++x) {
            range_for_accumulated = accumulator(range_for_accumulated, *x);
        }
        #endif
        return range_for_accumulated;
    }

    template<typename container, typename binary_op_type, typename init_value_type>
    inline init_value_type range_based_for_accumulate(container const& c, binary_op_type accumulator, init_value_type init )
    {
        typedef typename container::value_type value_type;
        return range_based_for_accumulate<value_type>(c,accumulator,init);
    }

    template <typename integral_type >
    integral_type gauss_summ_of_int_sequence(integral_type sequence_length){
        return (sequence_length +1)* sequence_length /2;
    }

    struct pair_second_summer{
        template<typename first_type, typename second_type>
        second_type operator() (second_type const& lhs, std::pair<first_type, second_type> const& rhs) const
        {
            return lhs + rhs.second;
        }
    };
}

#endif /* __TBB_test_range_based_for_H */
