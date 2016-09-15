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

#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <float.h>
#include <math.h>
#include <time.h>

#include <omp.h>
#include <assert.h>

#include "thread_level.h"

#include "tbb/task.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/scalable_allocator.h"

#if _WIN32||_WIN64
#include <Windows.h> /* Need Sleep */
#else
#include <unistd.h>  /* Need usleep */
#endif

void MilliSleep( unsigned milliseconds ) {
#if _WIN32||_WIN64
    Sleep( milliseconds );
#else
    usleep( milliseconds*1000 );
#endif /* _WIN32||_WIN64 */
}

using namespace std;
using namespace tbb;

// Algorithm parameters
const int Max_TBB_Threads = 16;
const int Max_OMP_Threads = 16;

// Global variables
int max_tbb_threads = Max_TBB_Threads;
int max_omp_threads = Max_OMP_Threads;

// Print help on command-line arguments
void help_message(char *prog_name) {
  fprintf(stderr, "\n%s usage:\n", prog_name);
  fprintf(stderr, 
	  "  Parameters:\n"
	  "    -t<num> : max # of threads TBB should use\n"
	  "    -o<num> : max # of threads OMP should use\n"
	  "\n  Help:\n"
	  "    -h : print this help message\n");
}

// Process command-line arguments
void process_args(int argc, char *argv[], int *max_tbb_t, int *max_omp_t) {
  for (int i=1; i<argc; ++i) {  
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
      case 't': // set max_tbb_threads
	if (sscanf(&argv[i][2], "%d", max_tbb_t) != 1 || *max_tbb_t < 1) {
	  fprintf(stderr, "%s Warning: argument of -t option unacceptable: %s\n", argv[0], &argv[i][2]);
	  help_message(argv[0]);
	}
	break;
      case 'o': // set max_omp_threads
	if (sscanf(&argv[i][2], "%d", max_omp_t) != 1 || *max_omp_t < 1) {
	  fprintf(stderr, "%s Warning: argument of -o option unacceptable: %s\n", argv[0], &argv[i][2]);
	  help_message(argv[0]);
	}
	break;
      case 'h': // print help message
	help_message(argv[0]);
	exit(0);
	break;
      default:
	fprintf(stderr, "%s: Warning: command-line option ignored: %s\n", argv[0], argv[i]);
	help_message(argv[0]);
	break;
      }
    } else {
      fprintf(stderr, "%s: Warning: command-line option ignored: %s\n", argv[0], argv[i]);
      help_message(argv[0]);
    }
  }
}

class SimpleTask : public task {
  bool isLeaf;
  int myId;
public:
  SimpleTask(bool isLeaf_, int myId_) : isLeaf(isLeaf_), myId(myId_) {}
  task* execute() {
#ifdef LOG_THREADS
    TotalThreadLevel.change_level(1, tbb_outer);
#endif
    omp_set_num_threads(max_omp_threads);
    if (!isLeaf) {
      set_ref_count(65);
      for (int i=0; i<64; ++i) {
	SimpleTask& st = *new(allocate_child()) SimpleTask(true, i);
	spawn(st);
      }
#ifdef LOG_THREADS
      TotalThreadLevel.change_level(-1, tbb_outer);
#endif
      wait_for_all();
#ifdef LOG_THREADS
      TotalThreadLevel.change_level(1, tbb_outer);
#endif
    }
    else {
      if (myId%2 == 0) {
	MilliSleep(3000);
#pragma omp parallel
	{
#ifdef LOG_THREADS
	  if (omp_get_thread_num() == 0)
	    TotalThreadLevel.change_level(omp_get_num_threads()-1, omp_inner);
#endif
	  //printf("In OMP parallel region on TBB task with myId=0: thread %d of %d\n", omp_get_thread_num(), omp_get_num_threads());
#ifdef LOG_THREADS
	  if (omp_get_thread_num() == 0)
	    TotalThreadLevel.change_level(-(omp_get_num_threads()-1), omp_inner);
#endif
	}
      }
      else {
	MilliSleep(6000);
      }
    }
#ifdef LOG_THREADS
    TotalThreadLevel.change_level(-1, tbb_outer);
#endif
    return NULL;
  }
};


int main(int argc, char *argv[]) { 
#ifdef LOG_THREADS
  TotalThreadLevel.init();
  TotalThreadLevel.change_level(1, tbb_outer);
#endif
  process_args(argc, argv, &max_tbb_threads, &max_omp_threads);

  task_scheduler_init phase(max_tbb_threads);
  tick_count start, end;
  start = tick_count::now();
  SimpleTask& st = *new(task::allocate_root()) SimpleTask(false, -1);
#ifdef LOG_THREADS
  TotalThreadLevel.change_level(-1, tbb_outer);
#endif
  task::spawn_root_and_wait(st);
#ifdef LOG_THREADS
  TotalThreadLevel.change_level(1, tbb_outer);
#endif
  end = tick_count::now();
  printf("Simple Test of TBB (%d threads max) with OMP (%d threads max) inside took: %6.6f\n", 
	 max_tbb_threads, max_omp_threads, (end-start).seconds());

#ifdef LOG_THREADS
  TotalThreadLevel.change_level(-1, tbb_outer);
  TotalThreadLevel.dump();
#endif
  return 0;
}
