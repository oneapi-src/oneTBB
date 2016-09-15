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

/* Example program that shows how to use parallel_do to do parallel preorder
   traversal of a directed acyclic graph. */

#include <cstdlib>
#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"
#include "../../common/utility/utility.h"
#include <iostream>
#include <vector>
#include "Graph.h"

// some forward declarations
class Cell;
void ParallelPreorderTraversal( const std::vector<Cell*>& root_set );

//------------------------------------------------------------------------
// Test driver
//------------------------------------------------------------------------
utility::thread_number_range threads(tbb::task_scheduler_init::default_num_threads);
static unsigned nodes = 1000;
static unsigned traversals = 500;
static bool SilentFlag = false;

//! Parse the command line.
static void ParseCommandLine( int argc, const char* argv[] ) {
    utility::parse_cli_arguments(
            argc,argv,
            utility::cli_argument_pack()
                //"-h" option for displaying help is present implicitly
                .positional_arg(threads,"n-of-threads",utility::thread_number_range_desc)
                .positional_arg(nodes,"n-of-nodes","number of nodes in the graph.")
                .positional_arg(traversals,"n-of-traversals","number of times to evaluate the graph. Reduce it (e.g. to 100) to shorten example run time\n")
                .arg(SilentFlag,"silent","no output except elapsed time ")
    );
}

int main( int argc, const char* argv[] ) {
    try {
        tbb::tick_count main_start = tbb::tick_count::now();
        ParseCommandLine(argc,argv);

        // Start scheduler with given number of threads.
        for( int p=threads.first; p<=threads.last; p = threads.step(p) ) {
            tbb::tick_count t0 = tbb::tick_count::now();
            tbb::task_scheduler_init init(p);
            srand(2);
            size_t root_set_size = 0;
            {
                Graph g;
                g.create_random_dag(nodes);
                std::vector<Cell*> root_set;
                g.get_root_set(root_set);
                root_set_size = root_set.size();
                for( unsigned int trial=0; trial<traversals; ++trial ) {
                    ParallelPreorderTraversal(root_set);
                }
            }
            tbb::tick_count::interval_t interval = tbb::tick_count::now()-t0;
            if (!SilentFlag){
                std::cout
                    <<interval.seconds()<<" seconds using "<<p<<" threads ("<<root_set_size<<" nodes in root_set)\n";
            }
        }
        utility::report_elapsed_time((tbb::tick_count::now()-main_start).seconds());

        return 0;
    }catch(std::exception& e){
        std::cerr
            << "unexpected error occurred. \n"
            << "error description: "<<e.what()<<std::endl;
        return -1;
    }
}
