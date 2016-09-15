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

#define VIDEO_WINMAIN_ARGS

#include <iostream>
#include "tbb/tick_count.h"
#include "../../common/utility/utility.h"

#include "seismic_video.h"
#include "universe.h"
#include "tbb/task_scheduler_init.h"

Universe u;

struct RunOptions {
    //! It is used for console mode for test with different number of threads and also has
    //! meaning for GUI: threads.first  - use separate event/updating loop thread (>0) or not (0).
    //!                  threads.second - initialization value for scheduler
    utility::thread_number_range threads;
    int numberOfFrames;
    bool silent;
    bool parallel;
    RunOptions(utility::thread_number_range threads_ ,    int number_of_frames_ ,     bool silent_ , bool parallel_ )
        : threads(threads_),numberOfFrames(number_of_frames_), silent(silent_), parallel(parallel_)
    {
    }
};

int do_get_default_num_threads() {
    int threads;
#if __TBB_MIC_OFFLOAD
    #pragma offload target(mic) out(threads)
#endif // __TBB_MIC_OFFLOAD
    threads = tbb::task_scheduler_init::default_num_threads();
    return threads;
}

int get_default_num_threads() {
    static int threads = do_get_default_num_threads();
    return threads;
}

RunOptions ParseCommandLine(int argc, char *argv[]){
    // zero number of threads means to run serial version
    utility::thread_number_range threads(get_default_num_threads,0,get_default_num_threads());

    int numberOfFrames = 0;
    bool silent = false;
    bool serial = false;

    utility::parse_cli_arguments(argc,argv,
        utility::cli_argument_pack()
            //"-h" option for displaying help is present implicitly
            .positional_arg(threads,"n-of-threads",utility::thread_number_range_desc)
            .positional_arg(numberOfFrames,"n-of-frames","number of frames the example processes internally (0 means unlimited)")
            .arg(silent,"silent","no output except elapsed time")
            .arg(serial,"serial","in GUI mode start with serial version of algorithm")
    );
    return RunOptions(threads,numberOfFrames,silent,!serial);
}

int main(int argc, char *argv[])
{
    try{
        tbb::tick_count mainStartTime = tbb::tick_count::now();
        RunOptions options = ParseCommandLine(argc,argv);
        SeismicVideo video(u,options.numberOfFrames,options.threads.last,options.parallel);

        // video layer init
        if(video.init_window(u.UniverseWidth, u.UniverseHeight)) {
            video.calc_fps = true;
            video.threaded = options.threads.first > 0;
            // video is ok, init Universe
            u.InitializeUniverse(video);
            // main loop
            video.main_loop();
        }
        else if(video.init_console()) {
            // do console mode
            if (options.numberOfFrames == 0) {
                options.numberOfFrames = 1000;
                std::cout << "Substituting 1000 for unlimited frames because not running interactively\n";
            }
            for(int p = options.threads.first;  p <= options.threads.last; p = options.threads.step(p)) {
                tbb::tick_count xwayParallelismStartTime = tbb::tick_count::now();
                u.InitializeUniverse(video);
                int numberOfFrames = options.numberOfFrames;
#if __TBB_MIC_OFFLOAD
                drawing_memory dmem = video.get_drawing_memory();
                char *pMem = dmem.get_address();
                size_t memSize = dmem.get_size();

                #pragma offload target(mic) in(u, numberOfFrames, p, dmem), out(pMem:length(memSize))
                {
                    // It is necessary to update the pointer on mic 
                    // since the address spaces on host and on target are different
                    dmem.set_address(pMem);
                    u.SetDrawingMemory(dmem);
#endif // __TBB_MIC_OFFLOAD
                    if (p==0) {
                        //run a serial version
                        for( int i=0; i<numberOfFrames; ++i ) {
                            u.SerialUpdateUniverse();
                        }
                    } else {
                        tbb::task_scheduler_init init(p);
                        for( int i=0; i<numberOfFrames; ++i ) {
                            u.ParallelUpdateUniverse();
                        }
                    }
#if __TBB_MIC_OFFLOAD
                }
#endif // __TBB_MIC_OFFLOAD

                if (!options.silent){
                    double fps =  options.numberOfFrames/((tbb::tick_count::now()-xwayParallelismStartTime).seconds());
                    std::cout<<fps<<" frame per sec with ";
                    if (p==0){
                        std::cout<<"serial code\n";
                    }else{
                        std::cout<<p<<" way parallelism\n";
                    }
                }
            }
        }
        video.terminate();
        utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());
        return 0;
    }catch(std::exception& e){
        std::cerr<<"error occurred. error text is :\"" <<e.what()<<"\"\n";
        return 1;
    }
}
