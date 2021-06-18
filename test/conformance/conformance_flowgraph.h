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

#ifndef __TBB_test_conformance_conformance_flowgraph_H
#define __TBB_test_conformance_conformance_flowgraph_H

#include "common/test.h"
#include "common/utils.h"
#include "common/graph_utils.h"
#include "common/concurrency_tracker.h"

#include "oneapi/tbb/flow_graph.h"
#include "oneapi/tbb/task_arena.h"
#include "oneapi/tbb/global_control.h"


namespace conformance{

template<typename V>
using test_push_receiver = oneapi::tbb::flow::queue_node<V>;

template<typename V>
std::vector<V> get_values( test_push_receiver<V>& rr ){
    std::vector<V> messages;
    int val = 0;
    for(V tmp; rr.try_get(tmp); ++val){
        messages.push_back(tmp);
    }
    return messages;
}

template< typename OutputType >
struct first_functor {
    int my_id;
    static std::atomic<int> first_id;

    first_functor(int id) : my_id(id) {}

    OutputType operator()( OutputType argument ) {
        int old_value = first_id;
        while(first_id == -1 &&
              !first_id.compare_exchange_weak(old_value, my_id))
            ;

        return argument;
    }

    OutputType operator()( const oneapi::tbb::flow::continue_msg&  ) {
        return operator()(OutputType());
    }

    void operator()( oneapi::tbb::flow::continue_msg, oneapi::tbb::flow::multifunction_node<oneapi::tbb::flow::continue_msg, std::tuple<oneapi::tbb::flow::continue_msg>>::output_ports_type &op ) {
       operator()(OutputType());
       std::get<0>(op).try_put(oneapi::tbb::flow::continue_msg());
    }
};

template<typename OutputType>
std::atomic<int> first_functor<OutputType>::first_id;

template< typename OutputType>
struct counting_functor {
    OutputType return_value;

    static std::atomic<std::size_t> execute_count;

    counting_functor(OutputType value = OutputType()) : return_value(value) {
        execute_count = 0;
    }

    OutputType operator()( oneapi::tbb::flow::continue_msg ) {
       ++execute_count;
       return return_value;
    }

    OutputType operator()( OutputType ) {
       ++execute_count;
       return return_value;
    }

    void operator()( const int&, oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type &op ) {
       ++execute_count;
       std::get<0>(op).try_put(return_value);
    }

    void operator()( oneapi::tbb::flow::continue_msg, oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type &op ) {
       ++execute_count;
       std::get<0>(op).try_put(return_value);
    }

    OutputType operator()( oneapi::tbb::flow_control & fc ) {
       ++execute_count;
       if(execute_count > std::size_t(return_value)){
           fc.stop();
           return return_value;
       }
       return return_value;
    }
};

template<typename OutputType>
std::atomic<std::size_t> counting_functor<OutputType>::execute_count;

template< typename OutputType>
struct worker_body {
    OutputType operator()( oneapi::tbb::flow::continue_msg ) {
       utils::doDummyWork(1000000);
       return OutputType();
    }
    OutputType operator()( OutputType ) {
        utils::doDummyWork(1000000);
       return OutputType();
    }
    void operator()( const int& argument, oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type &op ) {
        utils::doDummyWork(1000000);
        std::get<0>(op).try_put(argument);
    }
};

template< typename OutputType>
struct dummy_functor {
    OutputType operator()( oneapi::tbb::flow::continue_msg ) {
       return OutputType();
    }
    OutputType operator()( OutputType ) {
       return OutputType();
    }
    void operator()( const int& argument, oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type &op ) {
       std::get<0>(op).try_put(argument);
    }

    void operator()( const oneapi::tbb::flow::continue_msg&, oneapi::tbb::flow::multifunction_node<oneapi::tbb::flow::continue_msg, std::tuple<int>>::output_ports_type &op ) {
       std::get<0>(op).try_put(1);
    }
    
    OutputType operator()( oneapi::tbb::flow_control & fc ) {
        static int counter = 0;
        if(++counter > 5){
            counter = 0;
            fc.stop();
            return OutputType();
        }
        return OutputType();
    }
};

struct barrier_body{
    static std::atomic<bool> flag;
    std::size_t required_max_concurrency = 0;

    barrier_body(std::size_t req_max_concurrency) : required_max_concurrency(req_max_concurrency){
        flag.store(false);
    }

    barrier_body() = default;

    barrier_body(const barrier_body &) = default;

