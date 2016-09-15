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

#------------------------------------------------------------------------------
# Overriding settings from windows.inc
#------------------------------------------------------------------------------

SLASH= $(strip \)
OBJ = o
LIBEXT = dll # MinGW allows linking with DLLs directly

TBB.RES =
MALLOC.RES =
RML.RES =
TBB.MANIFEST =
MALLOC.MANIFEST =
RML.MANIFEST =

ifeq (ia32,$(arch))
    TBB.LST = $(tbb_root)/src/tbb/lin32-tbb-export.lst
else
    TBB.LST = $(tbb_root)/src/tbb/win64-gcc-tbb-export.lst
endif
MALLOC.DEF = $(MALLOC_ROOT)/$(def_prefix)-gcc-tbbmalloc-export.def
RML.DEF = $(RML_SERVER_ROOT)/lin-rml-export.def

LINK_TBB.LIB = $(TBB.LIB)
# no TBB proxy for the configuration
PROXY.LIB =

#------------------------------------------------------------------------------
# End of overridden settings
#------------------------------------------------------------------------------
# Compiler-specific variables
#------------------------------------------------------------------------------

CPLUS = g++
COMPILE_ONLY = -c -MMD
PREPROC_ONLY = -E -x c++
INCLUDE_KEY = -I
DEFINE_KEY = -D
OUTPUT_KEY = -o #
OUTPUTOBJ_KEY = -o #
PIC_KEY =
WARNING_AS_ERROR_KEY = -Werror
WARNING_KEY = -Wall
TEST_WARNING_KEY = -Wextra -Wshadow -Wcast-qual -Woverloaded-virtual -Wnon-virtual-dtor -Wno-uninitialized
WARNING_SUPPRESS = -Wno-parentheses -Wno-uninitialized -Wno-non-virtual-dtor
DYLIB_KEY = -shared
LIBDL =
EXPORT_KEY = -Wl,--version-script,
LIBS = -lpsapi

#------------------------------------------------------------------------------
# End of compiler-specific variables
#------------------------------------------------------------------------------
# Command lines
#------------------------------------------------------------------------------

LINK_FLAGS = -Wl,--enable-auto-import
LIB_LINK_FLAGS = $(DYLIB_KEY)
# gcc 4.4 and higher support -std=c++0x
ifeq (ok,$(call detect_js,/minversion gcc 4.4))
    CPP11_FLAGS = -std=c++0x -D_TBB_CPP0X
endif

# gcc 4.8 and later support RTM intrinsics, but require command line switch to enable them
ifeq (ok,$(call detect_js,/minversion gcc 4.8))
    RTM_KEY = -mrtm
endif

ifeq ($(cfg), release)
        CPLUS_FLAGS = -g -O2
endif
ifeq ($(cfg), debug)
        CPLUS_FLAGS = -g -O0 -DTBB_USE_DEBUG
endif

ifneq (00,$(lambdas)$(cpp0x))
    CXX_ONLY_FLAGS += $(CPP11_FLAGS)
endif

CPLUS_FLAGS += -DUSE_WINTHREAD
CPLUS_FLAGS += -D_WIN32_WINNT=$(_WIN32_WINNT)

# MinGW specific
CPLUS_FLAGS += -DMINGW_HAS_SECURE_API=1 -D__MSVCRT_VERSION__=0x0700 -msse -mthreads

CONLY = gcc
debugger = gdb
C_FLAGS = $(CPLUS_FLAGS)

ifeq (intel64,$(arch))
    CPLUS_FLAGS += -m64 $(RTM_KEY)
    LIB_LINK_FLAGS += -m64
endif

ifeq (ia32,$(arch))
    CPLUS_FLAGS += -m32 -march=i686 $(RTM_KEY)
    LIB_LINK_FLAGS += -m32
endif

# For examples
export UNIXMODE = 1

#------------------------------------------------------------------------------
# End of command lines
#------------------------------------------------------------------------------
# Setting assembler data
#------------------------------------------------------------------------------

ASM=
ASM_FLAGS=
TBB_ASM.OBJ=
ASSEMBLY_SOURCE=$(arch)-gas

#------------------------------------------------------------------------------
# End of setting assembler data
#------------------------------------------------------------------------------
# Setting tbbmalloc data
#------------------------------------------------------------------------------

M_CPLUS_FLAGS = $(CPLUS_FLAGS) -fno-rtti -fno-exceptions

#------------------------------------------------------------------------------
# End of setting tbbmalloc data
#------------------------------------------------------------------------------
