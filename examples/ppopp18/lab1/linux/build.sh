#!/bin/bash
#
#    Copyright (c) 2018 Intel Corporation
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

if [ -z "$1" ]
	then
	echo "
Usage: ./build.sh <lab_step>
Example: ./build.sh 00

Possible values of <lab_step>:

	01	: parallel_for
	01s	: parallel_for_solution
	02	: reduction
	02s	: reduction_solution
	03	: container
	03s	: container_solution
	04	: flow_graph
	04s	: flow_graph_solution
	05	: task
	05s	: task_solution"
	exit
fi

# Setting Intel(R) Threading Building Blocks (Intel(R) TBB) environment
if [ -z "$TBBROOT" ]	# Intel(R) C++ Compiler 16.0 and above
then
. /opt/intel/compilers_and_libraries/linux/tbb/bin/tbbvars.sh intel64
fi
if [ -z "$TBBROOT" ] # # Older Intel(R) C++ Compiler version
then
. /opt/intel/composerxe/tbb/bin/tbbvars.sh intel64
fi

make clean
make $1


