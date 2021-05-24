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

//! \file conformance_function_node.cpp
//! \brief Test for [flow_graph.function_node] specification

/*
    Test execution of node body
    Test node can do try_put call
*/
void test_func_body(){
    conformance::test_body_exec_impl<oneapi::tbb::flow::function_node<oneapi::tbb::flow::continue_msg, int>>(oneapi::tbb::flow::unlimited);
}

/*
    Test function_node is a graph_node, receiver<Input>, and sender<Output>
*/
template<typename I, typename O>
void test_inheritance(){
    conformance::test_inheritance_impl<oneapi::tbb::flow::function_node<I, O>, I, O>();
}

/*
    Test node deduction guides
*/
#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT

int function_body_f(const int&) { return 1; }

template <typename Body>
void test_deduction_guides_common(Body body) {
    using namespace tbb::flow;
    graph g;

    function_node f1(g, unlimited, body);
    static_assert(std::is_same_v<decltype(f1), function_node<int, int>>);

    function_node f2(g, unlimited, body, rejecting());
    static_assert(std::is_same_v<decltype(f2), function_node<int, int, rejecting>>);

    function_node f3(g, unlimited, body, node_priority_t(5));
    static_assert(std::is_same_v<decltype(f3), function_node<int, int>>);

    function_node f4(g, unlimited, body, rejecting(), node_priority_t(5));
    static_assert(std::is_same_v<decltype(f4), function_node<int, int, rejecting>>);

#if __TBB_PREVIEW_FLOW_GRAPH_NODE_SET
    function_node f5(follows(f2), unlimited, body);
    static_assert(std::is_same_v<decltype(f5), function_node<int, int>>);

    function_node f6(follows(f5), unlimited, body, rejecting());
    static_assert(std::is_same_v<decltype(f6), function_node<int, int, rejecting>>);

    function_node f7(follows(f6), unlimited, body, node_priority_t(5));
    static_assert(std::is_same_v<decltype(f7), function_node<int, int>>);

    function_node f8(follows(f7), unlimited, body, rejecting(), node_priority_t(5));
    static_assert(std::is_same_v<decltype(f8), function_node<int, int, rejecting>>);
#endif // __TBB_PREVIEW_FLOW_GRAPH_NODE_SET

    function_node f9(f1);
    static_assert(std::is_same_v<decltype(f9), function_node<int, int>>);
}

void test_deduction_guides() {
    test_deduction_guides_common([](const int&)->int { return 1; });
    test_deduction_guides_common([](const int&) mutable ->int { return 1; });
    test_deduction_guides_common(function_body_f);
}

#endif

/*
    Test node forvard messages to successors
*/
void test_forvarding(){
    conformance::test_forvarding_impl<oneapi::tbb::flow::function_node<oneapi::tbb::flow::continue_msg, int>>(oneapi::tbb::flow::unlimited);
}

/*
    Test node not buffered unsuccesful message, and try_get after rejection should not succeed.
*/
template<typename Policy>
void test_buffering(){
   conformance::test_buffering_impl<oneapi::tbb::flow::function_node<oneapi::tbb::flow::continue_msg, int, Policy>>(oneapi::tbb::flow::unlimited);
}

/*
    Test all node constructors
*/
void test_ctors(){
    using namespace oneapi::tbb::flow;
    graph g;

    conformance::counting_functor<int> fun;

    function_node<int, int> fn1(g, unlimited, fun);
    function_node<int, int> fn2(g, unlimited, fun, oneapi::tbb::flow::node_priority_t(1));

    function_node<int, int, lightweight> lw_node1(g, serial, fun, lightweight());
    function_node<int, int, lightweight> lw_node2(g, serial, fun, lightweight(), oneapi::tbb::flow::node_priority_t(1));
}

/*
    The node that is constructed has a reference to the same graph object as src, has a copy of the initial body used by src, and has the same concurrency threshold as src.
    The predecessors and successors of src are not copied.
*/
void test_copy_ctor(){
    using namespace oneapi::tbb::flow;
    graph g;

    conformance::dummy_functor<int> fun1;
    conformance::CountingObject<int> fun2;

    function_node<int, int> node0(g, unlimited, fun1);
    function_node<int, oneapi::tbb::flow::continue_msg> node1(g, unlimited, fun2);
    conformance::test_push_receiver<oneapi::tbb::flow::continue_msg> node2(g);
    conformance::test_push_receiver<oneapi::tbb::flow::continue_msg> node3(g);

    oneapi::tbb::flow::make_edge(node0, node1);
    oneapi::tbb::flow::make_edge(node1, node2);

    function_node<int, oneapi::tbb::flow::continue_msg> node_copy(node1);

    conformance::test_body_copying(node_copy, fun2);

    oneapi::tbb::flow::make_edge(node_copy, node3);

    node_copy.try_put(1);
    g.wait_for_all();

    CHECK_MESSAGE( (conformance::get_values(node2).size() == 0 && conformance::get_values(node3).size() == 1), "Copied node doesn`t copy successor, but copy number of predecessors");

    node0.try_put(1);
    g.wait_for_all();

    CHECK_MESSAGE( (conformance::get_values(node2).size() == 1 && conformance::get_values(node3).size() == 0), "Copied node doesn`t copy predecessor, but copy number of predecessors");
}

/*
    Test the body object passed to a node is copied
*/
void test_copy_body(){
    conformance::test_copy_body_impl<oneapi::tbb::flow::function_node<oneapi::tbb::flow::continue_msg, int>>(oneapi::tbb::flow::unlimited);
}

