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

// have to expose the reset_node method to be able to reset a function_body
#include "harness.h"
#include "harness_graph.h"
#include "tbb/flow_graph.h"
#include "tbb/task.h"
#include "tbb/task_scheduler_init.h"

const int N = 1000;

template< typename T >
class test_push_receiver : public tbb::flow::receiver<T> {

    tbb::atomic<int> my_counters[N];

public:

    test_push_receiver() {
        for (int i = 0; i < N; ++i )
            my_counters[i] = 0;
    }

    int get_count( int i ) {
       int v = my_counters[i];
       return v;
    }

    typedef tbb::flow::sender<T> predecessor_type;

#if TBB_PREVIEW_FLOW_GRAPH_FEATURES
    typedef typename tbb::flow::receiver<T>::built_predecessors_type built_predecessors_type;
    typedef typename tbb::flow::receiver<T>::predecessor_list_type predecessor_list_type;
    built_predecessors_type bpt;
    built_predecessors_type &built_predecessors() { return bpt; }
    void internal_add_built_predecessor( predecessor_type & ) { }
    void internal_delete_built_predecessor( predecessor_type & ) { }
    void copy_predecessors( predecessor_list_type & ) { }
    size_t predecessor_count() { return 0; }
#endif

    tbb::task *try_put_task( const T &v ) {
       int i = (int)v;
       ++my_counters[i];
       return const_cast<tbb::task *>(tbb::flow::interface8::SUCCESSFULLY_ENQUEUED);
    }



    /*override*/void reset_receiver(tbb::flow::reset_flags /*f*/) {}
};

template< typename T >
class source_body {

   tbb::atomic<int> my_count;
   int *ninvocations;

public:

   source_body() : ninvocations(NULL) { my_count = 0; }
   source_body(int &_inv) : ninvocations(&_inv)  { my_count = 0; }

   bool operator()( T &v ) {
      v = (T)my_count.fetch_and_increment();
      if(ninvocations) ++(*ninvocations);
      if ( (int)v < N )
         return true;
      else
         return false;
   }

};

template< typename T >
class function_body {

    tbb::atomic<int> *my_counters;

public:

    function_body( tbb::atomic<int> *counters ) : my_counters(counters) {
        for (int i = 0; i < N; ++i )
            my_counters[i] = 0;
    }

    bool operator()( T v ) {
        ++my_counters[(int)v];
        return true;
    }

};

template< typename T >
void test_single_dest() {

   // push only
   tbb::flow::graph g;
   tbb::flow::source_node<T> src(g, source_body<T>() );
   test_push_receiver<T> dest;
   tbb::flow::make_edge( src, dest );
   g.wait_for_all();
   for (int i = 0; i < N; ++i ) {
       ASSERT( dest.get_count(i) == 1, NULL );
   }

   // push only
   tbb::atomic<int> counters3[N];
   tbb::flow::source_node<T> src3(g, source_body<T>() );
   function_body<T> b3( counters3 );
   tbb::flow::function_node<T,bool> dest3(g, tbb::flow::unlimited, b3 );
   tbb::flow::make_edge( src3, dest3 );
   g.wait_for_all();
   for (int i = 0; i < N; ++i ) {
       int v = counters3[i];
       ASSERT( v == 1, NULL );
   }

   // push & pull
   tbb::flow::source_node<T> src2(g, source_body<T>() );
   tbb::atomic<int> counters2[N];
   function_body<T> b2( counters2 );
   tbb::flow::function_node<T,bool> dest2(g, tbb::flow::serial, b2 );
   tbb::flow::make_edge( src2, dest2 );
#if TBB_PREVIEW_FLOW_GRAPH_FEATURES
   ASSERT(src2.successor_count() == 1, NULL);
   typename tbb::flow::source_node<T>::successor_list_type my_succs;
   src2.copy_successors(my_succs);
   ASSERT(my_succs.size() == 1, NULL);
#endif
   g.wait_for_all();
   for (int i = 0; i < N; ++i ) {
       int v = counters2[i];
       ASSERT( v == 1, NULL );
   }

   // test copy constructor
   tbb::flow::source_node<T> src_copy(src);
   test_push_receiver<T> dest_c;
   ASSERT( src_copy.register_successor(dest_c), NULL );
   g.wait_for_all();
   for (int i = 0; i < N; ++i ) {
       ASSERT( dest_c.get_count(i) == 1, NULL );
   }
}

