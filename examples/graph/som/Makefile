# Copyright 2005-2015 Intel Corporation.  All Rights Reserved.
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
NAME=som
EXE=$(NAME).exe
ARGS=
LIGHT_ARGS=4

# The C++ compiler
ifneq (,$(shell which icc 2>/dev/null))
CXX=icc
endif # icc

override CXXFLAGS += $(UI_CXXFLAGS)

SRCFILES = som_graph.cpp som.cpp

ifeq ($(shell uname), Linux)
LIBS+= -lrt 
endif

all:	release test

release: $(SRCFILES)
ifeq ($(compiler),xl)
	# Avoiding "1586-346 (U) An error occurred during code generation.  The code generation return code was 40." with -O3.
	$(CXX) -O2 -DNDEBUG $(CXXFLAGS) -o $(EXE) $(SRCFILES) -ltbb -ltbbmalloc $(LIBS)
else
	$(CXX) -O3 -DNDEBUG $(CXXFLAGS) -o $(EXE) $(SRCFILES) -ltbb -ltbbmalloc $(LIBS)
endif

debug:  $(SRCFILES)
	$(CXX) -g -O0 -DTBB_USE_DEBUG -D_DEBUG $(CXXFLAGS) -o $(EXE) $(SRCFILES) -ltbb_debug -ltbbmalloc_debug $(LIBS)

clean:
	$(RM) $(EXE) *.o *.d

test:
	$(run_cmd) ./$(EXE) $(ARGS)

light_test:
	$(run_cmd) ./$(EXE) $(LIGHT_ARGS)
