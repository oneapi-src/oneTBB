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

COMPILE_ONLY = -c -xMMD -errtags
PREPROC_ONLY = -E -xMMD
INCLUDE_KEY = -I
DEFINE_KEY = -D
OUTPUT_KEY = -o #
OUTPUTOBJ_KEY = -o #
PIC_KEY = -KPIC
DYLIB_KEY = -G
LIBDL = -ldl
# WARNING_AS_ERROR_KEY = -errwarn=%all
WARNING_AS_ERROR_KEY = Warning as error
# Supported Solaris Studio* 12.2 and above, remove ',inlasmpnu' in the line below to build by compiler prior Solaris Studio* 12.2 
WARNING_SUPPRESS = -erroff=unassigned,attrskipunsup,badargtype2w,badbinaryopw,wbadasg,wvarhidemem,inlasmpnu
tbb_strict=0

CPLUS = CC
CONLY = cc
LIB_LINK_FLAGS = -G -R . -M$(tbb_root)/build/suncc.map.pause
LINK_FLAGS += -M$(tbb_root)/build/suncc.map.pause
LIBS = -lpthread -lrt -R .
C_FLAGS = $(CPLUS_FLAGS)

#TODO: the $(stdlib) instead of hard-wiring STLPort 
ifeq ($(cfg), release)
        CPLUS_FLAGS = -mt -xO2 -g -library=stlport4 -DUSE_PTHREAD $(WARNING_SUPPRESS)
endif
ifeq ($(cfg), debug)
        CPLUS_FLAGS = -mt -DTBB_USE_DEBUG -g -library=stlport4 -DUSE_PTHREAD $(WARNING_SUPPRESS)
endif

ASM=
ASM_FLAGS=

TBB_ASM.OBJ=

ifeq (intel64,$(arch))
    CPLUS_FLAGS += -m64
    ASM_FLAGS += -m64
    LIB_LINK_FLAGS += -m64
endif 

ifeq (ia32,$(arch))
    CPLUS_FLAGS += -m32
    LIB_LINK_FLAGS += -m32
endif 

# TODO: verify whether -m64 implies V9 on relevant Sun Studio versions
#       (those that handle gcc assembler syntax)
ifeq (sparc,$(arch))
    CPLUS_FLAGS    += -m64
    LIB_LINK_FLAGS += -m64
endif 

export TBB_CUSTOM_VARS_SH=export CXXFLAGS="-I$${TBBROOT}/include -library=stlport4 $(CXXFLAGS) -M$${TBBROOT}/build/suncc.map.pause"
export TBB_CUSTOM_VARS_CSH=setenv CXXFLAGS "-I$${TBBROOT}/include -library=stlport4 $(CXXFLAGS) -M$${TBBROOT}/build/suncc.map.pause"

#------------------------------------------------------------------------------
# Setting assembler data.
#------------------------------------------------------------------------------
ASSEMBLY_SOURCE=$(arch)-fbe
#------------------------------------------------------------------------------
# End of setting assembler data.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Setting tbbmalloc data.
#------------------------------------------------------------------------------
M_INCLUDES = $(INCLUDES) -I$(MALLOC_ROOT) -I$(MALLOC_SOURCE_ROOT)
M_CPLUS_FLAGS = $(CPLUS_FLAGS)
#------------------------------------------------------------------------------
# End of setting tbbmalloc data.
#------------------------------------------------------------------------------
