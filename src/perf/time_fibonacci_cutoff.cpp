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

#include <cstdio>
#include <cstdlib>

#include "tbb/task_scheduler_init.h"
#include "tbb/task.h"
#include "tbb/tick_count.h"
#include "fibonacci_impl_tbb.cpp"

long CutOff = 1;

long SerialFib( const long n );

long ParallelFib( const long n ); 

inline void dump_title() {
    printf("    Mode,   P, repeat,  N, =fib value, cutoff,     time, speedup\n");
}

inline void output(int P, long n, long c, int T, double serial_elapsed, double elapsed, long result) {
    printf("%s,%4d,%7d,%3ld,%11ld,%7ld,%9.3g,%8.3g\n", ( (P == 0) ? "  Serial" : "Parallel" ),
                 P, T, n, result, c, elapsed, serial_elapsed / elapsed);
}

#define MOVE_BY_FOURTHS 1
inline long calculate_new_cutoff(const long lo, const long hi) {
#if MOVE_BY_FOURTHS    
    return lo + (3 + hi - lo ) / 4;
#else
    return (hi + lo)/2;
#endif
}

void find_cutoff(const int P, const long n, const int T, const double serial_elapsed) {
    long lo = 1, hi = n;
    double elapsed = 0, lo_elapsed = 0, hi_elapsed = 0;
    long final_cutoff = -1;

    tbb::task_scheduler_init init(P);

    while(true) {
       CutOff = calculate_new_cutoff(lo, hi);
       long result = 0;
       tbb::tick_count t0;
       for (int t = -1; t < T; ++t) {
           if (t == 0) t0 = tbb::tick_count::now();
           result += ParallelFib(n);
       }
       elapsed = (tbb::tick_count::now() - t0).seconds();
       output(P,n,CutOff,T,serial_elapsed,elapsed,result);

       if (serial_elapsed / elapsed >= P/2.0) {
           final_cutoff = CutOff;
           if (hi == CutOff) {
               if (hi == lo) {
                  // we have had this value at both above and below 50%
                  lo = 1; lo_elapsed = 0;
               } else  {
                  break;
               }
           }
           hi = CutOff;
           hi_elapsed = elapsed;
       } else {
           if (lo == CutOff) break;
           lo = CutOff;
           lo_elapsed = elapsed;
       }
    } 

    double interpolated_cutoff = lo + ( P/2.0 - serial_elapsed/lo_elapsed ) * ( (hi - lo) / ( serial_elapsed/hi_elapsed - serial_elapsed/lo_elapsed ));

    if (final_cutoff != -1) {
        printf("50%% efficiency cutoff is %ld ( linearly interpolated cutoff is %g )\n", final_cutoff, interpolated_cutoff);
    } else {
        printf("Cannot achieve 50%% efficiency\n");
    }

    return;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s threads n repetitions\nWhere n make sense in range [25; 45]\n",argv[0]); 
        return 1;
    }

    int P = atoi(argv[1]);
    volatile long n = atol(argv[2]);
    int T = atoi(argv[3]);

    // warmup parallel engine
    ParallelFib(n);

    dump_title();

    // collect serial time
    long serial_result = 0;
    tbb::tick_count t0; 
    for (int t = -1; t < T; ++t) {
        if (t == 0) t0 = tbb::tick_count::now();        
        serial_result += SerialFib(n);
    }
    double serial_elapsed = (tbb::tick_count::now() - t0).seconds();
    output(0,n,0,T,serial_elapsed,serial_elapsed,serial_result);

    // perform search
    find_cutoff(P,n,T,serial_elapsed);

    return 0;
}
