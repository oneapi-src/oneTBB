# Copyright 2005-2016 Intel Corporation.  All Rights Reserved.
#
# This file is part of Threading Building Blocks. Threading Building Blocks is free software;
# you can redistribute it and/or modify it under the terms of the GNU General Public License
# version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
# distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See  the GNU General Public License for more details.   You should have received a copy of
# the  GNU General Public License along with Threading Building Blocks; if not, write to the
# Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA
#
# As a special exception,  you may use this file  as part of a free software library without
# restriction.  Specifically,  if other files instantiate templates  or use macros or inline
# functions from this file, or you compile this file and link it with other files to produce
# an executable,  this file does not by itself cause the resulting executable to be covered
# by the GNU General Public License. This exception does not however invalidate any other
# reasons why the executable file might be covered by the GNU General Public License.

# GNU Makefile that builds and runs example.

run_cmd=
PROG=fgbzip2
ARGS=-b=9 -a $(PROG)
PERF_RUN_ARGS=-b=9 -a $(PROG)

# The C++ compiler
ifneq (,$(shell which icc 2>/dev/null))
CXX=icpc
endif # which icc

ifeq ($(shell uname), Linux)
LIBS+= -lrt -lpthread
else ifeq ($(shell uname), Darwin)
override CXXFLAGS += -Wl,-rpath,$(TBBROOT)/lib
endif

CXXFLAGS += -std=c++0x -DTBB_PREVIEW_FLOW_GRAPH_NODES=1 $(CXX0XFLAGS)

all: release test

release:
	$(CXX) -O2 -DNDEBUG *.cpp $(CXXFLAGS) -o $(PROG) -ltbb $(LIBS)

debug:
	$(CXX) -O0 -g -DTBB_USE_DEBUG *.cpp $(CXXFLAGS) -o $(PROG) -ltbb_debug $(LIBS)

clean:
	$(RM) $(PROG) *.o *.d

test:
	$(run_cmd) ./$(PROG) $(ARGS)

perf_build: release

perf_run:
	$(run_cmd) ./$(PROG) $(PERF_RUN_ARGS)
