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

template <typename OutputType = int>
struct passthru_body {
    OutputType operator()( const OutputType& i ) {
        return i;
    }

    OutputType operator()( oneapi::tbb::flow::continue_msg ) {
       return OutputType();
    }

    void operator()( const int& argument, oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type &op ) {
       std::get<0>(op).try_put(argument);
    }

};

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

    void operator()( const OutputType& argument, oneapi::tbb::flow::multifunction_node<int, std::tuple<int>>::output_ports_type &op ) {
        operator()(OutputType());
        std::get<0>(op).try_put(argument);
    }
};

template<typename OutputType>
std::atomic<int> first_functor<OutputType>::first_id;

template< typename OutputType>
struct counting_functor {
    OutputType my_value;

    static std::atomic<size_t> execute_count;

    counting_functor(OutputType value = OutputType()) : my_value(value) {

    }

    OutputType operator()( oneapi::tbb::flow::continue_msg ) {
       ++execute_count;
       return my_value;
    }

    OutputType operator()( OutputType ) {
       ++execute_count;
       return my_value;
    }
};

template<typename OutputType>
std::atomic<size_t> counting_functor<OutputType>::execute_count;

template< typename OutputType>
struct dummy_functor {
    OutputType operator()( oneapi::tbb::flow::continue_msg ) {
       return OutputType();
    }
    OutputType operator()( OutputType ) {
       return OutputType();
    }
};

struct barrier_body{
    static std::atomic<bool> flag;
    size_t required_max_concurrency = 0;

    barrier_body(size_t req_max_concurrency) : required_max_concurrency(req_max_concurrency){
        flag.store(false);
    }

    barrier_body() = default;

    barrier_body(const barrier_body &) = default;

    void operator()(oneapi::tbb::flow::continue_msg){
        while(!flag.load()){ };
    }

    int operator()(int) {
        utils::ConcurrencyTracker ct;
        utils::doDummyWork(1000);
        CHECK_MESSAGE((int)utils::ConcurrencyTracker::PeakParallelism() <= required_max_concurrency, "Measured parallelism is not expected");
        return 1;
    }
};

std::atomic<bool> barrier_body::flag{false};

template<typename O>
struct CountingObject{
    size_t copy_count;
    mutable size_t copies_count;
    size_t assign_count;
    mutable size_t assignes_count;
    size_t move_count;
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
};

template<typename Node, typename ...Args>
void test_body_exec_impl(Args... args){
    oneapi::tbb::flow::graph g;
    conformance::counting_functor<int> cf;
    cf.execute_count = 0;

    Node node1(g, args..., cf);

    const size_t n = 10;
    for(size_t i = 0; i < n; ++i) {
        CHECK_MESSAGE((node1.try_put(oneapi::tbb::flow::continue_msg()) == true),
                      "try_put of first node should return true");
    }
    g.wait_for_all();

    CHECK_MESSAGE( (cf.execute_count == n), "Body of the first node needs to be executed N times");
}

template<typename Node>
void test_body_copying(Node& tested_node, CountingObject<int>& base_body){
    using namespace oneapi::tbb::flow;

    CountingObject<int> b2 = copy_body<CountingObject<int>, Node>(tested_node);

    CHECK_MESSAGE( (base_body.copy_count + 1 < b2.copy_count), "copy_body and constructor should copy bodies");
}

template<typename Node, typename ...Args>
void test_copy_body_impl(Args... args){
    using namespace oneapi::tbb::flow;

    conformance::CountingObject<int> base_body;

    graph g;

    Node tested_node(g, args..., base_body);

    CountingObject<int> b2 = copy_body<CountingObject<int>, Node>(tested_node);

    CHECK_MESSAGE( (base_body.copy_count + 1 < b2.copy_count), "copy_body and constructor should copy bodies");
}

template<typename Node, typename ...Args>
void test_buffering_impl(Args... args){
    oneapi::tbb::flow::graph g;
    conformance::dummy_functor<int> fun;
    Node tested_node(g, args..., fun);
    oneapi::tbb::flow::limiter_node<int> rejecter(g, 0);

    oneapi::tbb::flow::make_edge(tested_node, rejecter);
    tested_node.try_put(oneapi::tbb::flow::continue_msg());

    int tmp = -1;
    CHECK_MESSAGE( (tested_node.try_get(tmp) == false), "try_get after rejection should not succeed");
    CHECK_MESSAGE( (tmp == -1), "try_get after rejection should not alter passed value");
    g.wait_for_all();
}

template<typename Node, typename ...Args>
void test_forvarding_impl(Args... args){
    oneapi::tbb::flow::graph g;
    const int expected = 5;
    conformance::counting_functor<int> fun(expected);

    Node node1(g, args..., fun);
    conformance::test_push_receiver<int> node2(g);
    conformance::test_push_receiver<int> node3(g);

    oneapi::tbb::flow::make_edge(node1, node2);
    oneapi::tbb::flow::make_edge(node1, node3);

    node1.try_put(oneapi::tbb::flow::continue_msg());
    g.wait_for_all();

    auto values2 = conformance::get_values(node2);
    auto values3 = conformance::get_values(node3);

    CHECK_MESSAGE( (values2.size() == 1), "Descendant of the node must receive one message.");
    CHECK_MESSAGE( (values3.size() == 1), "Descendant of the node must receive one message.");
    CHECK_MESSAGE( (values2[0] == expected), "Value passed is the actual one received.");
    CHECK_MESSAGE( (values2 == values3), "Value passed is the actual one received.");
}

template<typename Node, typename I, typename O>
void test_inheritance_impl(){
    using namespace oneapi::tbb::flow;

    CHECK_MESSAGE( (std::is_base_of<graph_node, Node>::value), "Node should be derived from graph_node");
    CHECK_MESSAGE( (std::is_base_of<receiver<I>, Node>::value), "Node should be derived from receiver<Input>");
    CHECK_MESSAGE( (std::is_base_of<sender<O>, Node>::value), "Node should be derived from sender<Output>");
}

template<typename Node, typename ...Args>
void test_priority_impl(Args... args){
    size_t concurrency_limit = 1;
    oneapi::tbb::global_control control(oneapi::tbb::global_control::max_allowed_parallelism, concurrency_limit);

    oneapi::tbb::flow::graph g;

    Node source(g, args..., [](oneapi::tbb::flow::continue_msg){ return oneapi::tbb::flow::continue_msg();});

    conformance::first_functor<int>::first_id = -1;
    conformance::first_functor<int> low_functor(1);
    conformance::first_functor<int> high_functor(2);

    Node high(g, args..., high_functor, oneapi::tbb::flow::node_priority_t(1));
    Node low(g, args..., low_functor);

    make_edge(source, low);
    make_edge(source, high);

    source.try_put(oneapi::tbb::flow::continue_msg());

    g.wait_for_all();
    
    CHECK_MESSAGE( (conformance::first_functor<int>::first_id == 2), "High priority node should execute first");
}

}
#endif // __TBB_test_conformance_conformance_flowgraph_H
