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

ifeq (icc,$(compiler))
  export COMPILER_VERSION := ICC: $(shell icc -V </dev/null 2>&1 | grep 'Version')
  ifneq (,$(findstring running on IA-32, $(COMPILER_VERSION)))
    export arch:=ia32
  else
    ifneq (,$(findstring running on Intel(R) 64, $(COMPILER_VERSION)))
      export arch:=intel64
    endif
  endif
  ifeq (,$(arch))
    $(warning "Unknown Intel compiler")
  endif
endif

ifndef arch
 ifeq ($(shell /usr/sbin/sysctl -n hw.machine),Power Macintosh)
   ifeq ($(shell /usr/sbin/sysctl -n hw.optional.64bitops),1)
     export arch:=ppc64
   else
     export arch:=ppc32
   endif
 else
   ifeq ($(shell /usr/sbin/sysctl -n hw.optional.x86_64 2>/dev/null),1)
     export arch:=intel64
   else
     export arch:=ia32
   endif
 endif
endif

ifndef runtime
  clang_version:=$(shell clang -v 2>&1 >/dev/null | grep version | sed -e "s/.*version \(.*[0-9]\) .*/\1/")
  ifndef os_version
    os_version:=$(shell /usr/bin/sw_vers -productVersion)
  endif
  export runtime:=cc$(clang_version)_os$(os_version)
endif

native_compiler := clang
export compiler ?= clang
debugger ?= lldb

export stdlib ?= libc++

CMD=$(SHELL) -c
CWD=$(shell pwd)
RM?=rm -f
RD?=rmdir
MD?=mkdir -p
NUL= /dev/null
SLASH=/
MAKE_VERSIONS=sh $(tbb_root)/build/version_info_macos.sh $(VERSION_FLAGS) >version_string.ver
MAKE_TBBVARS=sh $(tbb_root)/build/generate_tbbvars.sh DY

ifdef DYLD_LIBRARY_PATH
        export DYLD_LIBRARY_PATH := .:$(DYLD_LIBRARY_PATH)
else
        export DYLD_LIBRARY_PATH := .
endif

####### Build settings ########################################################

OBJ=o
DLL=dylib
MALLOC_DLL?=$(DLL)
LIBEXT=dylib

def_prefix = $(if $(findstring 64,$(arch)),mac64,mac32)

TBB.LST = $(tbb_root)/src/tbb/$(def_prefix)-tbb-export.lst
TBB.DEF = $(TBB.LST:.lst=.def)
TBB.DLL = libtbb$(CPF_SUFFIX)$(DEBUG_SUFFIX).$(DLL)
TBB.LIB = $(TBB.DLL)
LINK_TBB.LIB = $(TBB.LIB)

MALLOC.DEF = $(MALLOC_ROOT)/$(def_prefix)-tbbmalloc-export.def
MALLOC.DLL = libtbbmalloc$(DEBUG_SUFFIX).$(MALLOC_DLL)
MALLOC.LIB = $(MALLOC.DLL)
LINK_MALLOC.LIB = $(MALLOC.LIB)

MALLOCPROXY.DLL = libtbbmalloc_proxy$(DEBUG_SUFFIX).$(MALLOC_DLL)
MALLOCPROXY.LIB = $(MALLOCPROXY.DLL)
LINK_MALLOCPROXY.LIB = $(MALLOCPROXY.LIB)

TEST_LAUNCHER=sh $(tbb_root)/build/test_launcher.sh $(largs)

OPENCL.LIB = -framework OpenCL
