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
WARNING_KEY = -w1
# Several options (e.g. -opt-streaming-stores) were deprecated in Intel(R) Compiler 15.0.
# When Intel(R) Compiler 14.0 support is dropped consider enabling remark 10009 and replacing deprecated options with
# new ones (e.g. -qopt-streaming-stores never).
WARNING_SUPPRESS = -diag-disable 10009
TEST_WARNING_KEY = -diag-disable 10009
DYLIB_KEY = -shared -Wl,-soname=$@
EXPORT_KEY = -Wl,--version-script,
NOINTRINSIC_KEY = -fno-builtin
LIBDL = -ldl

CPLUS = icpc
CONLY = icc

ifeq (release,$(cfg))
    CPLUS_FLAGS = -O2 -g -DUSE_PTHREAD
else
    CPLUS_FLAGS = -O0 -g -DUSE_PTHREAD -DTBB_USE_DEBUG
endif

ifneq (,$(codecov))
    CPLUS_FLAGS += -prof-genx
endif

ifneq (,$(shell icc -dumpversion | egrep  "^1[6-9]\."))
OPENMP_FLAG = -qopenmp
else
OPENMP_FLAG = -openmp
endif

LIB_LINK_FLAGS = -shared -static-intel -Wl,-soname=$(BUILDING_LIBRARY)
LIBS += -lpthread -lrt
C_FLAGS = $(CPLUS_FLAGS)
CPP11_FLAGS = -std=c++0x -D_TBB_CPP0X
CILK_AVAILABLE = yes

TBB_ASM.OBJ=
MALLOC_ASM.OBJ=

ifneq (00,$(lambdas)$(cpp0x))
    CXX_ONLY_FLAGS += $(CPP11_FLAGS)
endif

CPLUS_FLAGS += -DHARNESS_INCOMPLETE_SOURCES=1 -D__TBB_MIC_NATIVE -DTBB_USE_EXCEPTIONS=0 -opt-streaming-stores never
CPLUS += -mmic
CONLY += -mmic
LINK_FLAGS = -Wl,-rpath-link=. -rdynamic
# Tell the icc to not link against libcilk*. Otherwise icc tries to link and emits a warning message.
LIB_LINK_FLAGS += -no-intel-extensions

#------------------------------------------------------------------------------
# Setting tbbmalloc data.
#------------------------------------------------------------------------------

M_CPLUS_FLAGS = $(CPLUS_FLAGS) -fno-rtti -fno-exceptions

#------------------------------------------------------------------------------
# End of setting tbbmalloc data.
#------------------------------------------------------------------------------