/*
    Test node Input class meet the DefaultConstructible and CopyConstructible requirements and Output class meet the CopyConstructible requirements.
*/
void test_output_input_class(){
    using namespace oneapi::tbb::flow;

    conformance::passthru_body<conformance::CountingObject<int>> fun;

    graph g;
    function_node<conformance::CountingObject<int>, conformance::CountingObject<int>> node1(g, unlimited, fun);
    conformance::test_push_receiver<conformance::CountingObject<int>> node2(g);
    make_edge(node1, node2);
    conformance::CountingObject<int> b1;
    conformance::CountingObject<int> b2;
    node1.try_put(b1);
    g.wait_for_all();
    node2.try_get(b2);
    DOCTEST_WARN_MESSAGE( (b1.copies_count > 0), "The type Input must meet the DefaultConstructible and CopyConstructible requirements");
    DOCTEST_WARN_MESSAGE( (b2.is_copy), "The type Output must meet the CopyConstructible requirements");
}

/*
    Test nodes for execution with priority in single-threaded configuration
*/
void test_priority(){
    conformance::test_priority_impl<oneapi::tbb::flow::function_node<oneapi::tbb::flow::continue_msg, oneapi::tbb::flow::continue_msg>>(oneapi::tbb::flow::unlimited);
}

/*
    Test function_node has a user-settable concurrency limit. It can be set to one of predefined values. 
    The user can also provide a value of type std::size_t to limit concurrency.
    Test that not more than limited threads works in parallel.
*/
void test_node_concurrency(){
    oneapi::tbb::global_control c(oneapi::tbb::global_control::max_allowed_parallelism,
                                          oneapi::tbb::this_task_arena::max_concurrency());

    int max_num_threads = tbb::global_control::active_value(
        tbb::global_control::max_allowed_parallelism
    );

    std::vector<int> threads_count = {1, oneapi::tbb::flow::serial, max_num_threads, oneapi::tbb::flow::unlimited};

    if(max_num_threads > 2){
        threads_count.push_back(max_num_threads / 2);
    }

    for(auto num_threads : threads_count){
        utils::ConcurrencyTracker::Reset();
        int expected_threads = num_threads;
        if(num_threads == oneapi::tbb::flow::unlimited){
            expected_threads = max_num_threads;
        }
        oneapi::tbb::flow::graph g;
        conformance::barrier_body counter(expected_threads);
        oneapi::tbb::flow::function_node <int, int> fnode(g, num_threads, counter);

        conformance::test_push_receiver<int> sink(g);

        make_edge(fnode, sink);

        for(int i = 0; i < 500; ++i){
            fnode.try_put(i);
        }
        g.wait_for_all();
    }
}

/*
    Test node reject the incoming message if the concurrency limit achieved.
*/
void test_rejecting(){
    oneapi::tbb::flow::graph g;
    std::atomic<int64_t> value;
    oneapi::tbb::flow::function_node <int, int, oneapi::tbb::flow::rejecting> fnode(g, oneapi::tbb::flow::serial,
                                                                    [&](int v){
                                                                        for(size_t i = 0; i < 100000000; ++i){
                                                                            ++value;
                                                                        }
                                                                        return v;
                                                                    });

    conformance::test_push_receiver<int> sink(g);

    make_edge(fnode, sink);

    bool try_put_state;

    for(int i = 0; i < 10; ++i){
        try_put_state = fnode.try_put(i);
    }

    g.wait_for_all();
    CHECK_MESSAGE( (conformance::get_values(sink).size() == 1), "Messages should be rejected while the first is being processed");
    CHECK_MESSAGE( (!try_put_state), "`try_put()' should returns `false' after rejecting");
}

//! Test function_node costructors
//! \brief \ref requirement
TEST_CASE("function_node constructors"){
    test_ctors();
}

//! Test function_node copy costructor
//! \brief \ref requirement
TEST_CASE("function_node copy constructor"){
    test_copy_ctor();
}

//! Test function_node with rejecting policy
//! \brief \ref interface
TEST_CASE("function_node with rejecting policy"){
    test_rejecting();
}

//! Test body copying and copy_body logic
//! \brief \ref interface
TEST_CASE("function_node and body copying"){
    test_copy_body();
}

//! Test inheritance relations
//! \brief \ref interface
TEST_CASE("function_node superclasses"){
    test_inheritance<int, int>();
    test_inheritance<void*, float>();
}

//! Test function_node buffering
//! \brief \ref requirement
TEST_CASE("function_node buffering"){
    test_buffering<oneapi::tbb::flow::rejecting>();
    test_buffering<oneapi::tbb::flow::queueing>();
}

//! Test function_node broadcasting
//! \brief \ref requirement
TEST_CASE("function_node broadcast"){
    test_forvarding();
}

//! Test deduction guides
//! \brief \ref interface \ref requirement
TEST_CASE("Deduction guides"){
#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT
    test_deduction_guides();
#endif
}

//! Test priorities work in single-threaded configuration
//! \brief \ref requirement
TEST_CASE("function_node priority support"){
    test_priority();
}

//! Test that measured concurrency respects set limits
//! \brief \ref requirement
TEST_CASE("concurrency follows set limits"){
    test_node_concurrency();
}

//! Test calling function body
//! \brief \ref interface \ref requirement
TEST_CASE("Test function_node body") {
    test_func_body();
}

//! Test constructible Output and Input class
//! \brief \ref interface \ref requirement
TEST_CASE("Test function_node Output and Input class") {
    test_output_input_class();
}
