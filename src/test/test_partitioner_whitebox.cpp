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

#include "harness_assert.h"
#include "test_partitioner_whitebox.h"

using uniform_iterations_distribution::ParallelTestBody;

class ParallelBody: public ParallelTestBody {
public:
    ParallelBody(size_t parallel_group_thread_starting_index)
        : ParallelTestBody(parallel_group_thread_starting_index) { }

    void operator()(size_t relative_thread_index) const {
        use_case_settings_t settings = {
            m_parallel_group_thread_starting_index + relative_thread_index, // thread_num
            0,                                                              // factors_array_len
            0,                                                              // range_begin
            false,                                                          // provide_feedback (disabled)
            true,                                                           // ensure_non_empty_size
            0,                                                              // above_threads_size_tolerance
            0,                                                              // below_threads_size_tolerance
            0,                                                              // between_min_max_ranges_tolerance
            &ParallelTestBody::uniform_distribution_checker
        };
        g_threadNums.local() = settings.thread_num;
        using namespace test_partitioner_utils::TestRanges;
        {
            size_t factors[] = { 1, 2, 3, 4, 5, 7, 9, 13, 27, 29, 30, 31, 32 };
            settings.factors_array_len = sizeof(factors) / sizeof(factors[0]);

            settings.between_min_max_ranges_tolerance = 0; // it should be equal to zero for blocked_range
            test<BlockedRange>(settings, factors);

            settings.checker = &ParallelTestBody::nonuniform_distribution_checker;
            test<InvertedProportionRange>(settings, factors);
            test<RoundedDownRange>(settings, factors);
            test<RoundedUpRange>(settings, factors);

            test<Range1_2>(settings, factors);
            test<Range1_999>(settings, factors);
            test<Range999_1>(settings, factors);
        }

        {
            // iterations might not be distributed uniformly
            float factors[] = { 1.2f, 2.5f, 3.7f, 4.2f, 5.1f, 8.9f, 27.8f };
            settings.factors_array_len = sizeof(factors) / sizeof(factors[0]);

            settings.between_min_max_ranges_tolerance = 1; // it should be equal to one for blocked_range
            settings.checker = &ParallelTestBody::uniform_distribution_checker;
            test<BlockedRange>(settings, factors);

            settings.checker = &ParallelTestBody::nonuniform_distribution_checker;
            test<InvertedProportionRange>(settings, factors);
            test<RoundedDownRange>(settings, factors);
            test<RoundedUpRange>(settings, factors);

            test<Range1_2>(settings, factors);
            test<Range1_999>(settings, factors);
            test<Range999_1>(settings, factors);
        }

        {
            // iterations might not be distributed uniformly
            size_t factors[] = { 1, 2, 3, 4, 5, 7, 9, 11, 13, 27, 29, 30, 31, 32 };
            settings.factors_array_len = sizeof(factors) / sizeof(factors[0]);

            settings.checker = &ParallelTestBody::uniform_distribution_checker;
            test<BlockedRange>(settings, factors, &shifted_left_range_size_generator);
            test<BlockedRange>(settings, factors, &shifted_right_range_size_generator);

            settings.checker = &ParallelTestBody::nonuniform_distribution_checker;
            test<InvertedProportionRange>(settings, factors, &shifted_left_range_size_generator);
            test<InvertedProportionRange>(settings, factors, &shifted_right_range_size_generator);

            test<RoundedDownRange>(settings, factors, &shifted_left_range_size_generator);
            test<RoundedDownRange>(settings, factors, &shifted_right_range_size_generator);

            test<RoundedUpRange>(settings, factors, &shifted_left_range_size_generator);
            test<RoundedUpRange>(settings, factors, &shifted_right_range_size_generator);

            test<Range1_2>(settings, factors, &shifted_left_range_size_generator);
            test<Range1_2>(settings, factors, &shifted_right_range_size_generator);

            test<Range1_999>(settings, factors, &shifted_left_range_size_generator);
            test<Range1_999>(settings, factors, &shifted_right_range_size_generator);

            test<Range999_1>(settings, factors, &shifted_left_range_size_generator);
            test<Range999_1>(settings, factors, &shifted_right_range_size_generator);
        }

        {
            settings.factors_array_len = 1;
            settings.between_min_max_ranges_tolerance = 1; // since range iterations are not divided without remainder
            settings.checker = &ParallelTestBody::uniform_distribution_checker;
            test<ExactSplitRange, size_t>(settings, NULL, &max_range_size_generator);
            settings.range_begin = size_t(-1) - 10000;
            test<ExactSplitRange, size_t>(settings, NULL, &max_range_size_generator);
        }

        {
            settings.range_begin = 0;
            settings.factors_array_len = 2 * unsigned(settings.thread_num);
            settings.checker = &ParallelTestBody::nonuniform_distribution_checker;

            test<RoundedUpRange, size_t>(settings, NULL, &simple_size_generator);
            test<RoundedDownRange, size_t>(settings, NULL, &simple_size_generator);

            test<InvertedProportionRange, size_t>(settings, NULL, &simple_size_generator);
            test<Range1_2, size_t>(settings, NULL, &simple_size_generator);
            test<Range1_999, size_t>(settings, NULL, &simple_size_generator);
            test<Range999_1, size_t>(settings, NULL, &simple_size_generator);

            settings.ensure_non_empty_size = false;
            test<RoundedUpRange, size_t>(settings, NULL, &simple_size_generator);
            test<RoundedDownRange, size_t>(settings, NULL, &simple_size_generator);

            test<InvertedProportionRange, size_t>(settings, NULL, &simple_size_generator);
            test<Range1_2, size_t>(settings, NULL, &simple_size_generator);
            test<Range1_999, size_t>(settings, NULL, &simple_size_generator);
            test<Range999_1, size_t>(settings, NULL, &simple_size_generator);

        }
    }
};

int TestMain() {
    uniform_iterations_distribution::test<ParallelBody>();
    return Harness::Done;
}
