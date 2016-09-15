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

#include "harness.h"
#if __TBB_MIC_OFFLOAD
int TestMain () {
    return Harness::Skipped;
}
#else
#include "thread_monitor.h"
#include "harness_memory.h"
#include "tbb/semaphore.cpp"

class ThreadState {
    void loop();
public:
    static __RML_DECL_THREAD_ROUTINE routine( void* arg ) {
        static_cast<ThreadState*>(arg)->loop();
        return 0;
    }
    typedef rml::internal::thread_monitor thread_monitor;
    thread_monitor monitor;
    volatile int request;
    volatile int ack;
    volatile unsigned clock;
    volatile unsigned stamp;
    ThreadState() : request(-1), ack(-1), clock(0) {}
};

void ThreadState::loop() {
    for(;;) {
        ++clock;
        if( ack==request ) {
            thread_monitor::cookie c;
            monitor.prepare_wait(c);
            if( ack==request ) {
                REMARK("%p: request=%d ack=%d\n", this, request, ack );
                monitor.commit_wait(c);
            } else
                monitor.cancel_wait();
        } else {
            // Throw in delay occasionally
            switch( request%8 ) {
                case 0: 
                case 1:
                case 5:
                    rml::internal::thread_monitor::yield();
            }
            int r = request;
            ack = request;
            if( !r ) return;
        }
    }
}

// Linux on IA-64 architecture seems to require at least 1<<18 bytes per stack.
const size_t MinStackSize = 1<<18;
const size_t MaxStackSize = 1<<22;

int TestMain () {
    for( int p=MinThread; p<=MaxThread; ++p ) {
        ThreadState* t = new ThreadState[p];
        for( size_t stack_size = MinStackSize; stack_size<=MaxStackSize; stack_size*=2 ) {
            REMARK("launching %d threads\n",p);
            for( int i=0; i<p; ++i )
                rml::internal::thread_monitor::launch( ThreadState::routine, t+i, stack_size ); 
            for( int k=1000; k>=0; --k ) {
                if( k%8==0 ) {
                    // Wait for threads to wait.
                    for( int i=0; i<p; ++i ) {
                        unsigned count = 0;
                        do {
                            t[i].stamp = t[i].clock;
                            rml::internal::thread_monitor::yield();
                            if( ++count>=1000 ) {
                                REPORT("Warning: thread %d not waiting\n",i);
                                break;
                            }
                        } while( t[i].stamp!=t[i].clock );
                    }
                }
                REMARK("notifying threads\n");
                for( int i=0; i<p; ++i ) {
                    // Change state visible to launched thread
                    t[i].request = k;
                    t[i].monitor.notify();
                }
                REMARK("waiting for threads to respond\n");
                for( int i=0; i<p; ++i ) 
                    // Wait for thread to respond 
                    while( t[i].ack!=k ) 
                        rml::internal::thread_monitor::yield();
            }
        }
        delete[] t;
    }

    return Harness::Done;
}
#endif /* __TBB_MIC_OFFLOAD */
