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

####### Detections and Commands ###############################################

# Must set def_prefix according to target architecture detected above
ifeq (ia32,$(arch))
    def_prefix = lin32
endif
ifeq (arm,$(findstring arm,$(arch)))
    def_prefix = lin32
endif
ifeq (64,$(findstring 64,$(arch)))
    def_prefix = lin64
endif

gcc_version = $(shell $(tbb_tool_prefix)g++ -dumpversion)

ifdef ANDROID_NDK_ROOT
    ndk_version:= $(lastword $(subst -, ,$(ANDROID_NDK_ROOT)))
    $(warning "NDK version $(ndk_version)")
else
    $(warning "NDK version not set in environment, using \'unknown\' instead.")
    ndk_version:=unknown
endif

export runtime:=$(target)_cc$(gcc_version)_NDK$(ndk_version)_version_$(target_os_version)

AR = $(tbb_tool_prefix)ar
MAKE_VERSIONS=sh $(tbb_root)/build/version_info_android.sh $(VERSION_FLAGS) >version_string.ver

####### Build settings ########################################################

# No SONAME_SUFFIX for Android allowed in library names
TBB.LST = $(tbb_root)/src/tbb/$(def_prefix)-tbb-export.lst
TBB.DEF = $(TBB.LST:.lst=.def)
TBB.DLL = libtbb$(CPF_SUFFIX)$(DEBUG_SUFFIX).$(DLL)
TBB.LIB = $(TBB.DLL)
TBB_NO_VERSION.DLL=
LINK_TBB.LIB = $(TBB.LIB)

MALLOC.DEF = $(MALLOC_ROOT)/$(def_prefix)-tbbmalloc-export.def
MALLOC.DLL = libtbbmalloc$(DEBUG_SUFFIX).$(DLL)
MALLOC.LIB = $(MALLOC.DLL)
MALLOC_NO_VERSION.DLL=
LINK_MALLOC.LIB = $(MALLOC.LIB)

MALLOCPROXY.DEF = $(MALLOC_ROOT)/$(def_prefix)-proxy-export.def
MALLOCPROXY.DLL = libtbbmalloc_proxy$(DEBUG_SUFFIX).$(DLL)
MALLOCPROXY_NO_VERSION.DLL=
MALLOCPROXY.LIB = $(MALLOCPROXY.DLL)
LINK_MALLOCPROXY.LIB = $(MALLOCPROXY.LIB)

TBB.RES =
MALLOC.RES =
RML.RES =
TBB.MANIFEST =
MALLOC.MANIFEST =
RML.MANIFEST =
OBJ = o
DLL = so

TEST_LAUNCHER=
run_cmd ?= -sh $(tbb_root)/build/android.linux.launcher.sh $(largs)
