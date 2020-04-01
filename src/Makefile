# Copyright (c) 2005-2020 Intel Corporation
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

tbb_root?=..
examples_root:=$(tbb_root)/examples
include $(tbb_root)/build/common.inc

#workaround for non-depend targets tbb and tbbmalloc which both depend on version_string.ver
#According to documentation, recursively invoked make commands can process their targets in parallel
.NOTPARALLEL:

.PHONY: all tbb tbbmalloc test test_no_depends release debug examples clean

all: release debug examples

tbb: tbb_release tbb_debug

tbbmalloc: tbbmalloc_release tbbmalloc_debug

test: tbbmalloc_test_release tbb_test_release tbbmalloc_test_debug tbb_test_debug

test_no_depends: tbbmalloc_test_release_no_depends tbb_test_release_no_depends tbbmalloc_test_debug_no_depends tbb_test_debug_no_depends
	@echo done

release: tbb_release tbbmalloc_release
release: $(call cross_cfg,tbbmalloc_test_release) $(call cross_cfg,test_release)

debug: tbb_debug tbbmalloc_debug
debug: $(call cross_cfg,tbbmalloc_test_debug) $(call cross_cfg, test_debug)

examples: tbb tbbmalloc examples_debug clean_examples examples_release

examples_no_depends: examples_release_no_depends examples_debug_no_depends

clean: clean_release clean_debug clean_examples
	@echo clean done

.PHONY: full
full:
	$(MAKE) -sir --no-print-directory -f Makefile tbb_root=.. clean all
ifeq ($(tbb_os),windows)
	$(MAKE) -sir --no-print-directory -f Makefile tbb_root=.. compiler=icl clean all native_examples
else
	$(MAKE) -sir --no-print-directory -f Makefile tbb_root=.. compiler=icc clean all native_examples
endif
ifeq ($(arch),intel64)
	$(MAKE) -sir --no-print-directory -f Makefile tbb_root=.. arch=ia32 clean all
endif
# it doesn't test compiler=icc arch=ia32 on intel64 systems due to environment settings of icc

native_examples: tbb tbbmalloc
	$(MAKE) -C $(examples_root) -r -f Makefile tbb_root=.. compiler=$(native_compiler) debug test
	$(MAKE) -C $(examples_root) -r -f Makefile tbb_root=.. compiler=$(native_compiler) clean release test

../examples/% examples/%::
	$(MAKE) -C $(examples_root) -r -f Makefile tbb_root=.. $(subst examples/,,$(subst ../,,$@))

debug_%:: cfg:=$(if $(findstring file,$(origin cfg)),debug,$(cfg))
debug_%:: export run_cmd=$(debugger)
debug_malloc_% test_malloc_% debug_ScalableAlloc% test_ScalableAlloc%:: TESTFILE=tbbmalloc
debug_% test_% stress_% time_% perf_%:: TESTFILE?=test
debug_% test_% stress_% time_% perf_%::
	$(MAKE) -C "$(work_dir)_$(cfg)" -r -f $(tbb_root)/build/Makefile.$(TESTFILE) cfg=$(cfg) $(subst .cpp,,$@)

clean_%::
ifeq ($(origin cfg),file)
	@$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.test cfg=release $@
	@$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.test cfg=debug $@
else
	@$(MAKE) -C "$(work_dir)_$(cfg)"  -r -f $(tbb_root)/build/Makefile.test $@
endif

python_%: mkdir_release
	$(MAKE) -C "$(work_dir)_release" -rf $(tbb_root)/python/Makefile $(subst python_,,$@)

.PHONY: test_release test_debug test_release_no_depends test_debug_no_depends
.PHONY: tbb_release tbb_debug tbb_test_release tbb_test_debug tbb_test_release_no_depends tbb_test_debug_no_depends
# do not delete double-space after -C option
tbb_release: mkdir_release
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbb cfg=release

tbb_debug: mkdir_debug
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbb cfg=debug

tbb_test: tbb_test_release tbb_test_debug

tbb_test_release: $(call cross_cfg,tbb_release) tbb_test_release_no_depends
tbb_test_release_no_depends:$(call cross_cfg,mkdir_release)
	$(MAKE) -C "$(call cross_cfg,$(work_dir)_release)"  -r -f $(tbb_root)/build/Makefile.test cfg=release

tbb_test_debug: $(call cross_cfg,tbb_debug) tbb_test_debug_no_depends
tbb_test_debug_no_depends:$(call cross_cfg,mkdir_debug)
	$(MAKE) -C "$(call cross_cfg,$(work_dir)_debug)"  -r -f $(tbb_root)/build/Makefile.test cfg=debug
# backward compatibility
test_release: tbb_test_release
test_debug: tbb_test_debug
test_release_no_depends: tbb_test_release_no_depends
test_debug_no_depends: tbb_test_debug_no_depends

