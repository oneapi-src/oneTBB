# Copyright (c) 2017 Intel Corporation
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
#
#
#
#

# GNU Makefile that builds and runs example.
run_cmd=
PROG=stereo
ARGS=
PERF_RUN_ARGS=

TRACEFILE = $(shell date --iso=seconds)

# icpc by default
ifneq (,$(shell which icc 2>/dev/null))
CXX=icpc
endif

# autodetect C++11 support
include ../../common/examples-common.inc

# TBB libs
TBBLIB=-ltbb_preview
TBBLIB_DEBUG=-ltbb_preview_debug

FGT_FLAGS=-DTBB_PREVIEW_FLOW_GRAPH_TRACE -DTBB_PREVIEW_ALGORITHM_TRACE
DEMO_FLAGS=-DUSE_FLOW_GRAPH
DEMO_FLAGS+= -DSOLUTION

ifeq ($(shell uname), Linux)
ifeq ($(target), android)
LIBS+= --sysroot=$(SYSROOT)
run_cmd=../../common/android.linux.launcher.sh
else
LIBS+= -lrt
endif
else
ifeq ($(shell uname),Darwin)
override CXXFLAGS += -Wl,-rpath,$(TBBROOT)/lib
endif
endif

.PHONY: clean serial_build parallel_build test

all: parallel_build test

serial: serial_build test

parallel: parallel_build test

LIBS+= -lopencv_core -lopencv_imgcodecs -lopencv_highgui
LIBS+= -lopencv_imgproc

parallel_build: *.cpp
	$(CXX) -O2 -DNDEBUG $(CXXFLAGS) -o $(PROG) $^ $(TBBLIB) $(LIBS) $(CXX0XFLAGS) $(FGT_FLAGS) $(DEMO_FLAGS)

serial_build: *.cpp
	$(CXX) -O2 -DNDEBUG $(CXXFLAGS) -o $(PROG) $^ $(TBBLIB) $(LIBS) $(CXX0XFLAGS) $(FGT_FLAGS)

debug: *.cpp
	$(CXX) -O0 -g -DTBB_USE_DEBUG $(CXXFLAGS) -o $(PROG) $^ $(TBBLIB_DEBUG) $(LIBS) $(CXX0XFLAGS) $(FGT_FLAGS) $(DEMO_FLAGS)

clean:
	$(RM) $(PROG) *.o *.d _fgt.*

test:
	$(run_cmd) ./$(PROG) $(ARGS)

trace:
	fgt2xml.exe $(TRACEFILE)
	rm -rf ./_fgt.* 
	fga&

perf_build: release

perf_run:
	$(run_cmd) ./$(PROG) $(PERF_RUN_ARGS)

