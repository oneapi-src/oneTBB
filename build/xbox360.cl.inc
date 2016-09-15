# Copyright (c) 2005-2016 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#
#
#

#------------------------------------------------------------------------------
# Define compiler-specific variables.
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Setting compiler flags.
#------------------------------------------------------------------------------
CPLUS = cl /nologo
LINK_FLAGS = /link /nologo
LIB_LINK_FLAGS=/link /nologo /DLL /MAP /DEBUG
MS_CRT_KEY = /MT$(if $(findstring debug,$(cfg)),d)
EH_FLAGS = /EHsc /GR
               
ifeq ($(cfg), release)
        CPLUS_FLAGS = $(MS_CRT_KEY) /O2 /Zi $(EH_FLAGS) /Zc:forScope /D_XBOX /DTBB_NO_LEGACY=1
        ASM_FLAGS =
endif
ifeq ($(cfg), debug)
        CPLUS_FLAGS = $(MS_CRT_KEY) /Od /Ob0 /Zi $(EH_FLAGS) /Zc:forScope \
                      /DTBB_USE_DEBUG /D_XBOX /DTBB_NO_LEGACY=1
        ASM_FLAGS = /DUSE_FRAME_POINTER
endif


COMPILE_ONLY = /c
PREPROC_ONLY = /TP /EP
INCLUDE_KEY = /I
DEFINE_KEY = /D
OUTPUT_KEY = /Fe
OUTPUTOBJ_KEY = /Fo
WARNING_AS_ERROR_KEY = /WX
WARNING_KEY = /W3
DYLIB_KEY = /DLL
EXPORT_KEY = /DEF:

ifeq (em64t,$(arch))
        CPLUS_FLAGS += /GS-
endif

CPLUS_FLAGS += /DUSE_WINTHREAD /D_CRT_SECURE_NO_DEPRECATE /D_WIN32_WINNT=$(_WIN32_WINNT)
C_FLAGS = $(CPLUS_FLAGS) /TC
#------------------------------------------------------------------------------
# End of setting compiler flags.
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Setting assembler data.
#------------------------------------------------------------------------------
# nothing for XBOX360
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
