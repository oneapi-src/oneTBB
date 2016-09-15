/*
    Copyright 2005-2015 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

#ifndef tbb_test_harness_mic_H
#define tbb_test_harness_mic_H

#if ! __TBB_DEFINE_MIC
    #error test/harness_mic.h should be included only when building for Intel(R) Many Integrated Core Architecture
#endif

// test for unifed sources. See makefiles
#undef HARNESS_INCOMPLETE_SOURCES

#include <stdlib.h>
#include <stdio.h>

#define TBB_TEST_LOW_WORKLOAD 1

#define REPORT_FATAL_ERROR  REPORT
#define HARNESS_EXPORT

#if __TBB_MIC_NATIVE
    #define HARNESS_EXIT_ON_ASSERT 1
    #define __TBB_PLACEMENT_NEW_EXCEPTION_SAFETY_BROKEN 1
#else
    #define HARNESS_TERMINATE_ON_ASSERT 1
#endif

#endif /* tbb_test_harness_mic_H */
