/*
    Copyright (c) 2022 Intel Corporation

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

#if _WIN32 || _WIN64
#define _CRT_SECURE_NO_WARNINGS
#endif

#define __TBB_NO_IMPLICIT_LINKAGE 1
#include "../../src/tbb/assert_impl.h" // Out-of-line TBB assertion handling routines are instantiated here.
#include "src/tbb/static_string.h"


#include "common/test.h"
#include "common/utils.h"

TEST_CASE("Test creation") {
    using tbb::detail::r1::static_string;
    static_string<2> s;
    REQUIRE_MESSAGE(s.size() == 0, "Wrong size of empty string");
    REQUIRE_MESSAGE(std::strlen(s.c_str()) == 0, "c_str() for empty string should not contain any characters");
}

TEST_CASE("Test creation of specific size") {
    using tbb::detail::r1::static_string;
    static_string<2> s(2);
    REQUIRE_MESSAGE(s.size() == 2, "Wrong size of non empty string");
    REQUIRE_MESSAGE(std::count(s.data(), s.data() + s.size(), 0) == s.size(), "constructor should zero-fill the buffer");
    REQUIRE_MESSAGE(std::strlen(s.c_str()) == 0, "c_str() for zero-filled string should not contain any characters");
}

TEST_CASE("Test creation over size"
        * doctest::skip() //Test needs to be revised as implementation uses assertions instead of exceptions
) {
    using tbb::detail::r1::static_string;
    static_string<2> s(10);
    REQUIRE_MESSAGE(s.size() == s.max_size(), "size of static_string should not exceed max_size");
}

TEST_CASE("Test operator=") {
    using tbb::detail::r1::static_string;

    static_string<10> s;

    s = "not empty";
    REQUIRE_MESSAGE(s.size() == std::strlen("not empty"), "Wrong size of result string");
    REQUIRE_MESSAGE(std::string(s.c_str()) == "not empty", "");
}

TEST_CASE("Test +=") {
    using tbb::detail::r1::static_string;

    static_string<10> s;
    s += "";

    REQUIRE_MESSAGE(s.size() == 0, "adding empty c_string_view should not alter the string");
    REQUIRE_MESSAGE(std::string(s.c_str()).empty(), "adding empty c_string_view should not alter the string");

    s += "test";
    REQUIRE_MESSAGE(s.size() == std::strlen("test"), "Wrong size of result string");
    REQUIRE_MESSAGE(std::string(s.c_str()) == "test", "");

    s += "twice";
    REQUIRE_MESSAGE(s.size() == (std::string("test") + "twice").size(), "Wrong size of result string");
    REQUIRE_MESSAGE(std::string(s.c_str()) == std::string("test") + "twice", "");
}

TEST_CASE("Test append basic") {
    using tbb::detail::r1::static_string;

    static_string<10> s;
    s.append("", 0);

    REQUIRE_MESSAGE(s.size() == 0, "adding empty c style string should not alter the string");
    REQUIRE_MESSAGE(std::string(s.c_str()).empty(), "adding empty c style string should not alter the string");

    std::string test{"test"};

    s.append(test.c_str(), test.size());
    REQUIRE_MESSAGE(s.size() == test.size(), "Wrong size of result string");
    REQUIRE_MESSAGE(std::string(s.c_str()) == test, "");

    std::string twice {"twice"};
    s.append(twice.c_str(), twice.size());
    REQUIRE_MESSAGE(s.size() == (test + twice).size(), "Wrong size of result string");
    REQUIRE_MESSAGE(std::string(s.c_str()) == test + twice, "");
}

TEST_CASE("Test append clamping") {
    using tbb::detail::r1::static_string;

    static_string<10> s;
    std::string test{"too long string to fit in 10 characters"};

    REQUIRE_MESSAGE(test.size() > s.max_size() , "Incorrect test setup, test is intended to test clamping conditions");
    s.append(test.c_str(), test.size());
    REQUIRE_MESSAGE(s.size() == s.max_size(), "Wrong size of result string");
    REQUIRE_MESSAGE(std::string(s.c_str()) == test.substr(0, s.max_size()), "");
}

TEST_CASE("Test clear") {
    using tbb::detail::r1::static_string;
    static_string<10> s;

    s="test";
    s.clear();
    REQUIRE_MESSAGE(s.size() == 0, "clear() should empty the string");
    REQUIRE_MESSAGE(std::strlen(s.c_str()) == 0, "clear() should empty the string");
}

TEST_CASE("Test resize") {
    using tbb::detail::r1::static_string;
    static_string<10> s;

    s.resize(2,'a');
    REQUIRE_MESSAGE(s.size() == 2, "resize should change the size");
    REQUIRE_MESSAGE(std::string(s.c_str()) == "aa", "resize should initialize new string characters");

    s.resize(1,'a');
    REQUIRE_MESSAGE(s.size() == 1, "resize should change the size");
    REQUIRE_MESSAGE(std::string(s.c_str()) == "a", "");
}

TEST_CASE("Test resize overgrow") {
    using tbb::detail::r1::static_string;
    static_string<2> s;

    s.resize(10,'a');
    REQUIRE_MESSAGE(s.size() == 2, "resize should respect maximum size");
    REQUIRE_MESSAGE(std::string(s.c_str()) == "aa", "resize should initialize new string characters");
}
