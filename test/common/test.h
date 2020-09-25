/*
    Copyright (c) 2020 Intel Corporation

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

#ifndef __TBB_test_common_test_H
#define __TBB_test_common_test_H

#include "config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#if TBB_USE_EXCEPTIONS
#define TBB_TEST_THROW(x) throw x
#else
#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#define TBB_TEST_THROW(x) FAIL("Exceptions are disabled")
#endif

#include "doctest.h"

#endif // __TBB_test_common_test_H
