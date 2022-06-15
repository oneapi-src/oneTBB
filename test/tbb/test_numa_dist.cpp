/*
    Copyright (c) 2005-2021 Intel Corporation

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

#include "common/test.h"

#include <stdio.h>
#include "tbb/parallel_for.h"
#include "tbb/global_control.h"
#include "tbb/enumerable_thread_specific.h"

#include "common/config.h"
#include "common/utils.h"
#include "common/utils_concurrency_limit.h"
#include "common/utils_report.h"
#include "common/vector_types.h"
#include "common/cpu_usertime.h"
#include "common/spin_barrier.h"
#include "common/exception_handling.h"
#include "common/concepts_common.h"
#include "test_partitioner.h"

#include <cstddef>
#include <vector>

//! \file test_parallel_for.cpp
//! \brief Test for [algorithms.parallel_for] specification

#if _MSC_VER
#pragma warning (push)
// Suppress conditional expression is constant
#pragma warning (disable: 4127)
#if __TBB_MSVC_UNREACHABLE_CODE_IGNORED
    // Suppress pointless "unreachable code" warning.
    #pragma warning (disable: 4702)
#endif
#if defined(_Wp64)
    // Workaround for overzealous compiler warnings in /Wp64 mode
    #pragma warning (disable: 4267)
#endif
#define _SCL_SECURE_NO_WARNINGS
#endif //#if _MSC_VER


struct numa {
    WORD processorGroupCount;
    std::vector<DWORD> numaProcessors;
    DWORD maxProcessors;
    numa() : processorGroupCount(GetMaximumProcessorGroupCount()), maxProcessors(GetActiveProcessorCount(ALL_PROCESSOR_GROUPS)){   
        numaProcessors.resize(processorGroupCount);
       for (WORD i = 0; i < processorGroupCount; i++) {
            this->numaProcessors[i] = GetActiveProcessorCount((i));
        }        
    }
};


void TestNumaDistribution(){
	std::vector<DWORD> validateProcgrp;
	validateProcgrp.resize(GetMaximumProcessorGroupCount());
	PROCESSOR_NUMBER proc;
	struct numa nodes;
	GetThreadIdealProcessorEx(GetCurrentThread(), &proc);
	int master_thread_proc_grp = proc.Group;
	auto requested_parallelism = nodes.numaProcessors.at(master_thread_proc_grp);
    tbb::global_control global_limit(oneapi::tbb::global_control::max_allowed_parallelism, requested_parallelism);
    tbb::enumerable_thread_specific< std::pair<int, int> > tls;
	tbb::enumerable_thread_specific< double > tls_dummy;
	
    tbb::parallel_for(0, 1000, [&](int i)
        {
            // dummy work
            double& b = tls_dummy.local();
            for (int k = 0; k < 1000000; ++k)
                b += pow(i * k, 1. / 3.);

            PROCESSOR_NUMBER proc;
            if (GetThreadIdealProcessorEx(GetCurrentThread(), &proc))
            {
                tls.local() = std::pair<int, int>(proc.Group, proc.Number);
            }
        });
    for (const auto it : tls) {
		validateProcgrp[it.first]++;
	}
	REQUIRE(validateProcgrp[master_thread_proc_grp] == requested_parallelism);
}

//! Testing Numa Thread Distribution
//! \brief \ref number of processor in Numa node of master thread
TEST_CASE("Numa stability") {
    TestNumaDistribution();
}

#if _MSC_VER
#pragma warning (pop)
#endif
