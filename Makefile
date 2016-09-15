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

tbb_root?=.
include $(tbb_root)/build/common.inc
.PHONY: default all tbb tbbmalloc tbbproxy test examples

#workaround for non-depend targets tbb and tbbmalloc which both depend on version_string.ver
#According to documentation, recursively invoked make commands can process their targets in parallel
.NOTPARALLEL: tbb tbbmalloc tbbproxy

default: tbb tbbmalloc $(if $(use_proxy),tbbproxy)

all: tbb tbbmalloc tbbproxy test examples

tbb: mkdir
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbb cfg=debug
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbb cfg=release

tbbmalloc: mkdir
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=debug malloc
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=release malloc

tbbproxy: mkdir
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbbproxy cfg=debug tbbproxy
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbbproxy cfg=release tbbproxy

test: tbb tbbmalloc $(if $(use_proxy),tbbproxy)
	-$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=debug malloc_test
	-$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.test cfg=debug
	-$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=release malloc_test
	-$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.test cfg=release

rml: mkdir
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.rml cfg=debug
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.rml cfg=release


examples: tbb tbbmalloc
	$(MAKE) -C examples -r -f Makefile tbb_root=.. release test

.PHONY: clean clean_examples mkdir info

clean: clean_examples
	$(shell $(RM) $(work_dir)_release$(SLASH)*.* >$(NUL) 2>$(NUL))
	$(shell $(RD) $(work_dir)_release >$(NUL) 2>$(NUL))
	$(shell $(RM) $(work_dir)_debug$(SLASH)*.* >$(NUL) 2>$(NUL))
	$(shell $(RD) $(work_dir)_debug >$(NUL) 2>$(NUL))
	@echo clean done

clean_examples:
	$(shell $(MAKE) -s -i -r -C examples -f Makefile tbb_root=.. clean >$(NUL) 2>$(NUL))

mkdir:
	$(shell $(MD) "$(work_dir)_release" >$(NUL) 2>$(NUL))
	$(shell $(MD) "$(work_dir)_debug" >$(NUL) 2>$(NUL))
	@echo Created $(work_dir)_release and ..._debug directories

info:
	@echo OS: $(tbb_os)
	@echo arch=$(arch)
	@echo compiler=$(compiler)
	@echo runtime=$(runtime)
	@echo tbb_build_prefix=$(tbb_build_prefix)

