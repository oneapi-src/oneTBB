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
WARNING_KEY =
TEST_WARNING_KEY = -Wshadow -Woverloaded-virtual -Wextra

WARNING_SUPPRESS = -Wno-parentheses -Wno-non-virtual-dtor
DYLIB_KEY = -shared
EXPORT_KEY = -Wl,--version-script,
LIBDL = -ldl

CPLUS = icpc
CONLY = icc

# -soname is necessary for proper linkage to TBB prebuilt libraries when building application with Android SDK
LIB_LINK_FLAGS = $(DYLIB_KEY) -Wl,-soname=$(BUILDING_LIBRARY)

# pie is necessary for test executables to work and might be removed if newer NDK will add it implicitly
PIE_FLAG = -pie
ifeq ($(APP_PIE), false)
    PIE_FLAG=
endif

LINK_FLAGS = -Wl,-rpath-link=. -rdynamic 
C_FLAGS = $(CPLUS_FLAGS)
 
CPP11_FLAGS = -std=c++11 $(DEFINE_KEY)_TBB_CPP0X

ifeq ($(cfg), release)
    CPLUS_FLAGS = -O2
endif
ifeq ($(cfg), debug)
    CPLUS_FLAGS = -g -O0 $(DEFINE_KEY)TBB_USE_DEBUG
endif

CPLUS_FLAGS += $(DEFINE_KEY)USE_PTHREAD $(DEFINE_KEY)_GLIBCXX_HAVE_FENV_H 

ifneq (,$(findstring $(arch),ia32 intel64))
    CPLUS_FLAGS += $(DEFINE_KEY)DO_ITT_NOTIFY
endif

ifeq (0, $(dynamic_load))
     CPLUS_FLAGS += $(DEFINE_KEY)__TBB_DYNAMIC_LOAD_ENABLED=0
endif


# Paths to the NDK prebuilt tools and libraries
CPLUS_FLAGS    += --sysroot=$(SYSROOT)
LIB_LINK_FLAGS += --sysroot=$(SYSROOT)
# the -static-intel flag is to remove the need to copy Intel-specific libs to the device.
LIBS           = -L$(CPLUS_LIB_PATH) -lgnustl_shared -static-intel

# This causes CPP11_FLAGS to be issued twice for test_lambda.cpp
# TODO: Fix this in general for all platforms once the correct strategy is determined.
ifneq (00,$(lambdas)$(cpp0x))
    CXX_ONLY_FLAGS += $(CPP11_FLAGS)
endif

ifeq (ia32,$(arch))
    # TODO: Determine best setting of -march and add to CPLUS_FLAGS
    CPLUS_FLAGS += -m32 -march=pentium4 -falign-stack=maintain-16-byte
    LIB_LINK_FLAGS += -m32
else
    ifeq (intel64,$(arch))
        CPLUS_FLAGS += -m64
        LIB_LINK_FLAGS += -m64
    endif
endif

ifeq (arm,$(findstring arm,$(arch)))
    $(error "Unsupported architecture $(arch) for icc compiler")
endif

#------------------------------------------------------------------------------
# Setting assembler data.
#------------------------------------------------------------------------------
TBB_ASM.OBJ=
MALLOC_ASM.OBJ=

ASM = $(tbb_tool_prefix)as
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