void test_reset() {
    //    source_node -> function_node
    tbb::flow::graph g;
    tbb::atomic<int> counters3[N];
    tbb::flow::source_node<int> src3(g, source_body<int>() );
    tbb::flow::source_node<int> src_inactive(g, source_body<int>(), /*active*/ false );
    function_body<int> b3( counters3 );
    tbb::flow::function_node<int,bool> dest3(g, tbb::flow::unlimited, b3 );
    tbb::flow::make_edge( src3, dest3 );
    //    source_node already in active state.  Let the graph run,
    g.wait_for_all();
    //    check the array for each value.
    for (int i = 0; i < N; ++i ) {
        int v = counters3[i];
        ASSERT( v == 1, NULL );
        counters3[i] = 0;
    }
    g.reset(tbb::flow::rf_reset_bodies);  // <-- re-initializes the counts.
    // and spawns task to run source
    g.wait_for_all();
    //    check output queue again.  Should be the same contents.
    for (int i = 0; i < N; ++i ) {
        int v = counters3[i];
        ASSERT( v == 1, NULL );
        counters3[i] = 0;
    }
    g.reset();  // doesn't reset the source_node_body to initial state, but does spawn a task
                // to run the source_node.

    g.wait_for_all();
    // array should be all zero
    for (int i = 0; i < N; ++i ) {
        int v = counters3[i];
        ASSERT( v == 0, NULL );
    }

    remove_edge(src3, dest3);
    make_edge(src_inactive, dest3);

    // src_inactive doesn't run
    g.wait_for_all();
    for (int i = 0; i < N; ++i ) {
        int v = counters3[i];
        ASSERT( v == 0, NULL );
    }

    // run graph
    src_inactive.activate();
    g.wait_for_all();
    // check output
    for (int i = 0; i < N; ++i ) {
        int v = counters3[i];
        ASSERT( v == 1, NULL );
        counters3[i] = 0;
    }
    g.reset(tbb::flow::rf_reset_bodies);  // <-- reinitializes the counts
    // src_inactive doesn't run
    g.wait_for_all();
    for (int i = 0; i < N; ++i ) {
        int v = counters3[i];
        ASSERT( v == 0, NULL );
    }

    // start it up
    src_inactive.activate();
    g.wait_for_all();
    for (int i = 0; i < N; ++i ) {
        int v = counters3[i];
        ASSERT( v == 1, NULL );
        counters3[i] = 0;
    }
    g.reset();  // doesn't reset the source_node_body to initial state, and doesn't
                // spawn a task to run the source_node.

    g.wait_for_all();
    // array should be all zero
    for (int i = 0; i < N; ++i ) {
        int v = counters3[i];
        ASSERT( v == 0, NULL );
    }
    src_inactive.activate();
    // source_node_body is already in final state, so source_node will not forward a message.
    g.wait_for_all();
    for (int i = 0; i < N; ++i ) {
        int v = counters3[i];
        ASSERT( v == 0, NULL );
    }
}

