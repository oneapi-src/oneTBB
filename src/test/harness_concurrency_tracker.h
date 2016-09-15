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

#ifndef tbb_tests_harness_concurrency_tracker_H
#define tbb_tests_harness_concurrency_tracker_H

#include "harness_assert.h"
#include "harness_barrier.h"
#include "tbb/atomic.h"
#include "../tbb/tls.h"
// Note: This file is used by RML tests which do not link TBB
// Thus only header-only TBB features can be included

namespace Harness {

static tbb::atomic<unsigned> ctInstantParallelism;
static tbb::atomic<unsigned> ctPeakParallelism;
static tbb::internal::tls<uintptr_t>  ctNested;

class ConcurrencyTracker {
    bool    m_Outer;

    static void Started () {
        unsigned p = ++ctInstantParallelism;
        unsigned q = ctPeakParallelism;
        while( q<p ) {
            q = ctPeakParallelism.compare_and_swap(p,q);
        }
    }

    static void Stopped () {
        ASSERT ( ctInstantParallelism > 0, "Mismatched call to ConcurrencyTracker::Stopped()" );
        --ctInstantParallelism;
    }
public:
    ConcurrencyTracker() : m_Outer(false) {
        uintptr_t nested = ctNested;
        ASSERT (nested == 0 || nested == 1, NULL);
        if ( !ctNested ) {
            Started();
            m_Outer = true;
            ctNested = 1;
        }
    }
    ~ConcurrencyTracker() {
        if ( m_Outer ) {
            Stopped();
            ctNested = 0;
        }
    }

    static unsigned PeakParallelism() { return ctPeakParallelism; }
    static unsigned InstantParallelism() { return ctInstantParallelism; }

    static void Reset() {
        ASSERT (ctInstantParallelism == 0, "Reset cannot be called when concurrency tracking is underway");
        ctInstantParallelism = ctPeakParallelism = 0;
    }
}; // ConcurrencyTracker

} // namespace Harness

#endif /* tbb_tests_harness_concurrency_tracker_H */
