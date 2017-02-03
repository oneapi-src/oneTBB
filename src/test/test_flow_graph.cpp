/*
    Copyright (c) 2005-2016 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.




*/

#include "harness_graph.h"
#include "harness_barrier.h"
#include "tbb/task_scheduler_init.h"

const int T = 4;
const int W = 4;

struct decrement_wait : NoAssign {

    tbb::flow::graph * const my_graph;
    bool * const my_done_flag;

    decrement_wait( tbb::flow::graph &h, bool *done_flag ) : my_graph(&h), my_done_flag(done_flag) {}

    void operator()(int i) const {
        Harness::Sleep(10*i);
        my_done_flag[i] = true;
        my_graph->decrement_wait_count();
    }
};

static void test_wait_count() {
   tbb::flow::graph h;
   for (int i = 0; i < T; ++i ) {
       bool done_flag[W];
       for (int j = 0; j < W; ++j ) {
           for ( int w = 0; w < W; ++w ) done_flag[w] = false;
           for ( int w = 0; w < j; ++w ) h.increment_wait_count();

           NativeParallelFor( j, decrement_wait(h, done_flag) );
           h.wait_for_all();
           for ( int w = 0; w < W; ++w ) {
              if ( w < j ) ASSERT( done_flag[w] == true, NULL );
              else ASSERT( done_flag[w] == false, NULL );
           }
       }
   }
}

const int F = 100;

#if __TBB_LAMBDAS_PRESENT
bool lambda_flag[F];
#endif
bool functor_flag[F];

struct set_functor {
    int my_i;
    set_functor( int i ) : my_i(i) {}
    void operator()() { functor_flag[my_i] = true; }
};

struct return_functor {
    int my_i;
    return_functor( int i ) : my_i(i) {}
    int operator()() { return my_i; }
};

static void test_run() {
    tbb::flow::graph h;
    for (int i = 0; i < T; ++i ) {

        // Create receivers and flag arrays
        #if __TBB_LAMBDAS_PRESENT
        harness_mapped_receiver<int> lambda_r;
        lambda_r.initialize_map( F, 1 );
        #endif
        harness_mapped_receiver<int> functor_r;
        functor_r.initialize_map( F, 1 );

        // Initialize flag arrays
        for (int j = 0; j < F; ++j ) {
            #if __TBB_LAMBDAS_PRESENT
            lambda_flag[j] = false;
            #endif
            functor_flag[j] = false;
        }

        for ( int j = 0; j < F; ++j ) {
            #if __TBB_LAMBDAS_PRESENT
                h.run( [=]() { lambda_flag[j] = true; } );
                h.run( lambda_r, [=]() { return j; } );
            #endif
            h.run( set_functor(j) );
            h.run( functor_r, return_functor(j) );
        }
        h.wait_for_all();
        for ( int j = 0; j < F; ++j ) {
        #if __TBB_LAMBDAS_PRESENT
            ASSERT( lambda_flag[i] == true, NULL );
        #endif
            ASSERT( functor_flag[i] == true, NULL );
        }
        #if __TBB_LAMBDAS_PRESENT
        lambda_r.validate();
        #endif
        functor_r.validate();
    }
}

// Encapsulate object we want to store in vector (because contained type must have
// copy constructor and assignment operator
class my_int_buffer {
    tbb::flow::buffer_node<int> *b;
    tbb::flow::graph& my_graph;
public:
    my_int_buffer(tbb::flow::graph &g) : my_graph(g) { b = new tbb::flow::buffer_node<int>(my_graph); }
    my_int_buffer(const my_int_buffer& other) : my_graph(other.my_graph) {
        b = new tbb::flow::buffer_node<int>(my_graph);
    }
    ~my_int_buffer() { delete b; }
    my_int_buffer& operator=(const my_int_buffer& /*other*/) {
        return *this;
    }
};

// test the graph iterator, delete nodes from graph, test again
void test_iterator() {
   tbb::flow::graph g;
   my_int_buffer a_buffer(g);
   my_int_buffer b_buffer(g);
   my_int_buffer c_buffer(g);
   my_int_buffer *d_buffer = new my_int_buffer(g);
   my_int_buffer e_buffer(g);
   std::vector< my_int_buffer > my_buffer_vector(10, c_buffer);

   int count = 0;
   for (tbb::flow::graph::iterator it = g.begin(); it != g.end(); ++it) {
       count++;
   }
   ASSERT(count==15, "error in iterator count");

   delete d_buffer;

   count = 0;
   for (tbb::flow::graph::iterator it = g.begin(); it != g.end(); ++it) {
       count++;
   }
   ASSERT(count==14, "error in iterator count");

   my_buffer_vector.clear();

   count = 0;
   for (tbb::flow::graph::iterator it = g.begin(); it != g.end(); ++it) {
       count++;
   }
   ASSERT(count==4, "error in iterator count");
}

class AddRemoveBody : NoAssign {
    tbb::flow::graph& g;
    int nThreads;
    Harness::SpinBarrier &barrier;
public:
    AddRemoveBody(int nthr, Harness::SpinBarrier &barrier_, tbb::flow::graph& _g) :
        g(_g), nThreads(nthr), barrier(barrier_)
    {}
    void operator()(const int /*threadID*/) const {
        my_int_buffer b(g);
        {
            std::vector<my_int_buffer> my_buffer_vector(100, b);
            barrier.wait();  // wait until all nodes are created
            // now test that the proper number of nodes were created
            int count = 0;
            for (tbb::flow::graph::iterator it = g.begin(); it != g.end(); ++it) {
                count++;
            }
            ASSERT(count==101*nThreads, "error in iterator count");
            barrier.wait();  // wait until all threads are done counting
        } // all nodes but for the initial node on this thread are deleted
        barrier.wait(); // wait until all threads have deleted all nodes in their vectors
        // now test that all the nodes were deleted except for the initial node
        int count = 0;
        for (tbb::flow::graph::iterator it = g.begin(); it != g.end(); ++it) {
            count++;
        }
        ASSERT(count==nThreads, "error in iterator count");
        barrier.wait();  // wait until all threads are done counting
    } // initial node gets deleted
};

void test_parallel(int nThreads) {
    tbb::flow::graph g;
    Harness::SpinBarrier barrier(nThreads);
    AddRemoveBody body(nThreads, barrier, g);
    NativeParallelFor(nThreads, body);
}

int TestMain() {
    if( MinThread<1 ) {
        REPORT("number of threads must be positive\n");
        exit(1);
    }
    for( int p=MinThread; p<=MaxThread; ++p ) {
       tbb::task_scheduler_init init(p);
       test_wait_count();
       test_run();
       test_iterator();
       test_parallel(p);
   }
   return Harness::Done;
}

