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

####### Detections and Commands ###############################################

COMPILE_ONLY = -c
PREPROC_ONLY = -E -qsourcetype=c
INCLUDE_KEY = -I
DEFINE_KEY = -D
OUTPUT_KEY = -o #
OUTPUTOBJ_KEY = -o #
PIC_KEY = -qpic
WARNING_AS_ERROR_KEY = -qhalt=w
WARNING_KEY =
TEST_WARNING_KEY =

WARNING_SUPPRESS =
DYLIB_KEY = -qmkshrobj
EXPORT_KEY = -Wl,--version-script,
LIBDL = -ldl

CPLUS = xlc++_r
CONLY = xlc_r
LIB_LINK_FLAGS = $(DYLIB_KEY) -Wl,-soname=$(BUILDING_LIBRARY)
LIBS = -lpthread -lrt
C_FLAGS = $(CPLUS_FLAGS)

ifeq ($(cfg), release)
        CPLUS_FLAGS = $(ITT_NOTIFY) -O2 -DUSE_PTHREAD
endif
ifeq ($(cfg), debug)
        CPLUS_FLAGS = -DTBB_USE_DEBUG $(ITT_NOTIFY) -g -O0 -DUSE_PTHREAD
endif

# Adding directly to CPLUS_FLAGS instead of to WARNING_SUPPRESS because otherwise it would not be used in several tests (why not?).
# Suppress warnings like:
# - "1500-029: (W) WARNING: subprogram [...] could not be inlined into [...]."
# - "1501-201: (W) Maximum number of common component diagnostics, 10  has been exceeded."
#     see http://www-01.ibm.com/support/docview.wss?uid=swg1LI72843
#     it seems that the internal compiler error that would ensue has now been avoided, making the condition harmless
# - "1540-0198 (W) The omitted keyword "private" is assumed for base class "no_copy"."
# - "1540-0822 (W) The name "__FUNCTION__" must not be defined as a macro."
CPLUS_FLAGS += -qsuppress=1500-029:1501-201:1540-0198:1540-0822

ASM=
ASM_FLAGS=

TBB_ASM.OBJ=

ifeq (intel64,$(arch))
    ITT_NOTIFY = -DDO_ITT_NOTIFY
    CPLUS_FLAGS += -q64
    LIB_LINK_FLAGS += -q64
endif

# TODO: equivalent for -march=pentium4 in CPLUS_FLAGS
ifeq (ia32,$(arch))
    ITT_NOTIFY = -DDO_ITT_NOTIFY
    CPLUS_FLAGS += -q32 -qarch=pentium4
    LIB_LINK_FLAGS += -q32
endif

ifeq (ppc64,$(arch))
    CPLUS_FLAGS += -q64
    LIB_LINK_FLAGS += -q64
endif

ifeq (ppc32,$(arch))
    CPLUS_FLAGS += -q32
    LIB_LINK_FLAGS += -q32
endif

ifeq (bg,$(arch))
    CPLUS = bgxlC_r
    CONLY = bgxlc_r
endif

#------------------------------------------------------------------------------
# Setting tbbmalloc data.
#------------------------------------------------------------------------------

# Suppress innumerable warnings like "1540-1088 (W) The exception specification is being ignored."
# Suppress             warnings like "1540-1090 (I) The destructor of "lock" might not be called."
# TODO: aren't these warnings an indication that -qnoeh might not be appropriate?
M_CPLUS_FLAGS = $(CPLUS_FLAGS) -qnortti -qnoeh -qsuppress=1540-1088:1540-1090

#------------------------------------------------------------------------------
# End of setting tbbmalloc data.
#------------------------------------------------------------------------------
