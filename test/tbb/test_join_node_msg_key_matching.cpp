/*
    Copyright (c) 2005-2020 Intel Corporation

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

// Message based key matching is a preview feature
#define TBB_PREVIEW_FLOW_GRAPH_FEATURES 1

#include "common/config.h"

#include "test_join_node.h"

//! \file test_join_node_msg_key_matching.cpp
//! \brief Test for [preview] functionality

#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT
struct message_key {
    int my_key;
    double my_value;

    int key() const { return my_key; }

    operator size_t() const { return my_key; }

    bool operator==(const message_key& rhs) { return my_value == rhs.my_value; }
};

void test_deduction_guides() {
    using namespace tbb::flow;
    using tuple_type = std::tuple<message_key, message_key>;

    graph g;
    broadcast_node<message_key> bm1(g), bm2(g);
    broadcast_node<tuple_type> bm3(g);
    join_node<tuple_type, key_matching<int> > j0(g);

#if __TBB_PREVIEW_FLOW_GRAPH_NODE_SET
    join_node j1(follows(bm1, bm2), key_matching<int>());
    static_assert(std::is_same_v<decltype(j1), join_node<tuple_type, key_matching<int>>>);

    join_node j2(precedes(bm3), key_matching<int>());
    static_assert(std::is_same_v<decltype(j2), join_node<tuple_type, key_matching<int>>>);
#endif

    join_node j3(j0);
    static_assert(std::is_same_v<decltype(j3), join_node<tuple_type, key_matching<int>>>);
}
#endif

//! Serial test with different tuple sizes
//! \brief \ref error_guessing
TEST_CASE("Serial test"){
    generate_test<serial_test, std::tuple<MyMessageKeyWithBrokenKey<int, double>, MyMessageKeyWithoutKey<int, float> >, message_based_key_matching<int> >::do_test();
    generate_test<serial_test, std::tuple<MyMessageKeyWithoutKeyMethod<std::string, double>, MyMessageKeyWithBrokenKey<std::string, float> >, message_based_key_matching<std::string> >::do_test();
#if MAX_TUPLE_TEST_SIZE >= 3
    generate_test<serial_test, std::tuple<MyMessageKeyWithoutKey<std::string, double>, MyMessageKeyWithoutKeyMethod<std::string, float>, MyMessageKeyWithBrokenKey<std::string, int> >, message_based_key_matching<std::string&> >::do_test();
#endif
#if MAX_TUPLE_TEST_SIZE >= 7
    generate_test<serial_test, std::tuple<
        MyMessageKeyWithoutKey<std::string, double>,
        MyMessageKeyWithoutKeyMethod<std::string, int>,
        MyMessageKeyWithBrokenKey<std::string, int>,
        MyMessageKeyWithoutKey<std::string, size_t>,
        MyMessageKeyWithoutKeyMethod<std::string, int>,
        MyMessageKeyWithBrokenKey<std::string, short>,
        MyMessageKeyWithoutKey<std::string, threebyte>
    >, message_based_key_matching<std::string&> >::do_test();
#endif

#if MAX_TUPLE_TEST_SIZE >= 10
    generate_test<parallel_test, std::tuple<
        MyMessageKeyWithoutKeyMethod<std::string, double>,
        MyMessageKeyWithBrokenKey<std::string, int>,
        MyMessageKeyWithoutKey<std::string, int>,
        MyMessageKeyWithoutKeyMethod<std::string, size_t>,
        MyMessageKeyWithBrokenKey<std::string, int>,
        MyMessageKeyWithoutKeyMethod<std::string, short>,
        MyMessageKeyWithoutKeyMethod<std::string, threebyte>,
        MyMessageKeyWithBrokenKey<std::string, int>,
        MyMessageKeyWithoutKeyMethod<std::string, threebyte>,
        MyMessageKeyWithBrokenKey<std::string, size_t>
    >, message_based_key_matching<std::string&> >::do_test();
#endif
}

//! Parallel test with special key types
//! \brief \ref error_guessing
TEST_CASE("Parallel test"){
    generate_test<parallel_test, std::tuple<MyMessageKeyWithBrokenKey<int, double>, MyMessageKeyWithoutKey<int, float> >, message_based_key_matching<int> >::do_test();
    generate_test<parallel_test, std::tuple<MyMessageKeyWithoutKeyMethod<int, double>, MyMessageKeyWithBrokenKey<int, float> >, message_based_key_matching<int&> >::do_test();
    generate_test<parallel_test, std::tuple<MyMessageKeyWithoutKey<std::string, double>, MyMessageKeyWithoutKeyMethod<std::string, float> >, message_based_key_matching<std::string&> >::do_test();
}

#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT
//! Test deduction guides
//! \brief \ref requirement
TEST_CASE("Deduction guides test"){
    test_deduction_guides();
}
#endif

//! Serial test with special key types
//! \brief \ref error_guessing
TEST_CASE("Serial test"){
    generate_test<serial_test, std::tuple<MyMessageKeyWithBrokenKey<int, double>, MyMessageKeyWithoutKey<int, float> >, message_based_key_matching<int> >::do_test();
    generate_test<serial_test, std::tuple<MyMessageKeyWithoutKeyMethod<std::string, double>, MyMessageKeyWithBrokenKey<std::string, float> >, message_based_key_matching<std::string> >::do_test();
}
