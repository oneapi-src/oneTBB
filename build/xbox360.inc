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

ifdef tbb_build_dir
  test_dir:=$(tbb_build_dir)
else
  test_dir:=.
endif

# TODO give an error if archs doesn't match
ifndef arch
  export arch:=xbox360
endif

ifndef runtime
  export runtime:=xdk
endif

native_compiler := cl
export compiler ?= cl
debugger ?= devenv /debugexe

CMD=cmd /C
CWD=$(shell cmd /C echo %CD%)
RM=cmd /C del /Q /F
RD=cmd /C rmdir
MD=cmd /c mkdir
SLASH=\\
NUL = nul

OBJ = obj
DLL = dll
LIBEXT = lib

def_prefix = $(arch)

# Target Windows version. Do not increase beyond 0x0500 without prior discussion!
# Used as the value for macro definition option in compiler specific inc files.
_WIN32_WINNT=0x0400

TBB.LST =
TBB.DEF = $(tbb_root)/src/tbb/$(def_prefix)-tbb-export.def
TBB.DLL = tbb$(CPF_SUFFIX)$(DEBUG_SUFFIX).$(DLL)
TBB.LIB = tbb$(CPF_SUFFIX)$(DEBUG_SUFFIX).$(LIBEXT)
TBB.RES =
#On Windows we specify appropriate tbb library using #pragma comment
LINK_TBB.LIB =

MALLOC.DEF = $(MALLOC_ROOT)/$(def_prefix)-tbbmalloc-export.def
MALLOC.DLL = tbbmalloc$(DEBUG_SUFFIX).$(DLL)
MALLOC.LIB = tbbmalloc$(DEBUG_SUFFIX).$(LIBEXT)
#On Windows we specify appropriate tbbmalloc library using #pragma comment
LINK_MALLOC.LIB =
MALLOC.RES =

MAKE_VERSIONS = cmd /C cscript /nologo /E:jscript $(subst \,/,$(tbb_root))/build/version_info_windows.js $(compiler) $(arch) $(subst \,/,"$(VERSION_FLAGS)") > version_string.ver
MAKE_TBBVARS  = cmd /C "$(subst /,\,$(tbb_root))\build\generate_tbbvars.bat"