    void operator()(oneapi::tbb::flow::continue_msg){
        while(!flag.load()){ };
    }

    int operator()( oneapi::tbb::flow_control & fc ) {
        static int counter = 0;
        utils::ConcurrencyTracker ct;
        if(++counter > 500){
            counter = 0;
            fc.stop();
            return 1;
        }
        utils::doDummyWork(1000);
        CHECK_MESSAGE((int)utils::ConcurrencyTracker::PeakParallelism() <= required_max_concurrency, "Input node is serial and its body never invoked concurrently");
        return 1;
    }

    int operator()(int) {
        utils::ConcurrencyTracker ct;
        utils::doDummyWork(1000);
        CHECK_MESSAGE((int)utils::ConcurrencyTracker::PeakParallelism() <= required_max_concurrency, "Measured parallelism is not expected");
        return 1;
    }

    void operator()( const int& argument, oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type &op ) {
        utils::ConcurrencyTracker ct;
        utils::doDummyWork(1000);
        CHECK_MESSAGE((int)utils::ConcurrencyTracker::PeakParallelism() <= required_max_concurrency, "Measured parallelism is not expected");
        std::get<0>(op).try_put(argument);
    }
};

std::atomic<bool> barrier_body::flag{false};

template<typename O, typename I = int>
struct CountingObject{
    std::size_t copy_count;
    mutable std::size_t copies_count;
    std::size_t assign_count;
    mutable std::size_t assignes_count;
    std::size_t move_count;
    bool is_copy;

    CountingObject():
        copy_count(0), copies_count(0), assign_count(0), assignes_count(0), move_count(0), is_copy(false) {}

    CountingObject(const CountingObject<O>& other):
        copy_count(other.copy_count + 1), is_copy(true) {
            ++other.copies_count;
        }

    CountingObject& operator=(const CountingObject<O>& other) {
        assign_count = other.assign_count + 1;
        ++other.assignes_count;
        is_copy = true;
        return *this;
    }

    CountingObject(CountingObject<O>&& other):
         copy_count(other.copy_count), copies_count(other.copies_count),
         assign_count(other.assign_count), assignes_count(other.assignes_count),
         move_count(other.move_count + 1), is_copy(other.is_copy) {}

    CountingObject& operator=(CountingObject<O>&& other) {
        copy_count = other.copy_count;
        copies_count = other.copies_count;
        assign_count = other.assign_count;
        assignes_count = other.assignes_count;
        move_count = other.move_count + 1;
        is_copy = other.is_copy;
        return *this;
    }

    O operator()(oneapi::tbb::flow::continue_msg){
        return 1;
    }

    O operator()(O){
        return 1;
    }

    void operator()( const I& argument, oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type &op ) {
       std::get<0>(op).try_put(argument);
    }

    O operator()( oneapi::tbb::flow_control & fc ) {
        static int counter = 0;
        if(++counter > 5){
            counter = 0;
            fc.stop();
            return O();
        }
        return O();
    }
};

template <typename OutputType = int>
struct passthru_body {
    OutputType operator()( const OutputType& i ) {
        return i;
    }

    OutputType operator()( oneapi::tbb::flow::continue_msg ) {
       return OutputType();
    }

    OutputType operator()( oneapi::tbb::flow_control & fc ) {
        static int counter = 0;
        if(++counter > 5){
            counter = 0;
            fc.stop();
            return OutputType();
        }
        return OutputType();
    }

    void operator()( const int& argument, oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type &op ) {
       std::get<0>(op).try_put(argument);
    }
    
