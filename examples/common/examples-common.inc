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

# detect if a compiler can support C++11
# If CXX0XFLAGS already set, do not change it
ifneq (,$(findstring $(CXX), icc icpc clang++))
    # every supported icc or clang is OK
    CXX0XFLAGS ?= -std=c++0x
else
    ifneq (,$(findstring $(CXX), g++ gcc))
        ifneq (, $(strip $(shell $(CXX) -v 2>&1 | grep "clang-")))
            # This is clang actually,
            # every supported clang is OK
            CXX0XFLAGS ?= -std=c++0x
        else
            # support of lambda started GCC 4.5
            ifneq (, $(strip $(shell g++ -dumpversion | egrep "^(4\.[5-9]|[5-9])")))
               CXX0XFLAGS ?= -std=c++0x
            endif
        endif
    endif
endif

