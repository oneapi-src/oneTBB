/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

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

#ifndef PRIMES_H_
#define PRIMES_H_

#if __TBB_MIC_OFFLOAD
#pragma offload_attribute (push,target(mic))
#endif // __TBB_MIC_OFFLOAD

#include "tbb/task_scheduler_init.h"
#include <cstddef>
typedef std::size_t NumberType;

//! Count number of primes between 0 and n
/** This is the serial version. */
NumberType SerialCountPrimes( NumberType n);

//! Count number of primes between 0 and n
/** This is the parallel version. */
NumberType ParallelCountPrimes( NumberType n, int numberOfThreads= tbb::task_scheduler_init::automatic, NumberType grainSize = 1000);

#if __TBB_MIC_OFFLOAD
#pragma offload_attribute (pop)
#endif // __TBB_MIC_OFFLOAD

#endif /* PRIMES_H_ */
