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

COMPILE_ONLY = -c -MMD
PREPROC_ONLY = -E -x c++
INCLUDE_KEY = -I
DEFINE_KEY = -D
OUTPUT_KEY = -o #
OUTPUTOBJ_KEY = -o #
PIC_KEY = -fPIC
WARNING_AS_ERROR_KEY = -Werror
WARNING_KEY = -Wall
TEST_WARNING_KEY = -Wshadow -Wcast-qual -Woverloaded-virtual -Wnon-virtual-dtor -Wextra

WARNING_SUPPRESS = -Wno-parentheses -Wno-non-virtual-dtor
DYLIB_KEY = -shared
EXPORT_KEY = -Wl,--version-script,
LIBDL = -ldl

CPLUS = pathCC
CONLY = pathcc
LIB_LINK_FLAGS = $(DYLIB_KEY) -Wl,-soname=$(BUILDING_LIBRARY)
LIBS += -lstl -lpthread -lrt
LINK_FLAGS = -Wl,-rpath-link=. -rdynamic
C_FLAGS = $(CPLUS_FLAGS)

CPP11_FLAGS = -std=c++0x -D_TBB_CPP0X
OPENMP_FLAG = -openmp

ifeq ($(cfg), release)
        CPLUS_FLAGS = $(ITT_NOTIFY) -g -O2 -DUSE_PTHREAD
endif
ifeq ($(cfg), debug)
        CPLUS_FLAGS = -DTBB_USE_DEBUG $(ITT_NOTIFY) -g -O0 -DUSE_PTHREAD
endif

ifneq (00,$(lambdas)$(cpp0x))
    CXX_ONLY_FLAGS += $(CPP11_FLAGS)
endif

TBB_ASM.OBJ=
MALLOC_ASM.OBJ=

ifeq (intel64,$(arch))
    ITT_NOTIFY = -DDO_ITT_NOTIFY
    CPLUS_FLAGS += -m64
    LIB_LINK_FLAGS += -m64
endif

ifeq (ia32,$(arch))
    ITT_NOTIFY = -DDO_ITT_NOTIFY
    CPLUS_FLAGS += -m32 -march=pentium4
    LIB_LINK_FLAGS += -m32
endif

#------------------------------------------------------------------------------
# Setting assembler data.
#------------------------------------------------------------------------------
ASM = as
ifeq (intel64,$(arch))
    ASM_FLAGS += --64
endif
ifeq (ia32,$(arch))
    ASM_FLAGS += --32
endif
ifeq ($(cfg),debug)
    ASM_FLAGS += -g
endif

ASSEMBLY_SOURCE=$(arch)-gas
#------------------------------------------------------------------------------
# End of setting assembler data.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Setting tbbmalloc data.
#------------------------------------------------------------------------------

M_CPLUS_FLAGS = $(CPLUS_FLAGS) -fno-rtti -fno-exceptions

#------------------------------------------------------------------------------
# End of setting tbbmalloc data.
#------------------------------------------------------------------------------