    void operator()( const CountingObject<int>& argument, oneapi::tbb::flow::multifunction_node<CountingObject<int>, std::tuple<CountingObject<int>>>::output_ports_type &op ) {
       std::get<0>(op).try_put(argument);
    }
};

template<typename Node, typename Body = conformance::counting_functor<int>, typename ...Args>
void test_body_exec(Args... args){
    oneapi::tbb::flow::graph g;
    Body counting_body;
    counting_body.execute_count = 0;

    Node node1(g, args..., counting_body);

    constexpr std::size_t n = 10;
    for(std::size_t i = 0; i < n; ++i) {
        CHECK_MESSAGE((node1.try_put(oneapi::tbb::flow::continue_msg()) == true),
                      "try_put of first node should return true");
    }
    g.wait_for_all();

    CHECK_MESSAGE((counting_body.execute_count == n), "Body of the first node needs to be executed N times");
}

template<typename Node, typename Body>
void test_body_copying(Node& tested_node, Body& base_body){
    using namespace oneapi::tbb::flow;

    Body b2 = copy_body<Body, Node>(tested_node);

    CHECK_MESSAGE((base_body.copy_count + 1 < b2.copy_count), "copy_body and constructor should copy bodies");
}

template<typename Node, typename Body, typename ...Args>
void test_copy_body(Args... args){
    using namespace oneapi::tbb::flow;

    Body base_body;

    graph g;

    Node tested_node(g, args..., base_body);

    Body b2 = copy_body<Body, Node>(tested_node);

    CHECK_MESSAGE((base_body.copy_count + 1 < b2.copy_count), "copy_body and constructor should copy bodies");
}

template<typename Node, typename ...Args>
void test_buffering(Args... args){
    oneapi::tbb::flow::graph g;
    conformance::dummy_functor<int> fun;
    Node tested_node(g, args..., fun);
    oneapi::tbb::flow::limiter_node<int> rejecter(g, 0);

    oneapi::tbb::flow::make_edge(tested_node, rejecter);

    int tmp = -1;
    if constexpr(!std::is_same<Node, oneapi::tbb::flow::input_node<int>>::value){
        tested_node.try_put(oneapi::tbb::flow::continue_msg());
        g.wait_for_all();
        if constexpr(!std::is_same<Node, oneapi::tbb::flow::multifunction_node<oneapi::tbb::flow::continue_msg, std::tuple<int>, oneapi::tbb::flow::rejecting>>::value){
            CHECK_MESSAGE((tested_node.try_get(tmp) == false), "try_get after rejection should not succeed");
        }else{
            CHECK_MESSAGE((std::get<0>(tested_node.output_ports()).try_get(tmp) == false), "try_get after rejection should not succeed");
        }
        CHECK_MESSAGE((tmp == -1), "try_get after rejection should not alter passed value");
    }else{
        tested_node.activate();
        g.wait_for_all();
        CHECK_MESSAGE((tested_node.try_get(tmp) == true), "try_get after rejection should succeed");
        CHECK_MESSAGE((tmp == 0), "try_get should return correct value");
    }
}

#include <iostream>
template<typename Node, typename ...Args>
void test_forwarding(std::size_t messages_recieved, Args... args){
    oneapi::tbb::flow::graph g;
    constexpr int expected = 5;
    conformance::counting_functor<int> fun(expected);

    Node testing_node(g, args..., fun);
    std::vector<conformance::test_push_receiver<int>*> receiver_nodes(10);

    for(std::size_t i = 0; i < 10; ++i){
        conformance::test_push_receiver<int>* node = new conformance::test_push_receiver<int>(g);
        oneapi::tbb::flow::make_edge(testing_node, *node);
        receiver_nodes[i] = node;
    }
    if constexpr(!std::is_same<Node, oneapi::tbb::flow::input_node<int>>::value){
        testing_node.try_put(oneapi::tbb::flow::continue_msg());
    }else{
        __TBB_ASSERT(expected == messages_recieved, "For correct execution of test");
        testing_node.activate();
    }
    g.wait_for_all();
    for(std::size_t i = 0; i < 10; ++i){
        auto values = conformance::get_values(*receiver_nodes[i]);
        CHECK_MESSAGE((values.size() == messages_recieved), "Descendant of the node must receive " + std::to_string(messages_recieved) + " message.");
        CHECK_MESSAGE((values[0] == expected), "Value passed is the actual one received.");
        delete receiver_nodes[i];
    }
}

template<typename Node, typename I, typename O>
void test_inheritance(){
    using namespace oneapi::tbb::flow;

    CHECK_MESSAGE((std::is_base_of<graph_node, Node>::value), "Node should be derived from graph_node");
    CHECK_MESSAGE((std::is_base_of<receiver<I>, Node>::value), "Node should be derived from receiver<Input>");
    CHECK_MESSAGE((std::is_base_of<sender<O>, Node>::value), "Node should be derived from sender<Output>");
}

template<typename Node, typename CountingBody, typename ...Args>
void test_copy_ctor(Args...){
    using namespace oneapi::tbb::flow;
    graph g;

    conformance::dummy_functor<int> fun1;
    CountingBody fun2;

    Node node0(g, unlimited, fun1);
    Node node1(g, unlimited, fun2);
    conformance::test_push_receiver<int> node2(g);
    conformance::test_push_receiver<int> node3(g);

    oneapi::tbb::flow::make_edge(node0, node1);
    oneapi::tbb::flow::make_edge(node1, node2);

    Node node_copy(node1);

    conformance::test_body_copying(node_copy, fun2);

    oneapi::tbb::flow::make_edge(node_copy, node3);

    node_copy.try_put(1);
    g.wait_for_all();

    CHECK_MESSAGE((conformance::get_values(node2).size() == 0 && conformance::get_values(node3).size() == 1), "Copied node doesn`t copy successor, but copy number of predecessors");

    node0.try_put(1);
    g.wait_for_all();

    CHECK_MESSAGE((conformance::get_values(node2).size() == 1 && conformance::get_values(node3).size() == 0), "Copied node doesn`t copy predecessor, but copy number of predecessors");
}

template<typename Node, typename ...Args>
void test_priority(Args... args){
    std::size_t concurrency_limit = 1;
    oneapi::tbb::global_control control(oneapi::tbb::global_control::max_allowed_parallelism, concurrency_limit);

    oneapi::tbb::flow::graph g;

    oneapi::tbb::flow::continue_node<oneapi::tbb::flow::continue_msg> source(g,
                                          [](oneapi::tbb::flow::continue_msg){ return oneapi::tbb::flow::continue_msg();});

    conformance::first_functor<int>::first_id = -1;
    conformance::first_functor<int> low_functor(1);
    conformance::first_functor<int> high_functor(2);

    Node high(g, args..., high_functor, oneapi::tbb::flow::node_priority_t(1));
    Node low(g, args..., low_functor);

    make_edge(source, low);
    make_edge(source, high);

    source.try_put(oneapi::tbb::flow::continue_msg());

    g.wait_for_all();
    
    CHECK_MESSAGE((conformance::first_functor<int>::first_id == 2), "High priority node should execute first");
}

template<typename Node, typename ...Args>
void test_concurrency(Args...){
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
        Node fnode(g, num_threads, counter);

        conformance::test_push_receiver<int> sink(g);

        make_edge(fnode, sink);

        for(int i = 0; i < 500; ++i){
            fnode.try_put(i);
        }
        g.wait_for_all();
    }
}

