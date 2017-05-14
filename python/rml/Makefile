# Copyright (c) 2005-2017 Intel Corporation
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

TBB_ROOT = ../..
BUILDING_PHASE:=0
tbb_root = $(TBB_ROOT)
include $(TBB_ROOT)/build/common.inc

INCS += -I $(TBB_ROOT)/src
INCS += -I $(TBB_ROOT)/include
INCS += -I $(TBB_ROOT)/src/rml/include
INCS += -I .

SRCS += ipc_server.cpp
SRCS += ipc_utils.cpp
SRCS += $(TBB_ROOT)/src/tbb/cache_aligned_allocator.cpp
SRCS += $(TBB_ROOT)/src/tbb/dynamic_link.cpp
SRCS += $(TBB_ROOT)/src/tbb/tbb_misc.cpp
SRCS += $(TBB_ROOT)/src/tbb/tbb_misc_ex.cpp

OBJS += ipc_server.o
OBJS += ipc_utils.o
OBJS += cache_aligned_allocator.o
OBJS += dynamic_link.o
OBJS += tbb_misc.o
OBJS += tbb_misc_ex.o

CFLAGS = -c -fpic -lpthread -DUSE_PTHREAD
CFLAGS_DEBUG = -O0 -g -DTBB_USE_DEBUG
CFLAGS_RELEASE = -O2

.PHONY: all clean

all: release debug

clean:
	-rm -rf *.o *.so* version_string.ver
	-rm -rf $(work_dir)_release/libirml*
	-rm -rf $(work_dir)_debug/libirml*

mkdir_release:
	$(shell $(MD) "$(work_dir)_release" >$(NUL) 2>$(NUL))
	@echo Created $(work_dir)_release directory

mkdir_debug:
	$(shell $(MD) "$(work_dir)_debug" >$(NUL) 2>$(NUL))
	@echo Created $(work_dir)_debug directory

ifeq ($(OS), Linux)
debug: libirml_debug
release: libirml_release
else
debug:
release:
endif

libirml_debug: BUILDING_LIBRARY = libirml_debug.so.1
libirml_debug: mkdir_debug libirml_debug.o
	$(CXX) $(LIB_LINK_FLAGS) -o $(work_dir)_debug/libirml_debug.so.1 $(OBJS)
	echo "INPUT (libirml_debug.so.1) > "$(work_dir)_debug/libirml_debug.so

libirml_debug.o: $(SRCS) version_info
	$(CXX) $(CFLAGS) $(CFLAGS_DEBUG) $(INCS) $(SRCS)

libirml_release: BUILDING_LIBRARY = libirml.so.1
libirml_release: mkdir_release libirml_release.o
	$(CXX) $(LIB_LINK_FLAGS) -o $(work_dir)_release/libirml.so.1 $(OBJS)
	echo "INPUT (libirml.so.1)" > $(work_dir)_release/libirml.so

libirml_release.o: $(SRCS) version_info
	$(CXX) $(CFLAGS) $(CFLAGS_RELEASE) $(INCS) $(SRCS)

version_info:
	$(MAKE_VERSIONS)
