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

CPLUS = clang++
CONLY = clang
COMPILE_ONLY = -c -MMD
PREPROC_ONLY = -E -x c++
INCLUDE_KEY = -I
DEFINE_KEY = -D
OUTPUT_KEY = -o #
OUTPUTOBJ_KEY = -o #
PIC_KEY = -fPIC
WARNING_AS_ERROR_KEY = -Werror
WARNING_KEY = -Wall
TEST_WARNING_KEY = -Wextra -Wshadow -Wcast-qual -Woverloaded-virtual -Wnon-virtual-dtor
WARNING_SUPPRESS = -Wno-non-virtual-dtor -Wno-dangling-else
DYLIB_KEY = -dynamiclib
EXPORT_KEY = -Wl,-exported_symbols_list,
LIBDL = -ldl

LIBS = -lpthread
LINK_FLAGS =
LIB_LINK_FLAGS = -dynamiclib -install_name @rpath/$(BUILDING_LIBRARY)
C_FLAGS = $(CPLUS_FLAGS)

ifeq ($(cfg), release)
    CPLUS_FLAGS = -g -O2
else
    CPLUS_FLAGS = -g -O0 -DTBB_USE_DEBUG
endif

CPLUS_FLAGS += -DUSE_PTHREAD

# For Clang, we add the option to support RTM intrinsics *iff* xtest is found in <immintrin.h>
ifneq (,$(shell grep xtest `echo "\#include<immintrin.h>" | clang -E -M - 2>&1 | grep immintrin.h` 2>/dev/null))
    RTM_KEY = -mrtm
endif

ifneq (,$(stdlib))
    CPLUS_FLAGS    += -stdlib=$(stdlib)
    LIB_LINK_FLAGS += -stdlib=$(stdlib)
endif

CPP11_FLAGS = -std=c++11 -D_TBB_CPP0X
    
ifneq (00,$(lambdas)$(cpp0x))
    CXX_ONLY_FLAGS += $(CPP11_FLAGS)
endif

ifeq (intel64,$(arch))
    CPLUS_FLAGS += -m64 $(RTM_KEY)
    LINK_FLAGS += -m64
    LIB_LINK_FLAGS += -m64
endif

ifeq (ia32,$(arch))
    CPLUS_FLAGS += -m32 $(RTM_KEY)
    LINK_FLAGS += -m32
    LIB_LINK_FLAGS += -m32
endif

ifeq (ppc64,$(arch))
    CPLUS_FLAGS += -arch ppc64
    LINK_FLAGS += -arch ppc64
    LIB_LINK_FLAGS += -arch ppc64
endif

ifeq (ppc32,$(arch))
    CPLUS_FLAGS += -arch ppc
    LINK_FLAGS += -arch ppc
    LIB_LINK_FLAGS += -arch ppc
endif

ifeq ($(arch),$(filter $(arch),armv7 armv7s arm64))
    CPLUS_FLAGS += -arch $(arch)
    LINK_FLAGS += -arch $(arch)
    LIB_LINK_FLAGS += -arch $(arch)
endif

ifdef SDKROOT
    CPLUS_FLAGS += -isysroot $(SDKROOT)
    LINK_FLAGS += -L$(SDKROOT)/usr/lib/system -L$(SDKROOT)/usr/lib/
    LIB_LINK_FLAGS += -L$(SDKROOT)/usr/lib/system -L$(SDKROOT)/usr/lib/
endif

#------------------------------------------------------------------------------
# Setting assembler data.
#------------------------------------------------------------------------------

ASM = as
ifeq (intel64,$(arch))
    ASM_FLAGS += -arch x86_64
endif
ifeq (ia32,$(arch))
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