template<typename Node, typename ...Args>
void test_rejecting(Args...){
    oneapi::tbb::flow::graph g;

    worker_body<int> body;
    Node fnode(g, oneapi::tbb::flow::serial, body);

    conformance::test_push_receiver<int> sink(g);

    make_edge(fnode, sink);

    bool try_put_state;

    for(int i = 0; i < 10; ++i){
        try_put_state = fnode.try_put(i);
    }

    g.wait_for_all();
    CHECK_MESSAGE((conformance::get_values(sink).size() == 1), "Messages should be rejected while the first is being processed");
    CHECK_MESSAGE((!try_put_state), "`try_put()' should returns `false' after rejecting");
}

template<typename Node, typename ...Args>
void test_output_input_class(Args...){
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

template<typename Node, typename CountingBody, typename ...Args>
void test_output_input_class(Args...){
    using namespace oneapi::tbb::flow;

    conformance::passthru_body<CountingBody> fun;

    graph g;
    Node node1(g, unlimited, fun);
    conformance::test_push_receiver<CountingBody> node2(g);
    make_edge(node1, node2);
    CountingBody b1;
    CountingBody b2;
    node1.try_put(b1);
    g.wait_for_all();
    node2.try_get(b2);
    DOCTEST_WARN_MESSAGE( (b1.copies_count > 0), "The type Input must meet the DefaultConstructible and CopyConstructible requirements");
    DOCTEST_WARN_MESSAGE( (b2.is_copy), "The type Output must meet the CopyConstructible requirements");
}

template<typename Node>
void test_output_class(){
    using namespace oneapi::tbb::flow;

    conformance::passthru_body<conformance::CountingObject<int>> fun;

    graph g;
    Node node1(g, fun);
    conformance::test_push_receiver<conformance::CountingObject<int>> node2(g);
    make_edge(node1, node2);
    
    if constexpr(!std::is_same<Node, oneapi::tbb::flow::input_node<conformance::CountingObject<int>>>::value){
        node1.try_put(oneapi::tbb::flow::continue_msg());
    }else{
        node1.activate();
    }
    g.wait_for_all();
    conformance::CountingObject<int> b;
    node2.try_get(b);
    DOCTEST_WARN_MESSAGE( (b.is_copy), "The type Output must meet the CopyConstructible requirements");
}

}
#endif // __TBB_test_conformance_conformance_flowgraph_H
