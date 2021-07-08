/*
    Copyright (c) 2020-2021 Intel Corporation

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

#if __INTEL_COMPILER && _MSC_VER
#pragma warning(disable : 2586) // decorated name length exceeded, name was truncated
#endif

#include "conformance_flowgraph.h"

//! \file conformance_async_node.cpp
//! \brief Test for [flow_graph.async_node] specification

/*
TODO: implement missing conformance tests for async_node:
  - [X] Write `test_forwarding()'.
  - [ ] Improve test of the node's copy-constructor.
  - [X] Write `test_priority'.
  - [X] Rename `test_discarding' to `test_buffering'.
  - [X] Write inheritance test.
  - [X] Constructor with explicitly passed Policy parameter.
  - [X] Concurrency testing of the node: make a loop over possible concurrency levels. It is
    important to test at least on five values: 1, oneapi::tbb::flow::serial, `max_allowed_parallelism'
    obtained from `oneapi::tbb::global_control', `oneapi::tbb::flow::unlimited', and, if `max allowed
    parallelism' is > 2, use something in the middle of the [1, max_allowed_parallelism]
    interval. Use `utils::ExactConcurrencyLevel' entity (extending it if necessary).
  - [ ] Write `test_rejecting', where avoid dependency on OS scheduling of the threads; add check
    that `try_put()' returns `false'
  - [ ] The `copy_body' function copies altered body (e.g. after successful `try_put()' call).
  - [ ] The copy constructor and copy assignment are called for the node's input and output types.
  - [ ] Add CTAD test.
*/

template< typename OutputType >
struct as_inc_functor {
    std::thread my_thread;

    std::atomic<size_t>& local_execute_count;

    as_inc_functor(std::atomic<size_t>& execute_count ) :
        local_execute_count (execute_count)
    {  }

    as_inc_functor( const as_inc_functor &f ) : local_execute_count(f.local_execute_count) { }
    void operator=(const as_inc_functor &f) { local_execute_count = size_t(f.local_execute_count); }

    void operator()( int num , oneapi::tbb::flow::async_node<int, int>::gateway_type& g) {
        ++local_execute_count;
        g.try_put(num);
        //    my_thread = std::thread([&](){
        //                                g.try_put(num);
        //                            });
    }

};

//! Test async_node costructors
//! \brief \ref requirement
TEST_CASE("async_node constructors"){
    using namespace oneapi::tbb::flow;
    graph g;

    conformance::dummy_functor<int> fun;

    async_node<int, int> fn1(g, unlimited, fun);
    async_node<int, int> fn2(g, unlimited, fun, oneapi::tbb::flow::node_priority_t(1));

    async_node<int, int, lightweight> lw_node1(g, serial, fun, lightweight());
    async_node<int, int, lightweight> lw_node2(g, serial, fun, lightweight(), oneapi::tbb::flow::node_priority_t(1));
}

// //! Test buffering property
// //! \brief \ref requirement
// TEST_CASE("async_node buffering") {
//     conformance::test_buffering<oneapi::tbb::flow::async_node<oneapi::tbb::flow::continue_msg, int>>(oneapi::tbb::flow::unlimited);
// }

// //! Test async_node priority interface
// //! \brief \ref interface
// TEST_CASE("async_node priority interface"){
//     oneapi::tbb::flow::graph g;
//     std::atomic<size_t> local_count(0);
//     as_inc_functor<int> fun(local_count);

//     oneapi::tbb::flow::async_node<int, int> node1(g, oneapi::tbb::flow::unlimited, fun, oneapi::tbb::flow::no_priority);
// }

// //! Test async_node copy
// //! \brief \ref interface
// TEST_CASE("async_node copy"){
//     oneapi::tbb::flow::graph g;
//     std::atomic<size_t> local_count(0);
//     as_inc_functor<int> fun(local_count);

//     oneapi::tbb::flow::async_node<int, int> node1(g, oneapi::tbb::flow::unlimited, fun);
//     oneapi::tbb::flow::async_node<int, int> node2(node1);
// }

// //! Test calling async body
// //! \brief \ref interface \ref requirement
// TEST_CASE("Test async_node body") {
//     conformance::test_body_exec<oneapi::tbb::flow::async_node<oneapi::tbb::flow::continue_msg, int>>(oneapi::tbb::flow::unlimited);
// }

// //! Test async_node inheritance relations
// //! \brief \ref interface
// TEST_CASE("async_node superclasses"){
//     conformance::test_inheritance<oneapi::tbb::flow::async_node<int, int>, int, int>();
//     conformance::test_inheritance<oneapi::tbb::flow::async_node<void*, float>, void*, float>();
// }

// //! Test node broadcast messages to successors
// //! \brief \ref requirement
// TEST_CASE("function_node broadcast"){
//     conformance::counting_functor<int> fun(conformance::expected);
//     conformance::test_forwarding<oneapi::tbb::flow::async_node<oneapi::tbb::flow::continue_msg, int>>(1, oneapi::tbb::flow::unlimited, fun);
// }

// //! Test async_node has a user-settable concurrency limit. It can be set to one of predefined values. 
// //! The user can also provide a value of type std::size_t to limit concurrency.
// //! Test that not more than limited threads works in parallel.
// //! \brief \ref requirement
// TEST_CASE("concurrency follows set limits"){
//     conformance::test_concurrency<oneapi::tbb::flow::async_node<int, int>>();
// }

// //! Test body copying and copy_body logic
// //! Test the body object passed to a node is copied
// //! \brief \ref interface
// TEST_CASE("async_node body copying"){
//     conformance::test_copy_body<oneapi::tbb::flow::async_node<oneapi::tbb::flow::continue_msg, int>, conformance::CountingObject<int>>(oneapi::tbb::flow::unlimited);
// }

// //! The node that is constructed has a reference to the same graph object as src, has a copy of the initial body used by src, and has the same concurrency threshold as src.
// //! The predecessors and successors of src are not copied.
// //! \brief \ref requirement
// TEST_CASE("async_node copy constructor"){
//     conformance::test_copy_ctor<oneapi::tbb::flow::async_node<int, int>, conformance::CountingObject<int>>();
// }

// //! Test node Input class meet the DefaultConstructible and CopyConstructible requirements and Output class meet the CopyConstructible requirements.
// //! \brief \ref interface \ref requirement
// TEST_CASE("Test async_node Output and Input class") {
//     using Body = conformance::CountingObject<int>;
//     conformance::test_output_input_class<oneapi::tbb::flow::async_node<Body, Body>, Body>();
// }
