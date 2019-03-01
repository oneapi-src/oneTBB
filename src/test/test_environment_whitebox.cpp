/*
    Copyright (c) 2005-2019 Intel Corporation

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

#include "harness.h"
#include "../tbb/tbb_environment.h"

#include <time.h>
#include <string>
#include <algorithm>

// For WIN8UI applications reading and writing the environment variables
// is prohibited due to the platform limitations
#if !__TBB_WIN8UI_SUPPORT

void set_and_get_tbb_version(const char* value, bool expected_result){
    // TODO: consider to replace TBB_VERSION with test specific or arbitrary variable
    const char* name = "TBB_VERSION";
    Harness::SetEnv(name, value);
    bool result = tbb::internal::GetBoolEnvironmentVariable(name);
    ASSERT(result == expected_result, "Wrong GetBoolEnvironmentVariable return value");
    Harness::SetEnv(name, "");
}

Harness::FastRandom rnd(12345);

struct random_character_generator {
    char operator()() {
        return rnd.get() % 128; // 127 - the last ASCII symbol
    }
};

void test_random_environment_variable(size_t length) {
    ASSERT(length != 0, "Requested random string cannot be empty");
    std::string rand_string(length, ' ');
    std::generate(rand_string.begin(), rand_string.end(), random_character_generator());
    bool expected_result = false;
    for (unsigned i = 0; rand_string[i]; i++)
        if (rand_string[i] == '1') {
            // if we found more the one '1' character -> return false
            expected_result = !expected_result;
            if (!expected_result) break;
        }
        else if (rand_string[i] != ' ') {
            // if we found some character other than ' ' and '1'  -> return false
            expected_result = false;
            break;
        }

    set_and_get_tbb_version(rand_string.c_str(), expected_result);
}

void test_get_bool_environment_variable() {
    // Test undefined variable
    ASSERT(!tbb::internal::GetBoolEnvironmentVariable("TBB_VERSION"),
           "TBB_VERSION should not be defined in the beginning of the test");

    set_and_get_tbb_version("", false);
    set_and_get_tbb_version(" ", false);
    set_and_get_tbb_version("1", true);
    set_and_get_tbb_version(" 1 ", true);
    set_and_get_tbb_version("1               ", true);
    set_and_get_tbb_version("            1               ", true);
    set_and_get_tbb_version("            1", true);
    set_and_get_tbb_version(" 11", false);
    set_and_get_tbb_version("111111", false);
    set_and_get_tbb_version("1 1", false);
    set_and_get_tbb_version(" 1 abc?", false);
    set_and_get_tbb_version("1;", false);
    set_and_get_tbb_version(" d ", false);
    set_and_get_tbb_version("0", false);
    set_and_get_tbb_version("0 ", false);
    set_and_get_tbb_version("000000", false);
    set_and_get_tbb_version("01", false);
    set_and_get_tbb_version("00000001", false);
    set_and_get_tbb_version("ABCDEFG", false);
    set_and_get_tbb_version("2018", false);
    set_and_get_tbb_version("ABC_123", false);
    set_and_get_tbb_version("true", false);

    size_t length = 10000;

    for(size_t i =0; i < 10; ++i) {
        test_random_environment_variable((rnd.get() % length) + 1);
    }

#if _WIN32 || _WIN64
    // Environment variable length is limited by 32K on Windows systems
    size_t large_length = 32000;
#else
    size_t large_length = 1000000;
#endif
    set_and_get_tbb_version(std::string(large_length, 'A').c_str(), false);
    set_and_get_tbb_version((std::string(large_length, ' ')+'1').c_str(), true);
    test_random_environment_variable(large_length);
}

#else // __TBB_WIN8UI_SUPPORT

void test_get_bool_environment_variable() {
    for(size_t i = 0; i < 100; ++i) {
        ASSERT(!tbb::internal::GetBoolEnvironmentVariable("TBB_VERSION"),
               "GetBoolEnvironmentVariable should always return false for UWP applications");
    }
}
#endif // __TBB_WIN8UI_SUPPORT

int TestMain() {
    test_get_bool_environment_variable();
    return Harness::Done;
}
