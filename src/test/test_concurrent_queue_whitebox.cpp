/*
    Copyright 2005-2015 Intel Corporation.  All Rights Reserved.

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

#define HARNESS_DEFINE_PRIVATE_PUBLIC 1
#include "harness_inject_scheduler.h"
#define private public
#define protected public
#include "tbb/concurrent_queue.h"
#include "../tbb/concurrent_queue.cpp"
#undef protected
#undef private
#include "harness.h"

#if _MSC_VER==1500 && !__INTEL_COMPILER
    // VS2008/VC9 seems to have an issue; limits pull in math.h
    #pragma warning( push )
    #pragma warning( disable: 4985 )
#endif
#include <limits>
#if _MSC_VER==1500 && !__INTEL_COMPILER
    #pragma warning( pop )
#endif

template <typename Q>
class FloggerBody : NoAssign {
    Q& q;
    size_t elem_num;
public:
    FloggerBody(Q& q_, size_t elem_num_) : q(q_), elem_num(elem_num_) {}
    void operator()(const int threadID) const {
        typedef typename Q::value_type value_type;
        value_type elem = value_type(threadID);
        for (size_t i = 0; i < elem_num; ++i) {
            q.push(elem);
            (void) q.try_pop(elem);
        }
    }
};

template <typename Q>
void TestFloggerHelp(Q& q, size_t items_per_page) {
    size_t nq = q.my_rep->n_queue;
    size_t reserved_elem_num = nq * items_per_page - 1;
    size_t hack_val = std::numeric_limits<std::size_t>::max() & ~reserved_elem_num;
    q.my_rep->head_counter = hack_val;
    q.my_rep->tail_counter = hack_val;
    size_t k = q.my_rep->tail_counter & -(ptrdiff_t)nq;

    for (size_t i=0; i<nq; ++i) {
        q.my_rep->array[i].head_counter = k;
        q.my_rep->array[i].tail_counter = k;
    }
    NativeParallelFor(MaxThread, FloggerBody<Q>(q, reserved_elem_num + 20)); // to induce the overflow occurrence
    ASSERT(q.empty(), "FAILED flogger/empty test.");
    ASSERT(q.my_rep->head_counter < hack_val, "FAILED wraparound test.");
}

template <typename T>
void TestFlogger() {
    {
        tbb::concurrent_queue<T> q;
        REMARK("Wraparound on strict_ppl::concurrent_queue...");
        TestFloggerHelp(q, q.my_rep->items_per_page);
        REMARK(" works.\n");
    }
    {
        tbb::concurrent_bounded_queue<T> q;
        REMARK("Wraparound on tbb::concurrent_bounded_queue...");
        TestFloggerHelp(q, q.items_per_page);
        REMARK(" works.\n");
    }
}

void TestWraparound() {
    REMARK("Testing Wraparound...\n");
    TestFlogger<int>();
    TestFlogger<unsigned char>();
    REMARK("Done Testing Wraparound.\n");
}

int TestMain () {
    TestWraparound();
    return Harness::Done;
}
