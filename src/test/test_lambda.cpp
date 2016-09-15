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

#include "harness_defs.h"
#if __TBB_TEST_SKIP_LAMBDA

#include "harness.h"
int TestMain() {
    REPORT("Known issue: lambdas are not properly supported on the platform \n");
    return Harness::Skipped;
}

#else /*__TBB_TEST_SKIP_LAMBDA*/

#define NOMINMAX
#include "tbb/tbb.h"
#include "tbb/combinable.h"
#include <cstdio>

#if !TBB_USE_EXCEPTIONS && _MSC_VER
    // Suppress "C++ exception handler used, but unwind semantics are not enabled" warning in STL headers
    #pragma warning (push)
    #pragma warning (disable: 4530)
#endif

#include <list>

#if !TBB_USE_EXCEPTIONS && _MSC_VER
    #pragma warning (pop)
#endif

using namespace std;
using namespace tbb;

typedef pair<int,int> max_element_t;

void f(int val, int *arr, int start, int stop) {
    for (int i=start; i<=stop; ++i) {
        arr[i] = val;
    }
}

#include "harness.h"

#if __TBB_TASK_GROUP_CONTEXT
int Fib(int n) {
    if( n<2 ) {
        return n;
    } else {
        int x=0, y=0;
        task_group g;
        g.run( [&]{x=Fib(n-1);} ); // spawn a task
        g.run( [&]{y=Fib(n-2);} ); // spawn another task
        g.wait();                  // wait for both tasks to complete
        return x+y;
    }
}
#endif /* !__TBB_TASK_GROUP_CONTEXT */

#include "harness_report.h"
#include "harness_assert.h"

