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

// Thread level recorder
#ifndef __THREAD_LEVEL_H
#define __THREAD_LEVEL_H
#include <cstdio>
#include <omp.h>
#include <assert.h>
#include "tbb/atomic.h"
#include "tbb/tick_count.h"

//#define LOG_THREADS // use this to ifdef out calls to this class 
//#define NO_BAIL_OUT // continue execution after detecting oversubscription

using namespace tbb;

typedef enum {tbb_outer, tbb_inner, omp_outer, omp_inner} client_t;

class ThreadLevelRecorder {
  tbb::atomic<int> tbb_outer_level;
  tbb::atomic<int> tbb_inner_level;
  tbb::atomic<int> omp_outer_level;
  tbb::atomic<int> omp_inner_level;
  struct record {
    tbb::tick_count time;
    int n_tbb_outer_thread;
    int n_tbb_inner_thread;
    int n_omp_outer_thread;
    int n_omp_inner_thread;
  };
  tbb::atomic<unsigned> next;
  /** Must be power of two */
  static const unsigned max_record_count = 1<<20;
  record array[max_record_count];
  int max_threads;
  bool fail;
 public:
  void change_level(int delta, client_t whichClient);
  void dump();
  void init();
};

void ThreadLevelRecorder::change_level(int delta, client_t whichClient) {
  int tox=tbb_outer_level, tix=tbb_inner_level, oox=omp_outer_level, oix=omp_inner_level;
  if (whichClient == tbb_outer) {
    tox = tbb_outer_level+=delta;
  } else if (whichClient == tbb_inner) {
    tix = tbb_inner_level+=delta;
  } else if (whichClient == omp_outer) {
    oox = omp_outer_level+=delta;
  } else if (whichClient == omp_inner) {
    oix = omp_inner_level+=delta;
  } else {
    printf("WARNING: Bad client type; ignoring.\n");
    return;
  }
  // log non-negative entries
  tbb::tick_count t = tbb::tick_count::now();
  unsigned k = next++;
  if (k<max_record_count) {
    record& r = array[k];
    r.time = t;
    r.n_tbb_outer_thread = tox>=0?tox:0;
    r.n_omp_outer_thread = oox>=0?oox:0;
    r.n_tbb_inner_thread = tix>=0?tix:0;
    r.n_omp_inner_thread = oix>=0?oix:0;
  }
  char errStr[100];
  int tot_threads;
  tot_threads = tox+tix+oox+oix;
  sprintf(errStr, "ERROR: Number of threads (%d+%d+%d+%d=%d) in use exceeds maximum (%d).\n", 
	  tox, tix, oox, oix, tot_threads, max_threads);
  if (tot_threads > max_threads) {
#ifdef NO_BAIL_OUT
    if (!fail) {
      printf("%sContinuing...\n", errStr);
      fail = true;
    }
#else
    dump();
    printf("%s\n", errStr);
    assert(tot_threads <= max_threads);
#endif
  }
}

void ThreadLevelRecorder::dump() {
  FILE* f = fopen("time.txt","w");
  if (!f) {
    perror("fopen(time.txt)\n");
    exit(1);
  }
  unsigned limit = next;
  if (limit>max_record_count) { // Clip
    limit = max_record_count;
  }
  for (unsigned i=0; i<limit; ++i) {
    fprintf(f,"%f\t%d\t%d\t%d\t%d\n",(array[i].time-array[0].time).seconds(), array[i].n_tbb_outer_thread,
	    array[i].n_tbb_inner_thread, array[i].n_omp_outer_thread, array[i].n_omp_inner_thread);
  }
  fclose(f);
  int tox=tbb_outer_level, tix=tbb_inner_level, oox=omp_outer_level, oix=omp_inner_level;
  int tot_threads;
  tot_threads = tox+tix+oox+oix;
  if (!fail) printf("INFO: Passed.\n");
  else printf("INFO: Failed.\n");
}

void ThreadLevelRecorder::init() {
  fail = false;
  max_threads = omp_get_max_threads();
  printf("INFO: Getting maximum hardware threads... %d.\n", max_threads);
}

ThreadLevelRecorder TotalThreadLevel;
#endif
