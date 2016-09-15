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

#
# Extra gmake command-line parameters for use with Android:
#
#    dlopen_workaround:  Some OS versions need workaround for dlopen to avoid recursive calls.
#

####### Detections and Commands ###############################################

ifeq (android,$(findstring android,$(tbb_os)))
    $(error TBB only supports cross-compilation for Android. Specify "target=android" instead.)
endif

ifneq ("command line","$(origin arch)")
    ifeq (icc,$(compiler))
        export COMPILER_VERSION := ICC: $(shell icc -V </dev/null 2>&1 | grep 'Version')
        ifneq (,$(findstring IA-32, $(COMPILER_VERSION)))
            export arch:=ia32
        else
            $(error "No support for Android in $(COMPILER_VERSION)")
        endif

    else
        ifdef ANDROID_SERIAL
            uname_m:=$(shell adb shell uname -m)
            ifeq (i686,$(uname_m))
                export arch:=ia32
            else
                export arch:=$(uname_m)
            endif
        else
            ifndef arch
                $(error "No target architecture specified and \'ANDROID_SERIAL\' environment variable specifying target device not set")
            endif
        endif
    endif
endif

# Many OS versions (Android 4.0.[0-3] for example) need workaround for dlopen to avoid non-recursive loader lock hang
export dlopen_workaround = 1

# Android platform only supported from TBB 4.1 forward
NO_LEGACY_TESTS = 1


