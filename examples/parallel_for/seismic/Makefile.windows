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

# Just specify your program basename
PROG=Seismic
ARGS=auto 0
PERF_RUN_ARGS=auto 10000 silent
LIGHT_ARGS=1:2 100

# Trying to find if icl.exe is set
CXX1 = $(TBB_CXX)-
CXX2 = $(CXX1:icl.exe-=icl.exe)
CXX  = $(CXX2:-=cl.exe)

# Uncomment one of next lines to choose user interface type (console, gdiplus, direct draw)
#UI = con
UI = gdi
#UI = dd

# Machine architecture, auto-detected from TBB_TARGET_ARCH by default
# Use XARCH variable to change it. See index.html for more information
ARCH0 = $(TBB_TARGET_ARCH)-
ARCH1 = $(ARCH0:ia32-=x86)
ARCH2 = $(ARCH1:intel64-=AMD64)
XARCH = $(ARCH2:-=x86)

MAKEINC = ../../common/gui/Makefile.win
SOURCES = seismic_video.cpp universe.cpp main.cpp 
 
all: release test
release: compiler_check
	@$(MAKE) -f $(MAKEINC) UI=$(UI) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" LFLAGS="$(LDFLAGS) tbb.lib $(LIBS)" XARCH=$(XARCH) RCNAME=SeismicSimulation SOURCE="$(SOURCES)" EXE=$(PROG).exe build_one
debug: compiler_check
	@$(MAKE) -f $(MAKEINC) UI=$(UI) DEBUG=_debug CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS) /D TBB_USE_DEBUG" LFLAGS="$(LDFLAGS) tbb_debug.lib $(LIBS)" XARCH=$(XARCH) RCNAME=SeismicSimulation SOURCE="$(SOURCES)" EXE=$(PROG).exe build_one
clean:
	@cmd.exe /C del $(PROG).exe *.obj *.?db *.manifest msvs\SeismicSimulation.res >nul 2>&1
test:
	$(PROG) $(ARGS) 

perf_build: compiler_check
	@$(MAKE) -f $(MAKEINC) UI=con CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" LFLAGS="$(LDFLAGS) tbb.lib $(LIBS)" XARCH=$(XARCH) RCNAME=SeismicSimulation SOURCE="$(SOURCES) " EXE=$(PROG).exe build_one
perf_run:
	$(PROG) $(PERF_RUN_ARGS)

light_test:
	$(PROG) $(LIGHT_ARGS)

compiler_check:
	@echo compiler_test>compiler_test && @$(CXX) /E compiler_test >nul 2>&1  || echo "$(CXX) command not found. Check if CXX=$(CXX) is set properly"
	@cmd.exe /C del compiler_test