#if TBB_PREVIEW_FLOW_GRAPH_FEATURES
void test_extract() {
    int counts = 0;
    tbb::flow::tuple<int,int> dont_care;
    tbb::flow::graph g;
    typedef tbb::flow::source_node<int> snode_type;
    typedef snode_type::successor_list_type successor_list_type;
    snode_type s0(g, source_body<int>(counts), /*is_active*/false );
    tbb::flow::join_node< tbb::flow::tuple<int,int>, tbb::flow::reserving > j0(g);
    tbb::flow::join_node< tbb::flow::tuple<int,int>, tbb::flow::reserving > j1(g);
    tbb::flow::join_node< tbb::flow::tuple<int,int>, tbb::flow::reserving > j2(g);
    tbb::flow::queue_node<int> q0(g);
    tbb::flow::queue_node<tbb::flow::tuple<int,int> > q1(g);
    tbb::flow::make_edge(s0, tbb::flow::get<0>(j0.input_ports()));
    /*  s0 ----+    */
    /*         | j0 */
    /*         +    */
    ASSERT(!counts, "source_node activated too soon");
    s0.activate();
    g.wait_for_all();  // should produce one value, buffer it.
    ASSERT(counts == 1, "source_node did not react to activation");

    g.reset(tbb::flow::rf_reset_bodies);
    counts = 0;
    s0.extract();
    /*  s0     +    */
    /*         | j0 */
    /*         +    */
    s0.activate();
    g.wait_for_all();  // no successors, so the body will not execute
    ASSERT(counts == 0, "source_node shouldn't forward (no successors)");
    g.reset(tbb::flow::rf_reset_bodies);

    tbb::flow::make_edge(s0, tbb::flow::get<0>(j0.input_ports()));
    tbb::flow::make_edge(s0, tbb::flow::get<0>(j1.input_ports()));
    tbb::flow::make_edge(s0, tbb::flow::get<0>(j2.input_ports()));

    /*        /+    */
    /*       / | j0 */
    /*      /  +    */
    /*     /        */
    /*    / /--+    */
    /*  s0-/   | j1 */
    /*    \    +    */
    /*     \        */
    /*      \--+    */
    /*         | j2 */
    /*         +    */

    // do all joins appear in successor list?
    successor_list_type jv1;
    jv1.push_back(&(tbb::flow::get<0>(j0.input_ports())));
    jv1.push_back(&(tbb::flow::get<0>(j1.input_ports())));
    jv1.push_back(&(tbb::flow::get<0>(j2.input_ports())));
    snode_type::successor_list_type sv;
    s0.copy_successors(sv);
    ASSERT(lists_match(sv, jv1), "mismatch in successor list");

    tbb::flow::make_edge(q0, tbb::flow::get<1>(j2.input_ports()));
    tbb::flow::make_edge(j2, q1);
    s0.activate();

    /*        /+           */
    /*       / | j0        */
    /*      /  +           */
    /*     /               */
    /*    / /--+           */
    /*  s0-/   | j1        */
    /*    \    +           */
    /*     \               */
    /*      \--+           */
    /*         | j2----q1  */
    /*  q0-----+           */

    q0.try_put(1);
    g.wait_for_all();
    ASSERT(q1.try_get(dont_care), "join did not emit result");
    j2.extract();
    tbb::flow::make_edge(q0, tbb::flow::get<1>(j2.input_ports()));
    tbb::flow::make_edge(j2, q1);

    /*        /+           */
    /*       / | j0        */
    /*      /  +           */
    /*     /               */
    /*    / /--+           */
    /*  s0-/   | j1        */
    /*         +           */
    /*                     */
    /*         +           */
    /*         | j2----q1  */
    /*  q0-----+           */

    jv1.clear();
    jv1.push_back(&(tbb::flow::get<0>(j0.input_ports())));
    jv1.push_back(&(tbb::flow::get<0>(j1.input_ports())));
    s0.copy_successors(sv);
    ASSERT(lists_match(sv, jv1), "mismatch in successor list");

    q0.try_put(1);
    g.wait_for_all();
    ASSERT(!q1.try_get(dont_care), "extract of successor did not remove pred link");

    s0.extract();

    /*         +           */
    /*         | j0        */
    /*         +           */
    /*                     */
    /*         +           */
    /*  s0     | j1        */
    /*         +           */
    /*                     */
    /*         +           */
    /*         | j2----q1  */
    /*  q0-----+           */

    ASSERT(s0.successor_count() == 0, "successor list not cleared");
    s0.copy_successors(sv);
    ASSERT(sv.size() == 0, "non-empty successor list");

    tbb::flow::make_edge(s0, tbb::flow::get<0>(j2.input_ports()));

    /*         +           */
    /*         | j0        */
    /*         +           */
    /*                     */
    /*         +           */
    /*  s0     | j1        */
    /*    \    +           */
    /*     \               */
    /*      \--+           */
    /*         | j2----q1  */
    /*  q0-----+           */

    jv1.clear();
    jv1.push_back(&(tbb::flow::get<0>(j2.input_ports())));
    s0.copy_successors(sv);
    ASSERT(lists_match(sv, jv1), "mismatch in successor list");

    q0.try_put(1);
    g.wait_for_all();
    ASSERT(!q1.try_get(dont_care), "extract of successor did not remove pred link");
}
#endif  /* TBB_PREVIEW_FLOW_GRAPH_FEATURES */

int TestMain() {
    if( MinThread<1 ) {
        REPORT("number of threads must be positive\n");
        exit(1);
    }
    for ( int p = MinThread; p < MaxThread; ++p ) {
        tbb::task_scheduler_init init(p);
        test_single_dest<int>();
        test_single_dest<float>();
    }
    test_reset();
#if TBB_PREVIEW_FLOW_GRAPH_FEATURES
    test_extract();
#endif
    return Harness::Done;
}

