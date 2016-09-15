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

CPLUS = icpc
CONLY = icc
COMPILE_ONLY = -c -MMD
PREPROC_ONLY = -E -x c++
INCLUDE_KEY = -I
DEFINE_KEY = -D
OUTPUT_KEY = -o #
OUTPUTOBJ_KEY = -o #
PIC_KEY = -fPIC
WARNING_AS_ERROR_KEY = -Werror
WARNING_KEY = -w1
DYLIB_KEY = -dynamiclib
EXPORT_KEY = -Wl,-exported_symbols_list,
LIBDL = -ldl

LIBS = -lpthread
LINK_FLAGS =
LIB_LINK_FLAGS = -dynamiclib -static-intel -install_name @rpath/$(BUILDING_LIBRARY)
C_FLAGS = $(CPLUS_FLAGS)
# ICC 11.0 and higher support -std=c++0x
ifneq (,$(shell icc -dumpversion | egrep  "^1[1-9]\."))
    CPP11_FLAGS = -std=c++0x -D_TBB_CPP0X
endif

ifneq (,$(shell icc -dumpversion | egrep  "^1[6-9]\."))
OPENMP_FLAG = -qopenmp
else
OPENMP_FLAG = -openmp
endif

# ICC 12.0 and higher provide Intel(R) Cilk(TM) Plus
ifneq (,$(shell icc -dumpversion | egrep  "^1[2-9]\."))
    CILK_AVAILABLE = yes
endif

ifeq ($(cfg), release)
    CPLUS_FLAGS = -g -O2 -fno-omit-frame-pointer
else
    CPLUS_FLAGS = -g -O0 -DTBB_USE_DEBUG
endif

CPLUS_FLAGS += -DUSE_PTHREAD

ifneq (,$(codecov))
    CPLUS_FLAGS += -prof-genx
endif

ifneq (00,$(lambdas)$(cpp0x))
	CXX_ONLY_FLAGS += $(CPP11_FLAGS)
endif

# ICC 14.0 and higher support usage of libc++, clang standard library
# Extending minimal deployment version of official binaries to unsupported 10.8
# per customer requests. TODO: make a dedicated variable for this
ifneq (,$(shell icc -dumpversion | egrep  "^1[4-9]\."))
ifneq (,$(stdlib))
    CPLUS_FLAGS    += -stdlib=$(stdlib) -mmacosx-version-min=10.8
    LIB_LINK_FLAGS += -stdlib=$(stdlib) -mmacosx-version-min=10.8
endif
endif

#------------------------------------------------------------------------------
# Setting assembler data.
#------------------------------------------------------------------------------

ASM = as
ifeq (intel64,$(arch))
    ASM_FLAGS += -arch x86_64
endif
ifeq (ia32,$(arch))
    CPLUS_FLAGS += -m32
    LINK_FLAGS += -m32
    LIB_LINK_FLAGS += -m32
    ASM_FLAGS += -arch i386
endif
ifeq ($(cfg), debug)
    ASM_FLAGS += -g
endif

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
