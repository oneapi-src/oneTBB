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

#include "harness.h"
#include "tbb/flow_graph.h"
#include "tbb/task.h"

#include "tbb/atomic.h"

const int N = 1000;
const int R = 4;

class int_convertable_type : private NoAssign {

   int my_value;

public:

   int_convertable_type( int v ) : my_value(v) {}
   operator int() const { return my_value; }

};


template< typename T >
class counting_array_receiver : public tbb::flow::receiver<T> {

    tbb::atomic<size_t> my_counters[N];

public:

    counting_array_receiver() {
        for (int i = 0; i < N; ++i )
           my_counters[i] = 0;
    }

    size_t operator[]( int i ) {
        size_t v = my_counters[i];
        return v;
    }

    /* override */ tbb::task * try_put_task( const T &v ) {
        ++my_counters[(int)v];
        return const_cast<tbb::task *>(tbb::flow::interface8::SUCCESSFULLY_ENQUEUED);
    }

#if TBB_PREVIEW_FLOW_GRAPH_FEATURES
    typedef typename tbb::flow::receiver<T>::built_predecessors_type built_predecessors_type;
    built_predecessors_type mbp;
    /*override*/ built_predecessors_type &built_predecessors() { return mbp; }
    typedef typename tbb::flow::receiver<T>::predecessor_list_type predecessor_list_type;
    /*override*/void internal_add_built_predecessor(tbb::flow::sender<T> &) {}
    /*override*/void internal_delete_built_predecessor(tbb::flow::sender<T> &) {}
    /*override*/void copy_predecessors(predecessor_list_type &) {}
    /*override*/size_t predecessor_count() { return 0; }
    /*override*/void clear_predecessors() { }
#endif
    /*override*/void reset_receiver(tbb::flow::reset_flags /*f*/) { }

};

template< typename T >
void test_serial_broadcasts() {

    tbb::flow::graph g;
    tbb::flow::broadcast_node<T> b(g);

    for ( int num_receivers = 1; num_receivers < R; ++num_receivers ) {
        counting_array_receiver<T> *receivers = new counting_array_receiver<T>[num_receivers];
#if TBB_PREVIEW_FLOW_GRAPH_FEATURES
        ASSERT(b.successor_count() == 0, NULL);
        ASSERT(b.predecessor_count() == 0, NULL);
        typename tbb::flow::broadcast_node<T>::successor_list_type my_succs;
        b.copy_successors(my_succs);
        ASSERT(my_succs.size() == 0, NULL);
        typename tbb::flow::broadcast_node<T>::predecessor_list_type my_preds;
        b.copy_predecessors(my_preds);
        ASSERT(my_preds.size() == 0, NULL);
#endif

        for ( int r = 0; r < num_receivers; ++r ) {
            tbb::flow::make_edge( b, receivers[r] );
        }
#if TBB_PREVIEW_FLOW_GRAPH_FEATURES
        ASSERT( b.successor_count() == (size_t)num_receivers, NULL);
#endif

        for (int n = 0; n < N; ++n ) {
            ASSERT( b.try_put( (T)n ), NULL );
        }

        for ( int r = 0; r < num_receivers; ++r ) {
            for (int n = 0; n < N; ++n ) {
                ASSERT( receivers[r][n] == 1, NULL );
            }
            tbb::flow::remove_edge( b, receivers[r] );
        }
        ASSERT( b.try_put( (T)0 ), NULL );
        for ( int r = 0; r < num_receivers; ++r )
            ASSERT( receivers[0][0] == 1, NULL ) ;

        delete [] receivers;

    }

}

template< typename T >
class native_body : private NoAssign {

    tbb::flow::broadcast_node<T> &my_b;

public:

    native_body( tbb::flow::broadcast_node<T> &b ) : my_b(b) {}

    void operator()(int) const {
        for (int n = 0; n < N; ++n ) {
            ASSERT( my_b.try_put( (T)n ), NULL );
        }
    }

};

