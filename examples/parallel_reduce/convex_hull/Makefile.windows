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

# Common Makefile that builds and runs example.
PROG=convex_hull_bench
ARGS=
PERF_RUN_ARGS = silent auto 40000000
LIGHT_ARGS = 4 400

# Trying to find if icl.exe is set
CXX1 = $(TBB_CXX)-
CXX2 = $(CXX1:icl.exe-=icl.exe)
CXX  = $(CXX2:-=cl.exe)

# The C++ compiler options
# Below, _SECURE_SCL=0 is for VC9 and earlier, and _ITERATOR_DEBUG_LEVEL=0 is for VC10 and later
MYCXXFLAGS = /TP /EHsc /W3 /nologo /D _CONSOLE /D _MBCS /D WIN32 /D _ITERATOR_DEBUG_LEVEL=0 /D _SECURE_SCL=0 $(CXXFLAGS)
MYLDFLAGS =/INCREMENTAL:NO /NOLOGO /DEBUG /FIXED:NO $(LDFLAGS)

all: release test
release: compiler_check
	$(CXX) convex_hull_sample.cpp /MD /O2 /D NDEBUG $(MYCXXFLAGS) /link tbb.lib $(LIBS) $(MYLDFLAGS) /OUT:convex_hull_sample.exe
	$(CXX) convex_hull_bench.cpp /MD /O2 /D NDEBUG $(MYCXXFLAGS) /link tbb.lib $(LIBS) $(MYLDFLAGS) /OUT:convex_hull_bench.exe
debug: compiler_check
	$(CXX) convex_hull_sample.cpp /MDd /Od /Zi /D TBB_USE_DEBUG /D _DEBUG $(MYCXXFLAGS) /link tbb_debug.lib $(LIBS) $(MYLDFLAGS) /OUT:convex_hull_sample.exe
	$(CXX) convex_hull_bench.cpp /MDd /Od /Zi /D TBB_USE_DEBUG /D _DEBUG $(MYCXXFLAGS) /link tbb_debug.lib $(LIBS) $(MYLDFLAGS) /OUT:convex_hull_bench.exe
clean:
	@cmd.exe /C del convex_hull*.exe *.obj *.?db *.manifest
test:
	$(PROG) $(ARGS)
perf_build: release

perf_run:
	./convex_hull_sample $(PERF_RUN_ARGS)

light_test:
	./$(PROG) $(LIGHT_ARGS)

compiler_check:
	@echo compiler_test>compiler_test && @$(CXX) /E compiler_test >nul 2>&1  || echo "$(CXX) command not found. Check if CXX=$(CXX) is set properly"
	@cmd.exe /C del compiler_test
