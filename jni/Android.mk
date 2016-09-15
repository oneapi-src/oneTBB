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

export tbb_root?=$(NDK_PROJECT_PATH)

ifeq (armeabi-v7a,$(APP_ABI))
	export SYSROOT:=$(NDK_ROOT)/platforms/$(APP_PLATFORM)/arch-arm
else ifeq (arm64-v8a,$(APP_ABI))
	export SYSROOT:=$(NDK_ROOT)/platforms/$(APP_PLATFORM)/arch-arm64
else
	export SYSROOT:=$(NDK_ROOT)/platforms/$(APP_PLATFORM)/arch-$(APP_ABI)
endif

ifeq (windows,$(tbb_os))
	export CPATH_SEPARATOR :=;
else
	export CPATH_SEPARATOR :=:
endif
export CPATH := $(SYSROOT)/usr/include$(CPATH_SEPARATOR)$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/$(NDK_TOOLCHAIN_VERSION)/include$(CPATH_SEPARATOR)$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/$(NDK_TOOLCHAIN_VERSION)/libs/$(APP_ABI)/include
#LIB_GNU_STL_ANDROID is required to be set up for copying Android specific library libgnustl_shared.so to '.'
export LIB_GNU_STL_ANDROID := $(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/$(NDK_TOOLCHAIN_VERSION)/libs/$(APP_ABI)
export CPLUS_LIB_PATH := $(SYSROOT)/usr/lib -L$(LIB_GNU_STL_ANDROID)
export ANDROID_NDK_ROOT:=$(NDK_ROOT)
export target_os_version:=$(APP_PLATFORM)
export tbb_tool_prefix:=$(TOOLCHAIN_PREFIX)

include $(NDK_PROJECT_PATH)/src/Makefile
