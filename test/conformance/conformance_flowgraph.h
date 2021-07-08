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

namespace conformance {

constexpr int expected = 5;

template<typename V>
using test_push_receiver = oneapi::tbb::flow::queue_node<V>;

template<typename V>
std::vector<V> get_values( test_push_receiver<V>& rr ) {
    std::vector<V> messages;
    int val = 0;
    for(V tmp; rr.try_get(tmp); ++val) {
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
              !first_id.compare_exchange_weak(old_value, my_id));
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

template<typename OutputType>
struct counting_functor {
    OutputType return_value;

    static std::atomic<std::size_t> execute_count;

    counting_functor( OutputType value = OutputType() ) : return_value(value) {
        execute_count = 0;
    }

    template<typename InputType>
    OutputType operator()( InputType ) {
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
       if(execute_count > std::size_t(return_value)) {
           fc.stop();
           return return_value;
       }
       return return_value;
    }

    void operator()( oneapi::tbb::flow::continue_msg , oneapi::tbb::flow::async_node<oneapi::tbb::flow::continue_msg, int>::gateway_type& g) {
        ++execute_count;
        g.try_put(return_value);
    }
};

template<typename OutputType>
std::atomic<std::size_t> counting_functor<OutputType>::execute_count;

template<typename OutputType>
struct dummy_functor {

    template<typename InputType>
    OutputType operator()( InputType ) {
       return OutputType();
    }

    void operator()( const int& argument, oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type &op ) {
       std::get<0>(op).try_put(argument);
    }

    void operator()( const oneapi::tbb::flow::continue_msg&, oneapi::tbb::flow::multifunction_node<oneapi::tbb::flow::continue_msg, std::tuple<int>>::output_ports_type &op ) {
       std::get<0>(op).try_put(1);
    }

    void operator()( int num , oneapi::tbb::flow::async_node<int, int>::gateway_type& g) {
        g.try_put(num);
    }

    OutputType operator()( oneapi::tbb::flow_control & fc ) {
        static bool check = false;
        if(check) {
            check = false;
            fc.stop();
            return OutputType();
        }
        check = true;
        return OutputType();
    }
};

struct wait_flag_body {
    static std::atomic<bool> flag;

    wait_flag_body() {
        flag.store(false);
    }

    template<typename InputType>
    InputType operator()( InputType ) {
        while(!flag.load()) { };
        return InputType();
    }

    void operator()( const int& argument, oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type &op ) {
        while(!flag.load()) { };
        std::get<0>(op).try_put(argument);
    }
};

std::atomic<bool> wait_flag_body::flag{false};

struct concurrency_peak_checker_body {
    std::size_t required_max_concurrency = 0;

    concurrency_peak_checker_body( std::size_t req_max_concurrency = 0 ) : required_max_concurrency(req_max_concurrency) {}

    concurrency_peak_checker_body( const concurrency_peak_checker_body & ) = default;

    int operator()( oneapi::tbb::flow_control & fc ) {
        static int counter = 0;
        utils::ConcurrencyTracker ct;
        if(++counter > 500) {
            counter = 0;
            fc.stop();
            return 1;
        }
        utils::doDummyWork(1000);
        CHECK_MESSAGE((int)utils::ConcurrencyTracker::PeakParallelism() <= required_max_concurrency, "Input node is serial and its body never invoked concurrently");
        return 1;
    }

    int operator()( int ) {
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

    void operator()( const int& argument , oneapi::tbb::flow::async_node<int, int>::gateway_type& g) {
        utils::ConcurrencyTracker ct;
        utils::doDummyWork(1000);
        CHECK_MESSAGE((int)utils::ConcurrencyTracker::PeakParallelism() <= required_max_concurrency, "Measured parallelism is not expected");
        g.try_put(argument);
    }
};

template<typename O, typename I = int>
struct CountingObject {
    std::size_t copy_count;
    mutable std::size_t copies_count;
    std::size_t assign_count;
    mutable std::size_t assignes_count;
    std::size_t move_count;
    bool is_copy;

    CountingObject():
        copy_count(0), copies_count(0), assign_count(0), assignes_count(0), move_count(0), is_copy(false) {}

    CountingObject( const CountingObject<O>& other ):
        copy_count(other.copy_count + 1), is_copy(true) {
            ++other.copies_count;
        }

    CountingObject& operator=( const CountingObject<O>& other ) {
        assign_count = other.assign_count + 1;
        ++other.assignes_count;
        is_copy = true;
        return *this;
    }

    CountingObject( CountingObject<O>&& other ):
         copy_count(other.copy_count), copies_count(other.copies_count),
         assign_count(other.assign_count), assignes_count(other.assignes_count),
         move_count(other.move_count + 1), is_copy(other.is_copy) {}

    CountingObject& operator=( CountingObject<O>&& other ) {
        copy_count = other.copy_count;
        copies_count = other.copies_count;
        assign_count = other.assign_count;
        assignes_count = other.assignes_count;
        move_count = other.move_count + 1;
        is_copy = other.is_copy;
        return *this;
    }

    template<typename InputType>
    O operator()( InputType ) {
        return O();
    }

    void operator()( const I& argument, oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type &op ) {
       std::get<0>(op).try_put(argument);
    }

    O operator()( oneapi::tbb::flow_control & fc ) {
        static int counter = 0;
        if(++counter > 5) {
            counter = 0;
            fc.stop();
            return O();
        }
        return O();
    }

    void operator()( oneapi::tbb::flow::continue_msg , oneapi::tbb::flow::async_node<oneapi::tbb::flow::continue_msg, int>::gateway_type& g) {
        g.try_put(1);
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
        if(++counter > 5) {
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
void test_body_exec(Args... args) {
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
void test_body_copying(Node& tested_node, Body& base_body) {
    using namespace oneapi::tbb::flow;

    Body b2 = copy_body<Body, Node>(tested_node);

    CHECK_MESSAGE((base_body.copy_count + 1 < b2.copy_count), "copy_body and constructor should copy bodies");
}

template<typename Node, typename Body, typename ...Args>
void test_copy_body(Args... args) {
    using namespace oneapi::tbb::flow;

    Body base_body;

    graph g;

    Node tested_node(g, args..., base_body);

    Body b2 = copy_body<Body, Node>(tested_node);

    CHECK_MESSAGE((base_body.copy_count + 1 < b2.copy_count), "copy_body and constructor should copy bodies");
}

template<typename Node, typename ...Args>
void test_buffering(Args... args) {
    oneapi::tbb::flow::graph g;

    conformance::dummy_functor<int> fun;

    Node tested_node(g, args..., fun);
    oneapi::tbb::flow::limiter_node<int> rejecter(g, 0);

    oneapi::tbb::flow::make_edge(tested_node, rejecter);

    int tmp = -1;
    #ifndef INPUT_NODE
        tested_node.try_put(oneapi::tbb::flow::continue_msg());
        g.wait_for_all();
        #ifndef MULTIFUNCTION_NODE
            CHECK_MESSAGE((tested_node.try_get(tmp) == false), "try_get after rejection should not succeed");
        #else
            CHECK_MESSAGE((std::get<0>(tested_node.output_ports()).try_get(tmp) == false), "try_get after rejection should not succeed");
        #endif
        CHECK_MESSAGE((tmp == -1), "try_get after rejection should not alter passed value");
    #else
        tested_node.activate();
        g.wait_for_all();
        CHECK_MESSAGE((tested_node.try_get(tmp) == true), "try_get after rejection should succeed");
        CHECK_MESSAGE((tmp == 0), "try_get should return correct value");
    #endif
}

template<typename Node, typename ...Args>
void test_buffering_for_buffering_nodes(Args... args) {
    oneapi::tbb::flow::graph g;

    Node tested_node(g, args...);
    oneapi::tbb::flow::limiter_node<int> rejecter(g, 0);

    oneapi::tbb::flow::make_edge(tested_node, rejecter);

    tested_node.try_put(1);
    g.wait_for_all();

    int tmp = -1;
    #ifndef SEQUENCER_NODE
        CHECK_MESSAGE( (tested_node.try_get(tmp) == true), "try_get after rejection should succeed");
        CHECK_MESSAGE( (tmp == 1), "try_get after rejection should set value");
    #else
        CHECK_MESSAGE((tested_node.try_get(tmp) == false), "try_get after rejection should not succeed");
        CHECK_MESSAGE((tmp == -1), "try_get after rejection should not alter passed value");
    #endif
}

template<typename Node, typename ...Args>
void test_forwarding(std::size_t messages_recieved, Args... args) {
    oneapi::tbb::flow::graph g;
    
    Node testing_node(g, args...);
    std::vector<conformance::test_push_receiver<int>*> receiver_nodes(10);

    for(std::size_t i = 0; i < 10; ++i) {
        conformance::test_push_receiver<int>* node = new conformance::test_push_receiver<int>(g);
        oneapi::tbb::flow::make_edge(testing_node, *node);
        receiver_nodes[i] = node;
    }

    #ifndef INPUT_NODE
        #ifndef BUFFERING_NODES
            testing_node.try_put(oneapi::tbb::flow::continue_msg());
        #else
            testing_node.try_put(expected);
        #endif
    #else
        __TBB_ASSERT(expected == messages_recieved, "For correct execution of test");
        testing_node.activate();
    #endif

    g.wait_for_all();
    for(std::size_t i = 0; i < 10; ++i) {
        auto values = conformance::get_values(*receiver_nodes[i]);
        CHECK_MESSAGE((values.size() == messages_recieved), std::string("Descendant of the node must receive " + std::to_string(messages_recieved) + " message."));
        CHECK_MESSAGE((values[0] == expected), "Value passed is the actual one received.");
        delete receiver_nodes[i];
    }
}

template<typename Node, typename ...Args>
#include <iostream>
void test_forwarding_single_push(Args... args) {
    oneapi::tbb::flow::graph g;

    Node tested_node(g, args...);
    conformance::test_push_receiver<int> suc_node1(g);
    conformance::test_push_receiver<int> suc_node2(g);

    oneapi::tbb::flow::make_edge(tested_node, suc_node1);
    oneapi::tbb::flow::make_edge(tested_node, suc_node2);

    tested_node.try_put(0);
    g.wait_for_all();

    auto values1 = conformance::get_values(suc_node1);
    auto values2 = conformance::get_values(suc_node2);
    CHECK_MESSAGE((values1.size() != values2.size()), "Only one descendant the node needs to receive");
    CHECK_MESSAGE((values1.size() + values2.size() == 1), "All messages need to be received");


    tested_node.try_put(1);
    g.wait_for_all();

    auto values3 = conformance::get_values(suc_node1);
    auto values4 = conformance::get_values(suc_node2);
    CHECK_MESSAGE((values3.size() != values4.size()), "Only one descendant the node needs to receive");
    CHECK_MESSAGE((values3.size() + values4.size() == 1), "All messages need to be received");

    #ifdef QUEUE_NODE
        CHECK_MESSAGE((values1[0] == 0), "Value passed is the actual one received");
        CHECK_MESSAGE((values3[0] == 1), "Value passed is the actual one received");
    #else
        if(values1.size() == 1) {
            CHECK_MESSAGE((values1[0] == 0), "Value passed is the actual one received");
        }else{
            CHECK_MESSAGE((values2[0] == 0), "Value passed is the actual one received");
        }
    #endif
}

template<typename Node, typename I, typename O>
void test_inheritance() {
    using namespace oneapi::tbb::flow;

    CHECK_MESSAGE((std::is_base_of<graph_node, Node>::value), "Node should be derived from graph_node");
    CHECK_MESSAGE((std::is_base_of<receiver<I>, Node>::value), "Node should be derived from receiver<Input>");
    CHECK_MESSAGE((std::is_base_of<sender<O>, Node>::value), "Node should be derived from sender<Output>");
}

template<typename Node, typename CountingBody>
void test_copy_ctor() {
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
void test_copy_ctor_for_buffering_nodes(Args... args) {
    oneapi::tbb::flow::graph g;

    conformance::dummy_functor<int> fun;

    Node tested_node(g, args...);
    oneapi::tbb::flow::continue_node<int> pred_node(g, fun);
    conformance::test_push_receiver<int> suc_node1(g);
    conformance::test_push_receiver<int> suc_node2(g);

    oneapi::tbb::flow::make_edge(pred_node, tested_node);
    oneapi::tbb::flow::make_edge(tested_node, suc_node1);

    #ifdef OVERWRITE_NODE
        tested_node.try_put(1);
    #endif

    Node node_copy(tested_node);

    #ifdef OVERWRITE_NODE
        int tmp;
        CHECK_MESSAGE((!node_copy.is_valid() && !node_copy.try_get(tmp)), "The buffered value is not copied from src");
        conformance::get_values(suc_node1);
    #endif

    oneapi::tbb::flow::make_edge(node_copy, suc_node2);

    node_copy.try_put(0);
    g.wait_for_all();

    CHECK_MESSAGE((conformance::get_values(suc_node1).size() == 0 && conformance::get_values(suc_node2).size() == 1), "Copied node doesn`t copy successor");

    #ifdef OVERWRITE_NODE
        node_copy.clear();
        tested_node.clear();
    #endif

    pred_node.try_put(oneapi::tbb::flow::continue_msg());
    g.wait_for_all();

    CHECK_MESSAGE((conformance::get_values(suc_node1).size() == 1 && conformance::get_values(suc_node2).size() == 0), "Copied node doesn`t copy predecessor");
}

template<typename Node, typename ...Args>
void test_priority(Args... args) {
    std::size_t concurrency_limit = 1;
    oneapi::tbb::global_control control(oneapi::tbb::global_control::max_allowed_parallelism, concurrency_limit);

    oneapi::tbb::flow::graph g;

    oneapi::tbb::flow::continue_node<oneapi::tbb::flow::continue_msg> source(g,
                                          [](oneapi::tbb::flow::continue_msg) { return oneapi::tbb::flow::continue_msg();});

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

template<typename Node>
void test_concurrency() {
    auto max_num_threads = oneapi::tbb::this_task_arena::max_concurrency();

    oneapi::tbb::global_control c(oneapi::tbb::global_control::max_allowed_parallelism,
                                  max_num_threads);

    std::vector<int> threads_count = {1, oneapi::tbb::flow::serial, max_num_threads, oneapi::tbb::flow::unlimited};

    if(max_num_threads > 2) {
        threads_count.push_back(max_num_threads / 2);
    }

    for(auto num_threads : threads_count) {
        utils::ConcurrencyTracker::Reset();
        int expected_threads = num_threads;
        if(num_threads == oneapi::tbb::flow::unlimited) {
            expected_threads = max_num_threads;
        }
        if(num_threads == oneapi::tbb::flow::serial) {
            expected_threads = 1;
        }
        oneapi::tbb::flow::graph g;
        conformance::concurrency_peak_checker_body counter(expected_threads);
        Node fnode(g, num_threads, counter);

        conformance::test_push_receiver<int> sink(g);

        make_edge(fnode, sink);

        for(int i = 0; i < 500; ++i) {
            fnode.try_put(i);
        }
        g.wait_for_all();
    }
}

template<typename Node>
void test_rejecting() {
    oneapi::tbb::flow::graph g;

    wait_flag_body body;
    Node fnode(g, oneapi::tbb::flow::serial, body);

    conformance::test_push_receiver<int> sink(g);

    make_edge(fnode, sink);

    fnode.try_put(0);

    CHECK_MESSAGE((!fnode.try_put(1)), "Messages should be rejected while the first is being processed");

    wait_flag_body::flag = true;

    g.wait_for_all();
    CHECK_MESSAGE((conformance::get_values(sink).size() == 1), "Messages should be rejected while the first is being processed");
}

template<typename Node, typename CountingBody>
void test_output_input_class() {
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
void test_output_class() {
    using namespace oneapi::tbb::flow;

    conformance::passthru_body<conformance::CountingObject<int>> fun;

    graph g;
    Node node1(g, fun);
    conformance::test_push_receiver<conformance::CountingObject<int>> node2(g);
    make_edge(node1, node2);
    
    #ifndef INPUT_NODE
        node1.try_put(oneapi::tbb::flow::continue_msg());
    #else
        node1.activate();
    #endif

    g.wait_for_all();
    conformance::CountingObject<int> b;
    node2.try_get(b);
    DOCTEST_WARN_MESSAGE( (b.is_copy), "The type Output must meet the CopyConstructible requirements");
}

template<typename Node>
void test_with_reserving_join_node_class() {
    using namespace oneapi::tbb::flow;

    graph g;

    function_node<int, int> static_result_computer_n(
        g, serial,
        [&](const int& msg) {
            // compute the result using incoming message and pass it further, e.g.:
            int result = int((msg >> 2) / 4);
            return result;
        });
    Node tested_node(g); // for buffering once computed value

    buffer_node<int> buffer_n(g);
    join_node<std::tuple<int, int>, reserving> join_n(g);
    
    std::atomic<int> number{2};
    function_node<std::tuple<int, int>> consumer_n(
        g, unlimited,
        [&](const std::tuple<int, int>& arg) {
            // use the precomputed static result along with dynamic data
            #ifdef OVERWRITE_NODE
                CHECK_MESSAGE((std::get<0>(arg) == int((number >> 2) / 4)), "A overwrite_node store a single item that can be overwritten");
            #else
                CHECK_MESSAGE((std::get<0>(arg) == int((number >> 2) / 4)), "A write_once_node store a single item that cannot be overwritten");
            #endif
        });

    make_edge(static_result_computer_n, tested_node);
    make_edge(tested_node, input_port<0>(join_n));
    make_edge(buffer_n, input_port<1>(join_n));
    make_edge(join_n, consumer_n);

    // do one-time calculation that will be reused many times further in the graph
    static_result_computer_n.try_put(number);

    for (int i = 0; i < 50; i++) {
        buffer_n.try_put(i);
    }

    #ifdef OVERWRITE_NODE
        number = 3;
        static_result_computer_n.try_put(number);
        for (int i = 0; i < 50; i++) {
            buffer_n.try_put(i);
        }
    #endif

    g.wait_for_all();
}

}
#endif // __TBB_test_conformance_conformance_flowgraph_H
