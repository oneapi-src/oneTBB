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

#------------------------------------------------------------------------------
# Define compiler-specific variables.
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Setting default configuration to release.
#------------------------------------------------------------------------------
cfg ?= release
#------------------------------------------------------------------------------
# End of setting default configuration to release.
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Setting compiler flags.
#------------------------------------------------------------------------------
CPLUS = icl /nologo $(VCCOMPAT_FLAG)
LINK_FLAGS = /link /nologo
LIB_LINK_FLAGS= /link /nologo /DLL /MAP /DEBUG /fixed:no /INCREMENTAL:NO /DYNAMICBASE /NXCOMPAT

ifeq ($(arch), ia32)
	LIB_LINK_FLAGS += /SAFESEH
endif


# ICC 11.0 and higher support -std=c++0x
ifeq (ok,$(call detect_js,/minversion icl 11))
    CPP11_FLAGS = /Qstd=c++0x /D_TBB_CPP0X
endif

# ICC 12.0 and higher provide Intel(R) Cilk(TM) Plus
ifeq (ok,$(call detect_js,/minversion icl 12))
    CILK_AVAILABLE = yes
endif

ifeq ($(runtime), vc_mt)
    MS_CRT_KEY = /MT$(if $(findstring debug,$(cfg)),d)
else
    MS_CRT_KEY = /MD$(if $(findstring debug,$(cfg)),d)
endif
EH_FLAGS = /EHsc /GR

ifeq ($(cfg), release)
    CPLUS_FLAGS = $(MS_CRT_KEY) /O2 /Zi $(EH_FLAGS) /Zc:forScope /Zc:wchar_t /D__TBB_LIB_NAME=$(TBB.LIB)
    ASM_FLAGS =
endif
ifeq ($(cfg), debug)
    CPLUS_FLAGS = $(MS_CRT_KEY) /Od /Ob0 /Zi $(EH_FLAGS) /Zc:forScope /Zc:wchar_t /DTBB_USE_DEBUG /D__TBB_LIB_NAME=$(TBB.LIB)
    ASM_FLAGS = /DUSE_FRAME_POINTER
endif
CPLUS_FLAGS += /GS

COMPILE_ONLY = /c /QMMD
# PREPROC_ONLY should really use /TP which applies to all files in the command line.
# But with /TP, ICL does not preprocess *.def files.
PREPROC_ONLY = /EP /Tp
INCLUDE_KEY = /I
DEFINE_KEY = /D
OUTPUT_KEY = /Fe
OUTPUTOBJ_KEY = /Fo
WARNING_AS_ERROR_KEY = /WX
WARNING_KEY = /W3
DYLIB_KEY = /DLL
EXPORT_KEY = /DEF:
NODEFAULTLIB_KEY = /Zl
NOINTRINSIC_KEY = /Oi-


ifneq (,$(codecov))
    CPLUS_FLAGS += /Qprof-genx
else
    CPLUS_FLAGS += /DDO_ITT_NOTIFY
endif

OPENMP_FLAG = /Qopenmp
CPLUS_FLAGS += /DUSE_WINTHREAD /D_CRT_SECURE_NO_DEPRECATE \
               /D_WIN32_WINNT=$(_WIN32_WINNT)

ifeq ($(runtime),vc8)
    CPLUS_FLAGS += /D_USE_RTM_VERSION
endif


C_FLAGS = $(CPLUS_FLAGS)

ifneq (00,$(lambdas)$(cpp0x))
    CXX_ONLY_FLAGS += $(CPP11_FLAGS)
endif

VCVERSION:=$(runtime)
VCCOMPAT_FLAG ?= $(if $(findstring vc7.1, $(VCVERSION)),/Qvc7.1)
ifeq ($(VCCOMPAT_FLAG),)
        VCCOMPAT_FLAG := $(if $(findstring vc8, $(VCVERSION)),/Qvc8)
endif
ifeq ($(VCCOMPAT_FLAG),)
        VCCOMPAT_FLAG := $(if $(findstring vc_mt, $(VCVERSION)),/Qvc10)
endif
ifeq ($(VCCOMPAT_FLAG),)
        VCCOMPAT_FLAG := $(if $(findstring vc9, $(VCVERSION)),/Qvc9)
endif
ifeq ($(VCCOMPAT_FLAG),)
        VCCOMPAT_FLAG := $(if $(findstring vc10, $(VCVERSION)),/Qvc10)
endif
ifeq ($(VCCOMPAT_FLAG),)
        VCCOMPAT_FLAG := $(if $(findstring vc11, $(VCVERSION)),/Qvc11)
endif
ifeq ($(VCCOMPAT_FLAG),)
        VCCOMPAT_FLAG := $(if $(findstring vc12, $(VCVERSION)),/Qvc12)
endif
ifeq ($(VCCOMPAT_FLAG),)
        VCCOMPAT_FLAG := $(if $(findstring vc14, $(VCVERSION)),/Qvc14)
endif
ifeq ($(VCCOMPAT_FLAG),)
        $(error VC version not detected correctly: $(VCVERSION) )
endif
export VCCOMPAT_FLAG

#------------------------------------------------------------------------------
# End of setting compiler flags.
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Setting assembler data.
#------------------------------------------------------------------------------
ASSEMBLY_SOURCE=$(arch)-masm
ifeq (intel64,$(arch))
    ASM=ml64 /nologo
    ASM_FLAGS += /DEM64T=1 /c /Zi
    TBB_ASM.OBJ = atomic_support.obj intel64_misc.obj itsx.obj
    MALLOC_ASM.OBJ = atomic_support.obj
else
    ASM=ml /nologo
    ASM_FLAGS += /c /coff /Zi /safeseh
    TBB_ASM.OBJ = atomic_support.obj lock_byte.obj itsx.obj
endif
#------------------------------------------------------------------------------
# End of setting assembler data.
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Setting tbbmalloc data.
#------------------------------------------------------------------------------
M_CPLUS_FLAGS = $(subst $(EH_FLAGS),/EHs-,$(CPLUS_FLAGS))
#------------------------------------------------------------------------------
# End of setting tbbmalloc data.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# End of define compiler-specific variables.
#------------------------------------------------------------------------------
