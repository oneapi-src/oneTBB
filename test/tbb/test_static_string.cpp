/*
 * test_internal_static_string.cpp
 *
 *  Created on: Nov 10, 2021
 *      Author: anton
 */

#if _WIN32 || _WIN64
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "src/tbb/static_string.h"


#include "common/test.h"
#include "common/utils.h"

TEST_CASE("Test creation") {
    using tbb::detail::r1::static_string;
    REQUIRE_MESSAGE( static_string<2>{}.size() == 0, "Wrong size of empty string");

}

TEST_CASE("Test assignment") {
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

    //FIXME: add clamping logic to the test
//    s += "triple";
//    REQUIRE_MESSAGE(s.size() == (std::string("test") += std::string("twice") += "triple").size(), "Wrong size of result string");
//    REQUIRE_MESSAGE(std::string(s.c_str()) == (std::string("test") += std::string("twice") += "triple"), "");
}