template< typename T >
void run_parallel_broadcasts(int p, tbb::flow::broadcast_node<T>& b) {
    for ( int num_receivers = 1; num_receivers < R; ++num_receivers ) {
        counting_array_receiver<T> *receivers = new counting_array_receiver<T>[num_receivers];

        for ( int r = 0; r < num_receivers; ++r ) {
            tbb::flow::make_edge( b, receivers[r] );
        }

        NativeParallelFor( p, native_body<T>( b ) );

        for ( int r = 0; r < num_receivers; ++r ) {
            for (int n = 0; n < N; ++n ) {
                ASSERT( (int)receivers[r][n] == p, NULL );
            }
            tbb::flow::remove_edge( b, receivers[r] );
        }
        ASSERT( b.try_put( (T)0 ), NULL );
        for ( int r = 0; r < num_receivers; ++r )
            ASSERT( (int)receivers[r][0] == p, NULL ) ;

        delete [] receivers;

    }
}

template< typename T >
void test_parallel_broadcasts(int p) {

    tbb::flow::graph g;
    tbb::flow::broadcast_node<T> b(g);
    run_parallel_broadcasts(p, b);

    // test copy constructor
    tbb::flow::broadcast_node<T> b_copy(b);
    run_parallel_broadcasts(p, b_copy);
}

// broadcast_node does not allow successors to try_get from it (it does not allow
// the flow edge to switch) so we only need test the forward direction.
template<typename T>
void test_resets() {
    tbb::flow::graph g;
    tbb::flow::broadcast_node<T> b0(g);
    tbb::flow::broadcast_node<T> b1(g);
    tbb::flow::queue_node<T> q0(g);
    tbb::flow::make_edge(b0,b1);
    tbb::flow::make_edge(b1,q0);
    T j;

    // test standard reset
    for(int testNo = 0; testNo < 2; ++testNo) {
        for(T i= 0; i <= 3; i += 1) {
            b0.try_put(i);
        }
        g.wait_for_all();
        for(T i= 0; i <= 3; i += 1) {
            ASSERT(q0.try_get(j) && j == i, "Bad value in queue");
        }
        ASSERT(!q0.try_get(j), "extra value in queue");

        // reset the graph.  It should work as before.
        if (testNo == 0) g.reset();
    }

    g.reset(tbb::flow::rf_clear_edges);
    for(T i= 0; i <= 3; i += 1) {
        b0.try_put(i);
    }
    g.wait_for_all();
    ASSERT(!q0.try_get(j), "edge between nodes not removed");
    for(T i= 0; i <= 3; i += 1) {
        b1.try_put(i);
    }
    g.wait_for_all();
    ASSERT(!q0.try_get(j), "edge between nodes not removed");
}

