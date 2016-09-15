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

ifndef tbb_os

  # Windows sets environment variable OS; for other systems, ask uname
  ifeq ($(OS),)
    OS:=$(shell uname)
    ifeq ($(OS),)
      $(error "Cannot detect operating system")
    endif
    export tbb_os=$(OS)
  endif

  ifeq ($(OS), Windows_NT)
    export tbb_os=windows
  endif
  ifeq ($(OS), Linux)
    export tbb_os=linux
  endif
  ifeq ($(OS), Darwin)
    export tbb_os=macos
  endif

endif # !tbb_os

ifeq ($(tbb_cpf),1)
  export CPF_SUFFIX ?=_preview
endif

ifeq (,$(wildcard $(tbb_root)/build/$(tbb_os).inc))
  $(error "$(tbb_os)" is not supported. Add build/$(tbb_os).inc file with os-specific settings )
endif

# detect arch and runtime versions, provide common host-specific definitions
include $(tbb_root)/build/$(tbb_os).inc

ifeq ($(arch),)
 $(error Architecture not detected)
endif
ifeq ($(runtime),)
 $(error Runtime version not detected)
endif

# process target-dependent compilation and testing configurations
ifdef target
 # optionally process target-dependent options for compilation and testing
 ifneq (,$(wildcard $(tbb_root)/build/$(target).inc))
  include $(tbb_root)/build/$(target).inc
 endif

 # optionally process host-dependent environment for target-dependent compilation and testing
 ifneq (,$(wildcard $(tbb_root)/build/$(target).$(tbb_os).inc))
  include $(tbb_root)/build/$(target).$(tbb_os).inc
 endif

 # insure at least one target-dependent configuration file was found for compilation and testing
 ifeq (,$(wildcard $(tbb_root)/build/$(target).inc)$(wildcard $(tbb_root)/build/$(target).$(tbb_os).inc))
  $(error "$(target)" is not supported. Add build/$(target).inc or build/$(target).$(tbb_os).inc file)
 endif
endif  #target

# Support for running debug tests to release library and vice versa
flip_cfg=$(subst _flipcfg,_release,$(subst _release,_debug,$(subst _debug,_flipcfg,$(1))))
cross_cfg = $(if $(crosstest),$(call flip_cfg,$(1)),$(1))
# Setting default configuration to release
cfg?=release

ifdef BUILDING_PHASE
 # No lambdas or other C++0x extensions by default for compilers that implement them as experimental features
 # TODO: it should become unnecessary when all relevant tests are "moved" to the TEST_TBB_CPP11 set
 lambdas ?= 0
 cpp0x ?= 0

 ifndef target
  target:=$(tbb_os)
 endif
 # process host/target compiler-dependent build configuration
 ifeq (,$(wildcard $(tbb_root)/build/$(target).$(compiler).inc))
  $(error "$(compiler)" is not supported on $(target). Add build/$(target).$(compiler).inc file with compiler-specific settings. )
 endif
 include $(tbb_root)/build/$(target).$(compiler).inc
endif

ifneq ($(BUILDING_PHASE),1)
 # definitions for top-level Makefiles
 origin_build_dir:=$(origin tbb_build_dir)
 tbb_build_dir?=$(tbb_root)$(SLASH)build
 export tbb_build_prefix?=$(tbb_os)_$(arch)_$(compiler)_$(runtime)$(CPF_SUFFIX)
 work_dir=$(tbb_build_dir)$(SLASH)$(tbb_build_prefix)
endif  # BUILDING_PHASE != 1

ifdef offload
  extra_inc=$(offload).offload.inc
endif
ifdef extra_inc
 ifneq (,$(wildcard $(tbb_root)/build/$(extra_inc)))
  include $(tbb_root)/build/$(extra_inc)
 else
  $(error specified build file: "build/$(extra_inc)" is not found. )
 endif
endif

ifndef BUILDING_PHASE
  work_dir:=$(work_dir)
  # assign new value for tbb_root if path is not absolute (the filter keeps only /* paths)
  ifeq ($(filter /% $(SLASH)%, $(subst :, ,$(tbb_root)) ),)
   full_tbb_root:=$(CURDIR)/$(tbb_root)
   ifeq ($(origin_build_dir),undefined)
   #relative path are needed here as a workaround to support whitespaces in path
    override tbb_root:=../..
   else
    override tbb_root:=$(full_tbb_root)
   endif
  export tbb_root
  endif
 endif # !BUILDING_PHASE

.DELETE_ON_ERROR:    # Make will delete target if error occurred when building it.

# MAKEOVERRIDES contains the command line variable definitions. Reseting it to
# empty allows propogating all exported overridden variables to nested makes.
# NOTEs:
#   1. All variable set in command line are propagated to nested makes.
#   2. All variables declared with the "export" keyword are propagated to
#   nested makes.
#   3. "override" allows changing variables set in command line. But it doesn't
#   propagate new values to nested makes. For propagation, the "export" keyword
#   should be used.
#   4. gmake v3.80 doesn't support exporting of target-specific variables using
#   the "export" keyword
MAKEOVERRIDES =
