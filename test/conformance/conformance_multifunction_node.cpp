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

//! \file conformance_multifunction_node.cpp
//! \brief Test for [flow_graph.function_node] specification

/*
    Test execution of node body
    Test node can do try_put call
*/
void test_multifunc_body(){
    conformance::test_body_exec_impl<oneapi::tbb::flow::multifunction_node<oneapi::tbb::flow::continue_msg, std::tuple<int>, oneapi::tbb::flow::rejecting>>(oneapi::tbb::flow::unlimited);
}

/*
    Test function_node is a graph_node, receiver<Input>, and sender<Output>
*/
template<typename I, typename O>
void test_inheritance(){
    using namespace oneapi::tbb::flow;

    CHECK_MESSAGE( (std::is_base_of<graph_node, multifunction_node<I, O>>::value), "multifunction_node should be derived from graph_node");
    CHECK_MESSAGE( (std::is_base_of<receiver<I>, multifunction_node<I, O>>::value), "multifunction_node should be derived from receiver<Input>");
}

/*
    Test the body object passed to a node is copied
*/
void test_copy_body(){
    conformance::test_copy_body_impl<oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>, conformance::CountingObject<std::tuple<int>, int>>(oneapi::tbb::flow::unlimited);
}

/*
    Test node forvard messages to successors
*/
void test_forwarding(){
    conformance::test_forwarding_impl<oneapi::tbb::flow::multifunction_node<oneapi::tbb::flow::continue_msg, std::tuple<int>>>(oneapi::tbb::flow::unlimited);
}

/*
    Test node not buffered unsuccesful message, and try_get after rejection should not succeed.
*/
void test_rejecting_buffering(){
    oneapi::tbb::flow::graph g;
    conformance::dummy_functor<int> fun;

    oneapi::tbb::flow::multifunction_node<int, std::tuple<int>, oneapi::tbb::flow::rejecting> node(g, oneapi::tbb::flow::unlimited, fun);
    oneapi::tbb::flow::limiter_node<int> rejecter(g, 0);

    oneapi::tbb::flow::make_edge(node, rejecter);
    node.try_put(1);

    int tmp = -1;
    CHECK_MESSAGE( (std::get<0>(node.output_ports()).try_get(tmp) == false), "try_get after rejection should not succeed");
    CHECK_MESSAGE( (tmp == -1), "try_get after rejection should alter passed value");
    g.wait_for_all();
}

/*
    Test all node constructors
*/
void test_ctors(){
    using namespace oneapi::tbb::flow;
    graph g;

    conformance::counting_functor<int> fun;

    multifunction_node<int, std::tuple<int>> fn1(g, unlimited, fun);
    multifunction_node<int, std::tuple<int>> fn2(g, unlimited, fun, oneapi::tbb::flow::node_priority_t(1));

    multifunction_node<int, std::tuple<int>, lightweight> lw_node1(g, serial, fun, lightweight());
    multifunction_node<int, std::tuple<int>, lightweight> lw_node2(g, serial, fun, lightweight(), oneapi::tbb::flow::node_priority_t(1));
}

/*
    The node that is constructed has a reference to the same graph object as src, has a copy of the initial body used by src, and has the same concurrency threshold as src.
    The predecessors and successors of src are not copied.
*/
void test_copy_ctor(){
    conformance::test_copy_ctor_impl<oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>, conformance::CountingObject<std::tuple<int>, int>>();
}

/*
    Test nodes for execution with priority in single-threaded configuration
*/
void test_priority(){
    conformance::test_priority_impl<oneapi::tbb::flow::multifunction_node<oneapi::tbb::flow::continue_msg, std::tuple<oneapi::tbb::flow::continue_msg>>>(oneapi::tbb::flow::unlimited);
}

/*
    Test function_node has a user-settable concurrency limit. It can be set to one of predefined values. 
    The user can also provide a value of type std::size_t to limit concurrency.
    Test that not more than limited threads works in parallel.
*/
void test_node_concurrency(){
    conformance::test_concurrency_impl<oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>>(oneapi::tbb::flow::unlimited);
}

/*
    Test node reject the incoming message if the concurrency limit achieved.
*/
void test_rejecting(){
    conformance::test_rejecting_impl<oneapi::tbb::flow::multifunction_node <int, std::tuple<int>, oneapi::tbb::flow::rejecting>>();
}

/*
    Test node Input class meet the DefaultConstructible and CopyConstructible requirements and Output class meet the CopyConstructible requirements.
*/
void test_output_input_class(){
    using Body = conformance::CountingObject<int>;
    conformance::test_output_input_class_impl<oneapi::tbb::flow::multifunction_node<Body, std::tuple<Body>>, Body>();
}

/*
    Test multifunction_node output_ports() returns a tuple of output ports.
*/
void test_output_ports(){
    using namespace oneapi::tbb::flow;
    graph g;
    conformance::dummy_functor<int> fun;
    oneapi::tbb::flow::multifunction_node <int, std::tuple<int>> node(g, unlimited, fun);

    CHECK_MESSAGE( (std::is_same<oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type&, decltype(node.output_ports())>::value), "multifunction_node output_ports should returns a tuple of output ports");
}

//! Test multifunction_node with rejecting policy
//! \brief \ref interface
TEST_CASE("multifunction_node with rejecting policy"){
    test_rejecting();
}

//! Test priorities
//! \brief \ref interface
TEST_CASE("multifunction_node priority"){
    test_priority();
}

//! Test concurrency
//! \brief \ref interface
TEST_CASE("multifunction_node concurrency"){
    test_node_concurrency();
}

//! Test all node constructors
//! \brief \ref interface
TEST_CASE("multifunction_node constructors"){
    test_ctors();
}

//! Test copy constructor
//! \brief \ref interface
TEST_CASE("multifunction_node copy constructor"){
    test_copy_ctor();
}

//! Test multifunction_node buffering
//! \brief \ref requirement
TEST_CASE("multifunction_node buffering"){
    test_rejecting_buffering();
}

//! Test multifunction_node broadcasting
//! \brief \ref requirement
TEST_CASE("multifunction_node broadcast"){
    test_forwarding();
}

//! Test body copying and copy_body logic
//! \brief \ref interface
TEST_CASE("multifunction_node constructors"){
    test_copy_body();
}

//! Test calling function body
//! \brief \ref interface \ref requirement
TEST_CASE("multifunction_node body") {
    test_multifunc_body();
}

//! Test multifunction_node output_ports
//! \brief \ref interface \ref requirement
TEST_CASE("multifunction_node output_ports") {
    test_output_ports();
}

//! Test inheritance relations
//! \brief \ref interface
TEST_CASE("multifunction_node superclasses"){
    test_inheritance<int, std::tuple<int>>();
    test_inheritance<void*, std::tuple<float>>();
}

//! Test constructible Output and Input class
//! \brief \ref interface \ref requirement
TEST_CASE("Test function_node Output and Input class") {
    test_output_input_class();
}
