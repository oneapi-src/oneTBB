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
# Defines settings for building the TBB run-time as a static library.
# Use these only on platforms where dynamic linking is impractical.
#
# IF YOU USE TBB AS A STATIC LIBRARY, YOU MUST GUARANTEE THAT ONLY ONE COPY OF 
# THE TBB RUN-TIME IS LINKED INTO AN APPLICATION!  LINKING IN MULTIPLE COPIES
# OF THE TBB RUN-TIME, DIRECTLY OR INDIRECTLY, MAY CAUSE PROGRAM FAILURE!
#------------------------------------------------------------------------------

# Note that ITT_NOTIFY allows to selectively remove the definition of
# DO_ITT_NOTIFY without sabotaging deferred expansion of CPLUS_FLAGS.
# TODO: currently only in linux.{gcc,xl}.inc

# Note that -pthread with xl gives "1501-210 (W) command option t contains an incorrect subargument";
# multithreading is instead achieved by using the _r affix in the compiler name.
# TODO: is -lpthread still relevant/needed with XL and _r affix?

# Note that usage of dynamic (shared) libraries is disabled
# (via -D__TBB_DYNAMIC_LOAD_ENABLED=0 and LIBDL emptied) primarily for performance.

# OS specific settings =>
    LIB_LINK_CMD = ar rcs
    LIB_LINK_FLAGS =
    LIB_LINK_LIBS =
    LIB_OUTPUT_KEY =
    DYLIB_KEY =
    ifeq ($(tbb_os),linux)
        ifeq ($(compiler),clang)
            LIBS = -pthread -lrt
        endif
        ifeq ($(compiler),gcc)
            LIBS = -pthread -lrt
        endif
        ifeq ($(compiler),xl)
            LIBS = -lpthread -lrt
        endif
        LINK_FLAGS =
    endif
    override CXXFLAGS += -D__TBB_DYNAMIC_LOAD_ENABLED=0 -D__TBB_SOURCE_DIRECTLY_INCLUDED=1
    ITT_NOTIFY =
    DLL = a
    LIBEXT = a
    LIBPREF = lib
    LIBDL =
# <= OS specific settings

TBB.DLL = $(LIBPREF)tbb$(DEBUG_SUFFIX).$(LIBEXT)
LINK_TBB.LIB = $(TBB.DLL)
TBB.LST =
TBB.DEF =
TBB_NO_VERSION.DLL =

MALLOC.DLL = $(LIBPREF)tbbmalloc$(DEBUG_SUFFIX).$(LIBEXT)
LINK_MALLOC.LIB = $(MALLOC.DLL)
MALLOC.DEF =
MALLOC_NO_VERSION.DLL =
MALLOCPROXY.DLL =
MALLOCPROXY.DEF =
