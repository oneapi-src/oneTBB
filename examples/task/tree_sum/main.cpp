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

#include "common.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

// The performance of this example can be significantly better when
// the objects are allocated by the scalable_allocator instead of the
// default "operator new".  The reason is that the scalable_allocator
// typically packs small objects more tightly than the default "operator new",
// resulting in a smaller memory footprint, and thus more efficient use of
// cache and virtual memory.  Also the scalable_allocator works faster for
// multi-threaded allocations.
//
// Pass stdmalloc as the 1st command line parameter to use the default "operator new"
// and see the performance difference.
#include "tbb/scalable_allocator.h"
#include "TreeMaker.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"

#include "../../common/utility/utility.h"

using namespace std;

void Run( const char* which, Value(*SumTree)(TreeNode*), TreeNode* root, bool silent) {
    tbb::tick_count t0;
    if ( !silent ) t0 = tbb::tick_count::now();
    Value result = SumTree(root);
    if ( !silent ) printf ("%24s: time = %.1f msec, sum=%g\n", which, (tbb::tick_count::now()-t0).seconds()*1000, result);
}

int main( int argc, const char *argv[] ) {
    try{
        tbb::tick_count mainStartTime = tbb::tick_count::now();

        // The 1st argument is the function to obtain 'auto' value; the 2nd is the default value
        // The example interprets 0 threads as "run serially, then fully subscribed"
        utility::thread_number_range threads( tbb::task_scheduler_init::default_num_threads, 0 );
        long number_of_nodes = 10000000;
        bool silent = false;
        bool use_stdmalloc = false;

        utility::parse_cli_arguments(argc,argv,
            utility::cli_argument_pack()
            //"-h" option for displaying help is present implicitly
            .positional_arg(threads,"n-of-threads",utility::thread_number_range_desc)
            .positional_arg(number_of_nodes,"number-of-nodes","the number of nodes")
            .arg(silent,"silent","no output except elapsed time")
            .arg(use_stdmalloc,"stdmalloc","use standard allocator")
        );

        TreeNode* root;
        { // In this scope, TBB will use default number of threads for tree creation
            tbb::task_scheduler_init init;

            if( use_stdmalloc ) {
                if ( !silent ) printf("Tree creation using standard operator new\n");
                root = TreeMaker<stdmalloc>::create_and_time( number_of_nodes, silent );
            } else {
                if ( !silent ) printf("Tree creation using TBB scalable allocator\n");
                root = TreeMaker<tbbmalloc>::create_and_time( number_of_nodes, silent );
            }
        }

        // Warm up caches
        SerialSumTree(root);
        if ( !silent ) printf("Calculations:\n");
        if ( threads.first ) {
            for(int p = threads.first;  p <= threads.last; p = threads.step(p) ) {
                if ( !silent ) printf("threads = %d\n", p );
                tbb::task_scheduler_init init( p );
                Run ( "SimpleParallelSumTree", SimpleParallelSumTree, root, silent );
                Run ( "OptimizedParallelSumTree", OptimizedParallelSumTree, root, silent );
            }
        } else { // Number of threads wasn't set explicitly. Run serial and two parallel versions
            Run ( "SerialSumTree", SerialSumTree, root, silent );
            tbb::task_scheduler_init init;
            Run ( "SimpleParallelSumTree", SimpleParallelSumTree, root, silent );
            Run ( "OptimizedParallelSumTree", OptimizedParallelSumTree, root, silent );
        }
        utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());
        return 0;
    }catch(std::exception& e){
        std::cerr<<"error occurred. error text is :\"" <<e.what()<<"\"\n";
        return 1;
    }
}
