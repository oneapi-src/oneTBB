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

tbb_root?=.
cfg?=release
include $(tbb_root)/build/common.inc
.PHONY: default all tbb tbbmalloc tbbproxy test examples

#workaround for non-depend targets tbb and tbbmalloc which both depend on version_string.ver
#According to documentation, recursively invoked make commands can process their targets in parallel
.NOTPARALLEL: tbb tbbmalloc tbbproxy

default: tbb tbbmalloc $(if $(use_proxy),tbbproxy)

all: tbb tbbmalloc tbbproxy test examples

tbb: mkdir
	$(MAKE) -C "$(work_dir)_$(cfg)"  -r -f $(tbb_root)/build/Makefile.tbb cfg=$(cfg)

tbbmalloc: mkdir
	$(MAKE) -C "$(work_dir)_$(cfg)"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=$(cfg) malloc

tbbproxy: mkdir
	$(MAKE) -C "$(work_dir)_$(cfg)"  -r -f $(tbb_root)/build/Makefile.tbbproxy cfg=$(cfg) tbbproxy

tbbbind: mkdir
	$(MAKE) -C "$(work_dir)_$(cfg)"  -r -f $(tbb_root)/build/Makefile.tbbbind cfg=$(cfg) tbbbind

test: tbb tbbmalloc $(if $(use_proxy),tbbproxy)
	-$(MAKE) -C "$(work_dir)_$(cfg)"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=$(cfg) malloc_test
	-$(MAKE) -C "$(work_dir)_$(cfg)"  -r -f $(tbb_root)/build/Makefile.test cfg=$(cfg)

rml: mkdir
	$(MAKE) -C "$(work_dir)_$(cfg)"  -r -f $(tbb_root)/build/Makefile.rml cfg=$(cfg)

examples: tbb tbbmalloc
	$(MAKE) -C examples -r -f Makefile tbb_root=.. $(cfg) test

python: tbb
	$(MAKE) -C "$(work_dir)_$(cfg)" -rf $(tbb_root)/python/Makefile install

doxygen:
	doxygen Doxyfile

.PHONY: clean clean_examples mkdir info

clean: clean_examples
	$(shell $(RM) $(work_dir)_$(cfg)$(SLASH)*.* >$(NUL) 2>$(NUL))
	$(shell $(RD) $(work_dir)_$(cfg) >$(NUL) 2>$(NUL))
	@echo clean done

clean_examples:
	$(shell $(MAKE) -s -i -r -C examples -f Makefile tbb_root=.. clean >$(NUL) 2>$(NUL))

mkdir:
	$(shell $(MD) "$(work_dir)_$(cfg)" >$(NUL) 2>$(NUL))
	@echo Created the $(work_dir)_$(cfg) directory

info:
	@echo OS: $(tbb_os)
	@echo arch=$(arch)
	@echo compiler=$(compiler)
	@echo runtime=$(runtime)
	@echo tbb_build_prefix=$(tbb_build_prefix)
