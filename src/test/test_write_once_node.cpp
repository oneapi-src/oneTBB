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

#include "harness_graph.h"

#include "tbb/task_scheduler_init.h"

#define N 300
#define T 4
#define M 4

template< typename R >
void simple_read_write_tests() {
    tbb::flow::graph g;
    tbb::flow::write_once_node<R> n(g);

    for ( int t = 0; t < T; ++t ) {
        R v0(0);
        harness_counting_receiver<R> r[M];

        ASSERT( n.is_valid() == false, NULL );
        ASSERT( n.try_get( v0 ) == false, NULL );

        if ( t % 2 ) {
            ASSERT( n.try_put( static_cast<R>(N+1) ), NULL );
            ASSERT( n.is_valid() == true, NULL );
            ASSERT( n.try_get( v0 ) == true, NULL );
            ASSERT( v0 == R(N+1), NULL );
       }

        for (int i = 0; i < M; ++i) {
           tbb::flow::make_edge( n, r[i] );
        }

        if ( t%2 ) {
            for (int i = 0; i < M; ++i) {
                 size_t c = r[i].my_count;
                 ASSERT( int(c) == 1, NULL );
            }
        }

        for (int i = 1; i <= N; ++i ) {
            R v1(static_cast<R>(i));

            bool result = n.try_put( v1 );
            if ( !(t%2) && i == 1 )
                ASSERT( result == true, NULL );
            else
                ASSERT( result == false, NULL );

            ASSERT( n.is_valid() == true, NULL );

            for (int j = 0; j < N; ++j ) {
                R v2(0);
                ASSERT( n.try_get( v2 ), NULL );
                if ( t%2 )
                    ASSERT( R(N+1) == v2, NULL );
                else
                    ASSERT( R(1) == v2, NULL );
            }
        }
        for (int i = 0; i < M; ++i) {
             size_t c = r[i].my_count;
             ASSERT( int(c) == 1, NULL );
        }
        for (int i = 0; i < M; ++i) {
           tbb::flow::remove_edge( n, r[i] );
        }
        ASSERT( n.try_put( R(0) ) == false, NULL );
        for (int i = 0; i < M; ++i) {
             size_t c = r[i].my_count;
             ASSERT( int(c) == 1, NULL );
        }
        n.clear();
        ASSERT( n.is_valid() == false, NULL );
        ASSERT( n.try_get( v0 ) == false, NULL );
    }
}

template< typename R >
class native_body : NoAssign {
    tbb::flow::write_once_node<R> &my_node;

public:

     native_body( tbb::flow::write_once_node<R> &n ) : my_node(n) {}

     void operator()( int i ) const {
         R v1(static_cast<R>(i));
         ASSERT( my_node.try_put( v1 ) == false, NULL );
         ASSERT( my_node.is_valid() == true, NULL );
         ASSERT( my_node.try_get( v1 ) == true, NULL );
         ASSERT( v1 == R(-1), NULL );
     }
};

template< typename R >
void parallel_read_write_tests() {
    tbb::flow::graph g;
    tbb::flow::write_once_node<R> n(g);
    //Create a vector of identical nodes
    std::vector< tbb::flow::write_once_node<R> > wo_vec(2, n);

    for (size_t node_idx=0; node_idx<wo_vec.size(); ++node_idx) {
    for ( int t = 0; t < T; ++t ) {
        harness_counting_receiver<R> r[M];

        for (int i = 0; i < M; ++i) {
           tbb::flow::make_edge( wo_vec[node_idx], r[i] );
        }
        R v0;
        ASSERT( wo_vec[node_idx].is_valid() == false, NULL );
        ASSERT( wo_vec[node_idx].try_get( v0 ) == false, NULL );

        ASSERT( wo_vec[node_idx].try_put( R(-1) ), NULL );

        NativeParallelFor( N, native_body<R>( wo_vec[node_idx] ) );

        for (int i = 0; i < M; ++i) {
             size_t c = r[i].my_count;
             ASSERT( int(c) == 1, NULL );
        }
        for (int i = 0; i < M; ++i) {
           tbb::flow::remove_edge( wo_vec[node_idx], r[i] );
        }
        ASSERT( wo_vec[node_idx].try_put( R(0) ) == false, NULL );
        for (int i = 0; i < M; ++i) {
             size_t c = r[i].my_count;
             ASSERT( int(c) == 1, NULL );
        }
        wo_vec[node_idx].clear();
        ASSERT( wo_vec[node_idx].is_valid() == false, NULL );
        ASSERT( wo_vec[node_idx].try_get( v0 ) == false, NULL );
    }
    }
}

int TestMain() {
    simple_read_write_tests<int>();
    simple_read_write_tests<float>();
    for( int p=MinThread; p<=MaxThread; ++p ) {
        tbb::task_scheduler_init init(p);
        parallel_read_write_tests<int>();
        parallel_read_write_tests<float>();
    }
#if TBB_PREVIEW_FLOW_GRAPH_FEATURES
    test_extract_on_node<tbb::flow::write_once_node, int>();
#endif
    return Harness::Done;
}