int TestMain () {
    const int N = 1000;
    const int Grainsize = N/1000;
    int a[N];
    int max_sum;
    ASSERT( MinThread>=1, "Error: Number of threads must be positive.\n");

    for(int p=MinThread; p<=MaxThread; ++p) {
        task_scheduler_init init(p);

        REMARK("Running lambda expression tests on %d threads...\n", p);

        //test parallel_for
        REMARK("Testing parallel_for... ");
        parallel_for(blocked_range<int>(0,N,Grainsize),
                     [&] (blocked_range<int>& r) {
                         for (int i=r.begin(); i!=r.end(); ++i)    a[i] = i;
                     });
        ASSERT(a[0]==0 && a[N-1]==N-1, "parallel_for w/lambdas failed.\n");
        REMARK("passed.\n");

        //test parallel_reduce
        REMARK("Testing parallel_reduce... ");
        int sum = parallel_reduce(blocked_range<int>(0,N,Grainsize), int(0),
                                  [&] (blocked_range<int>& r, int current_sum) -> int {
                                      for (int i=r.begin(); i!=r.end(); ++i)
                                          current_sum += a[i]*(1000-i);
                                      return current_sum;
                                  },
                                  [] (const int x1, const int x2) {
                                      return x1+x2;
                                  } );

        max_element_t max_el =
            parallel_reduce(blocked_range<int>(0,N,Grainsize), make_pair(a[0], 0),
                            [&] (blocked_range<int>& r, max_element_t current_max)
                            -> max_element_t {
                                for (int i=r.begin(); i!=r.end(); ++i)
                                    if (a[i]>current_max.first)
                                        current_max = make_pair(a[i], i);
                                return current_max;
                            },
                            [] (const max_element_t x1, const max_element_t x2) {
                                return (x1.first>x2.first)?x1:x2;
                            });
        ASSERT(sum==166666500 && max_el.first==999 && max_el.second==999,
               "parallel_reduce w/lambdas failed.\n");
        REMARK("passed.\n");

        //test parallel_do
        REMARK("Testing parallel_do... ");
        list<int> s;
        s.push_back(0);

        parallel_do(s.begin(), s.end(),
                    [&](int foo, parallel_do_feeder<int>& feeder) {
                        if (foo == 42) return;
                        else if (foo>42) {
                            s.push_back(foo-3);
                            feeder.add(foo-3);
                        } else {
                            s.push_back(foo+5);
                            feeder.add(foo+5);
                        }
                    });
        ASSERT(s.back()==42, "parallel_do w/lambda failed.\n");
        REMARK("passed.\n");

        //test parallel_invoke
        REMARK("Testing parallel_invoke... ");
        parallel_invoke([&]{ f(2, a, 0, N/3); },
                        [&]{ f(1, a, N/3+1, 2*(N/3)); },
                        [&]{ f(0, a, 2*(N/3)+1, N-1); });
        ASSERT(a[0]==2.0 && a[N-1]==0.0, "parallel_invoke w/lambda failed.\n");
        REMARK("passed.\n");

        //test tbb_thread
        REMARK("Testing tbb_thread... ");
        tbb_thread::id myId;
        tbb_thread myThread([](int x, int y) {
                                ASSERT(x==42 && y==64, "tbb_thread w/lambda failed.\n");
                                REMARK("passed.\n");
                            }, 42, 64);
        myThread.join();

#if __TBB_TASK_GROUP_CONTEXT
        // test task_group
        REMARK("Testing task_group... ");
        int result;
        result = Fib(32);
        ASSERT(result==2178309, "task_group w/lambda failed.\n");
        REMARK("passed.\n");
#endif /* __TBB_TASK_GROUP_CONTEXT */

        // Reset array a to index values
        parallel_for(blocked_range<int>(0,N,Grainsize),
                     [&] (blocked_range<int>& r) {
                         for (int i=r.begin(); i!=r.end(); ++i)    a[i] = i;
                     });
        // test parallel_sort
        REMARK("Testing parallel_sort... ");
        int pivot = 42;

        // sort nearest by increasing distance from pivot
        parallel_sort(a, a+N,
                      [&](int x, int y) { return(abs(pivot-x) < abs(pivot-y)); });
        ASSERT(a[0]==42 && a[N-1]==N-1, "parallel_sort w/lambda failed.\n");
        REMARK("passed.\n");

        //test combinable
        REMARK("Testing combinable... ");
        combinable<std::pair<int,int> > minmax_c([&]() { return std::make_pair(a[0], a[0]); } );

        parallel_for(blocked_range<int>(0,N),
                     [&] (const blocked_range<int> &r) {
                         std::pair<int,int>& mmr = minmax_c.local();
                         for(int i=r.begin(); i!=r.end(); ++i) {
                             if (mmr.first > a[i]) mmr.first = a[i];
                             if (mmr.second < a[i]) mmr.second = a[i];
                         }
                     });
        max_sum = 0;
        minmax_c.combine_each([&max_sum](std::pair<int,int> x) {
                                  int tsum = x.first + x.second;
                                  if( tsum>max_sum ) max_sum = tsum;
                              });
        ASSERT( (N-1)<=max_sum && max_sum<=a[0]+N-1, "combinable::combine_each /w lambda failed." );

        std::pair<int,int> minmax_result_c;
        minmax_result_c =
            minmax_c.combine([](std::pair<int,int> x, std::pair<int,int> y) {
                                 return std::make_pair(x.first<y.first?x.first:y.first,
                                                       x.second>y.second?x.second:y.second);
                             });
        ASSERT(minmax_result_c.first==0 && minmax_result_c.second==999, 
               "combinable w/lambda failed.\n");
        REMARK("passed.\n");

        //test enumerable_thread_specific
        REMARK("Testing enumerable_thread_specific... ");
        enumerable_thread_specific< std::pair<int,int> > minmax_ets([&]() { return std::make_pair(a[0], a[0]); } );

        max_sum = 0;
        parallel_for(blocked_range<int>(0,N),
                     [&] (const blocked_range<int> &r) {
                         std::pair<int,int>& mmr = minmax_ets.local();
                         for(int i=r.begin(); i!=r.end(); ++i) {
                             if (mmr.first > a[i]) mmr.first = a[i];
                             if (mmr.second < a[i]) mmr.second = a[i];
                         }
                     });
        minmax_ets.combine_each([&max_sum](std::pair<int,int> x) {
                                  int tsum = x.first + x.second;
                                  if( tsum>max_sum ) max_sum = tsum;
                                });
        ASSERT( (N-1)<=max_sum && max_sum<=a[0]+N-1, "enumerable_thread_specific::combine_each /w lambda failed." );

        std::pair<int,int> minmax_result_ets;
        minmax_result_ets =
            minmax_ets.combine([](std::pair<int,int> x, std::pair<int,int> y) {
                                   return std::make_pair(x.first<y.first?x.first:y.first,
                                                         x.second>y.second?x.second:y.second);
                               });
        ASSERT(minmax_result_ets.first==0 && minmax_result_ets.second==999,
               "enumerable_thread_specific w/lambda failed.\n");
        REMARK("passed.\n");
    }
    return Harness::Done;
}
#endif /* __TBB_TEST_SKIP_LAMBDA */