.PHONY: tbbmalloc_release tbbmalloc_debug
.PHONY: tbbmalloc_dll_release tbbmalloc_dll_debug tbbmalloc_proxy_dll_release tbbmalloc_proxy_dll_debug
.PHONY: tbbmalloc_test tbbmalloc_test_release tbbmalloc_test_debug tbbmalloc_test_release_no_depends tbbmalloc_test_debug_no_depends

tbbmalloc_release: mkdir_release
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=release malloc

tbbmalloc_debug: mkdir_debug
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=debug malloc

tbbmalloc_dll_release: mkdir_release
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=release malloc_dll

tbbmalloc_proxy_dll_release: mkdir_release
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=release  malloc_proxy_dll

tbbmalloc_dll_debug: mkdir_debug
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=debug malloc_dll

tbbmalloc_proxy_dll_debug: mkdir_debug
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=debug malloc_proxy_dll

tbbmalloc_test: tbbmalloc_test_release tbbmalloc_test_debug

tbbmalloc_test_release: $(call cross_cfg,tbbmalloc_release) tbbmalloc_test_release_no_depends
tbbmalloc_test_release_no_depends: $(call cross_cfg,mkdir_release)
	$(MAKE) -C "$(call cross_cfg,$(work_dir)_release)"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=release malloc_test_no_depends

tbbmalloc_test_debug: $(call cross_cfg,tbbmalloc_debug) tbbmalloc_test_debug_no_depends
tbbmalloc_test_debug_no_depends: $(call cross_cfg,mkdir_debug)
	$(MAKE) -C "$(call cross_cfg,$(work_dir)_debug)"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=debug malloc_test_no_depends

.PHONY: examples_release examples_debug examples_release_no_depends examples_debug_no_depends

examples_release: tbb_release tbbmalloc_release examples_release_no_depends
examples_release_no_depends:
	$(MAKE) -C $(examples_root) -r -f Makefile tbb_root=.. release test UI=con

examples_debug: tbb_debug tbbmalloc_debug examples_debug_no_depends
examples_debug_no_depends:
	$(MAKE) -C $(examples_root) -r -f Makefile tbb_root=.. debug test UI=con

.PHONY: clean_release clean_debug clean_examples

clean_release:
	$(shell $(RM) $(work_dir)_release$(SLASH)*.* >$(NUL) 2>$(NUL))
	$(shell $(RD) $(work_dir)_release >$(NUL) 2>$(NUL))

clean_debug:
	$(shell $(RM) $(work_dir)_debug$(SLASH)*.* >$(NUL) 2>$(NUL))
	$(shell $(RD) $(work_dir)_debug >$(NUL) 2>$(NUL))

clean_examples:
	$(shell $(MAKE) -s -i -r -C $(examples_root) -f Makefile tbb_root=.. clean >$(NUL) 2>$(NUL))

.PHONY: mkdir_release mkdir_debug codecov do_codecov info tbbvars shell

mkdir_release:
	$(shell $(MD) "$(work_dir)_release" >$(NUL) 2>$(NUL))
	@echo Created $(work_dir)_release directory

mkdir_debug:
	$(shell $(MD) "$(work_dir)_debug" >$(NUL) 2>$(NUL))
	@echo Created $(work_dir)_debug directory

ifeq ($(compiler),$(if $(findstring windows,$(tbb_os)),icl,icc))
codecov: codecov=yes
codecov: do_codecov
	$(MAKE) -C "$(work_dir)_release" -r -f $(tbb_root)/build/Makefile.test cfg=release codecov_gen
else
codecov:
	$(error Only Intel(R) C++ Compiler is supported for code coverage)
endif

export codecov

do_codecov: tbb_root=..
do_codecov:
	$(MAKE) tbbmalloc_test_release test_release
	$(MAKE) clean_test_* cfg=release
	$(MAKE) crosstest=yes tbbmalloc_test_debug test_debug
	$(MAKE) clean_test_* cfg=release
	$(MAKE) clean_test_* cfg=release

info:
	@echo OS: $(tbb_os)
	@echo arch=$(arch)
	@echo compiler=$(compiler)
	@echo runtime=$(runtime)
	@echo tbb_build_prefix=$(tbb_build_prefix)
	@echo work_dir=$(abspath $(tbb_build_dir)$(SLASH)$(tbb_build_prefix)_$(cfg))

# [usage]$ source `make <options> tbbvars`.sh
tbbvars:
	@echo $(tbb_build_dir)$(SLASH)$(tbb_build_prefix)_$(cfg)$(SLASH)tbbvars

symbols: args=$(if $(findstring cl,$(compiler)), dumpbin /section:.text *.obj|findstr COMDAT , nm -Pg *.o|grep ' T '|cut -f1 -d' ')
symbols: shell

shell:
ifdef BUILDING_PHASE
	-$(run_cmd) $(shell_cmd)
else
	@$(MAKE) -C "$(work_dir)_$(cfg)" -rf $(tbb_root)/src/Makefile BUILDING_PHASE=1 shell shell_cmd="$(if $(args),$(args),$(SHELL))"
endif

