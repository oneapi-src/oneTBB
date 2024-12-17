#!/bin/bash

# Copyright (c) 2024 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
