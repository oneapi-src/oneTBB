#!/bin/bash
echo 'unset LD_PRELOAD'
unset LD_PRELOAD
echo 'g++ -o tbb_mem tbb_mem.cpp -ltbb'
g++ -o tbb_mem tbb_mem.cpp -ltbb
echo 'time ./tbb_mem '
time ./tbb_mem 
echo 'export LD_PRELOAD=$TBBROOT/lib/libtbbmalloc_proxy.so'
export LD_PRELOAD=$TBBROOT/lib/libtbbmalloc_proxy.so
echo 'time ./tbb_mem '
time ./tbb_mem 
echo 'unset LD_PRELOAD'
unset LD_PRELOAD
echo 'time ./tbb_mem '
time ./tbb_mem 
echo 'g++ -o tbb_mem tbb_mem.cpp -ltbb -ltbbmalloc_proxy'
g++ -o tbb_mem tbb_mem.cpp -ltbb -ltbbmalloc_proxy
echo 'time ./tbb_mem '
time ./tbb_mem 
