# Copyright (c) 2005-2016 Intel Corporation
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

# Common Makefile that builds and runs examples.
PROG=sub_string_finder_extended
ARGS=
LIGHT_PROG=sub_string_finder

# The C++ compiler options
# Trying to find if icl.exe is set
CXX1 = $(TBB_CXX)-
CXX2 = $(CXX1:icl.exe-=icl.exe)
CXX  = $(CXX2:-=cl.exe)

MYCXXFLAGS = /TP /EHsc /W3 /nologo /D _CONSOLE /D _MBCS /D WIN32 $(offload:mic=/D__TBB_MIC_OFFLOAD /Qdiag-disable:3335,3440,3441) $(CXXFLAGS)
MYLDFLAGS =/INCREMENTAL:NO /NOLOGO /DEBUG /FIXED:NO $(LDFLAGS)

TBB_MIC_LIB = $(offload:mic=/Qtbb)
TBB_DEBUG_MIC_LIB = $(offload:mic=/Qoffload-option,mic,link,"-ltbb_debug")

all: release test
release: compiler_check
	$(CXX) sub_string_finder.cpp /MD /O2 /D NDEBUG $(MYCXXFLAGS) $(TBB_MIC_LIB) /link tbb.lib $(LIBS) $(MYLDFLAGS) /OUT:sub_string_finder.exe
	$(CXX) sub_string_finder_extended.cpp /MD /O2 /D NDEBUG $(MYCXXFLAGS) $(TBB_MIC_LIB) /link tbb.lib $(LIBS) $(MYLDFLAGS) /OUT:sub_string_finder_extended.exe
	$(CXX) sub_string_finder_pretty.cpp /MD /O2 /D NDEBUG $(MYCXXFLAGS) $(TBB_MIC_LIB) /link tbb.lib $(LIBS) $(MYLDFLAGS) /OUT:sub_string_finder_pretty.exe
debug: compiler_check
	$(CXX) sub_string_finder.cpp /MDd /Od /Zi /D TBB_USE_DEBUG /D _DEBUG $(MYCXXFLAGS) $(TBB_DEBUG_MIC_LIB) /link tbb_debug.lib $(LIBS) $(MYLDFLAGS) /OUT:sub_string_finder.exe
	$(CXX) sub_string_finder_extended.cpp /MDd /Od /Zi /D TBB_USE_DEBUG /D _DEBUG $(MYCXXFLAGS) $(TBB_DEBUG_MIC_LIB) /link tbb_debug.lib $(LIBS) $(MYLDFLAGS) /OUT:sub_string_finder_extended.exe
	$(CXX) sub_string_finder_pretty.cpp /MDd /Od /Zi /D TBB_USE_DEBUG /D _DEBUG $(MYCXXFLAGS) $(TBB_DEBUG_MIC_LIB) /link tbb_debug.lib $(LIBS) $(MYLDFLAGS) /OUT:sub_string_finder_pretty.exe
clean:
	@cmd.exe /C del sub_string_finder*.exe *.obj *.?db *.manifest
test:
	$(PROG) $(ARGS)
light_test:
	$(LIGHT_PROG) $(ARGS)
compiler_check:
	@echo compiler_test>compiler_test && @$(CXX) /E compiler_test >nul 2>&1  || echo "$(CXX) command not found. Check if CXX=$(CXX) is set properly"
	@cmd.exe /C del compiler_test