#if TBB_PREVIEW_FLOW_GRAPH_FEATURES
void test_extract() {
    int dont_care;
    tbb::flow::graph g;
    tbb::flow::broadcast_node<int> b0(g);
    tbb::flow::broadcast_node<int> b1(g);
    tbb::flow::broadcast_node<int> b2(g);
    tbb::flow::broadcast_node<int> b3(g);
    tbb::flow::broadcast_node<int> b4(g);
    tbb::flow::broadcast_node<int> b5(g);
    tbb::flow::queue_node<int> q0(g);
    tbb::flow::make_edge(b0,b1);
    tbb::flow::make_edge(b0,b2);
    tbb::flow::make_edge(b1,b3);
    tbb::flow::make_edge(b1,b4);
    tbb::flow::make_edge(b2,b4);
    tbb::flow::make_edge(b2,b5);
    tbb::flow::make_edge(b3,q0);
    tbb::flow::make_edge(b4,q0);
    tbb::flow::make_edge(b5,q0);

    /*          b3       */
    /*         /  \      */
    /*        b1   \     */
    /*       / \    \    */
    /*     b0   b4---q0  */
    /*       \ /    /    */
    /*        b2   /     */
    /*         \  /      */
    /*          b5       */

    g.wait_for_all();
    b0.try_put(1);
    g.wait_for_all();
    for( int i = 0; i < 4; ++i ) {
        int j;
        ASSERT(q0.try_get(j) && j == 1, "missing or incorrect message");
    }
    ASSERT(!q0.try_get(dont_care), "extra message in queue");
    ASSERT(b0.predecessor_count() == 0 && b0.successor_count() == 2, "improper count for b0");
    ASSERT(b1.predecessor_count() == 1 && b1.successor_count() == 2, "improper count for b1");
    ASSERT(b2.predecessor_count() == 1 && b2.successor_count() == 2, "improper count for b2");
    ASSERT(b3.predecessor_count() == 1 && b3.successor_count() == 1, "improper count for b3");
    ASSERT(b4.predecessor_count() == 2 && b4.successor_count() == 1, "improper count before extract of b4");
    ASSERT(b5.predecessor_count() == 1 && b5.successor_count() == 1, "improper count for b5");
    b4.extract();  // remove from tree of nodes.
    ASSERT(b0.predecessor_count() == 0 && b0.successor_count() == 2, "improper count for b0 after");
    ASSERT(b1.predecessor_count() == 1 && b1.successor_count() == 1, "improper succ count for b1 after");
    ASSERT(b2.predecessor_count() == 1 && b2.successor_count() == 1, "improper succ count for b2 after");
    ASSERT(b3.predecessor_count() == 1 && b3.successor_count() == 1, "improper succ count for b3 after");
    ASSERT(b4.predecessor_count() == 0 && b4.successor_count() == 0, "improper succ count after extract");
    ASSERT(b5.predecessor_count() == 1 && b5.successor_count() == 1, "improper succ count for b5 after");

    /*          b3       */
    /*         /  \      */
    /*        b1   \     */
    /*       /      \    */
    /*     b0        q0  */
    /*       \      /    */
    /*        b2   /     */
    /*         \  /      */
    /*          b5       */

    b0.try_put(1);
    g.wait_for_all();
    for( int i = 0; i < 2; ++i ) {
        int j;
        ASSERT(q0.try_get(j) && j == 1, "missing or incorrect message");
    }
    ASSERT(!q0.try_get(dont_care), "extra message in queue");
    tbb::flow::make_edge(b0,b4);
    tbb::flow::make_edge(b4,q0);
    g.wait_for_all();
    ASSERT(b0.predecessor_count() == 0 && b0.successor_count() == 3, "improper count for b0 after");
    ASSERT(b1.predecessor_count() == 1 && b1.successor_count() == 1, "improper succ count for b1 after");
    ASSERT(b2.predecessor_count() == 1 && b2.successor_count() == 1, "improper succ count for b2 after");
    ASSERT(b3.predecessor_count() == 1 && b3.successor_count() == 1, "improper succ count for b3 after");
    ASSERT(b4.predecessor_count() == 1 && b4.successor_count() == 1, "improper succ count after extract");
    ASSERT(b5.predecessor_count() == 1 && b5.successor_count() == 1, "improper succ count for b5 after");

    /*          b3       */
    /*         /  \      */
    /*        b1   \     */
    /*       /      \    */
    /*     b0---b4---q0  */
    /*       \      /    */
    /*        b2   /     */
    /*         \  /      */
    /*          b5       */

    b0.try_put(1);
    g.wait_for_all();
    for( int i = 0; i < 3; ++i ) {
        int j;
        ASSERT(q0.try_get(j) && j == 1, "missing or incorrect message");
    }
    ASSERT(!q0.try_get(dont_care), "extra message in queue");
}
#endif  // TBB_PREVIEW_FLOW_GRAPH_FEATURES

int TestMain() {
    if( MinThread<1 ) {
        REPORT("number of threads must be positive\n");
        exit(1);
    }

   test_serial_broadcasts<int>();
   test_serial_broadcasts<float>();
   test_serial_broadcasts<int_convertable_type>();

   for( int p=MinThread; p<=MaxThread; ++p ) {
       test_parallel_broadcasts<int>(p);
       test_parallel_broadcasts<float>(p);
       test_parallel_broadcasts<int_convertable_type>(p);
   }

   test_resets<int>();
   test_resets<float>();
#if TBB_PREVIEW_FLOW_GRAPH_FEATURES
   test_extract();
#endif

   return Harness::Done;
}
