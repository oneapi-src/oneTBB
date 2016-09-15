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

#if __APPLE__

#define HARNESS_CUSTOM_MAIN 1
#include "harness.h"
#include <cstdlib>
#include "tbb/task_scheduler_init.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

bool exec_test(const char *self) {
    int status = 1;
    pid_t p = fork();
    if(p < 0) {
        REPORT("fork error: errno=%d: %s\n", errno, strerror(errno));
        return true;
    }
    else if(p) { // parent
        if(waitpid(p, &status, 0) != p) {
            REPORT("wait error: errno=%d: %s\n", errno, strerror(errno));
            return true;
        }
        if(WIFEXITED(status)) {
            if(!WEXITSTATUS(status)) return false; // ok
            else REPORT("child has exited with return code 0x%x\n", WEXITSTATUS(status));
        } else {
            REPORT("child error 0x%x:%s%s ", status, WIFSIGNALED(status)?" signalled":"",
                WIFSTOPPED(status)?" stopped":"");
            if(WIFSIGNALED(status))
                REPORT("%s%s", sys_siglist[WTERMSIG(status)], WCOREDUMP(status)?" core dumped":"");
            if(WIFSTOPPED(status))
                REPORT("with %d stop-code", WSTOPSIG(status));
            REPORT("\n");
        }
    }
    else { // child
        // reproduces error much often
        execl(self, self, "0", NULL);
        REPORT("exec fails %s: %d: %s\n", self, errno, strerror(errno));
        exit(2);
    }
    return true;
}

HARNESS_EXPORT
int main( int argc, char * argv[] ) {
    MinThread = 3000;
    ParseCommandLine( argc, argv );
    if( MinThread <= 0 ) {
        tbb::task_scheduler_init init( 2 ); // even number required for an error
    } else {
        for(int i = 0; i<MinThread; i++) {
            if(exec_test(argv[0])) {
                REPORT("ERROR: execution fails at %d-th iteration!\n", i);
                exit(1);
            }
        }
        REPORT("done\n");
    }
}

#else /* !__APPLE__ */

#define HARNESS_NO_PARSE_COMMAND_LINE 1
#include "harness.h"

int TestMain () {
    return Harness::Skipped;
}

#endif /* !__APPLE__ */
