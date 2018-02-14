#!/usr/bin/env python
#
# Copyright (c) 2017 Intel Corporation
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

# Provides unified tool for preparing TBB for packaging

from __future__ import print_function
import os
import re
import sys
import shutil
import platform
import argparse
from glob import glob
from collections import OrderedDict

jp = os.path.join
is_win = (platform.system() == 'Windows')
is_lin = (platform.system() == 'Linux')
is_mac = (platform.system() == 'Darwin')

default_prefix = os.getenv('PREFIX', 'install_prefix')
if is_win:
    default_prefix = jp(default_prefix, 'Library') # conda-specific by default on Windows

parser = argparse.ArgumentParser()
parser.add_argument('--tbbroot',       default='.', help='Take Intel TBB from here')
parser.add_argument('--prefix',        default=default_prefix, help='Prefix')
parser.add_argument('--no-rebuild',    default=False, action='store_true', help='do not rebuild')
parser.add_argument('--install',       default=False, action='store_true', help='install all')
parser.add_argument('--install-libs',  default=False, action='store_true', help='install libs')
parser.add_argument('--install-devel', default=False, action='store_true', help='install devel')
parser.add_argument('--install-docs',  default=False, action='store_true', help='install docs')
parser.add_argument('--install-python',default=False, action='store_true', help='install python module')
parser.add_argument('--make-tool',     default='make', help='Use different make command instead')
parser.add_argument('--copy-tool',     default=None, help='Use this command for copying ($ tool file dest-dir)')
parser.add_argument('--build-args',    default="", help='specify extra build args')
parser.add_argument('--build-prefix',  default='local', help='build dir prefix')
if is_win:
    parser.add_argument('--msbuild',   default=False, action='store_true', help='Use msbuild')
    parser.add_argument('--vs',          default="2012", help='select VS version for build')
    parser.add_argument('--vs-platform', default="x64",  help='select VS platform for build')
parser.add_argument('ignore', nargs='?', help="workaround conda-build issue #2512")

args = parser.parse_args()

if args.install:
    args.install_libs  = True
    args.install_devel = True
    args.install_docs  = True
    args.install_python= True

def custom_cp(src, dst):
     assert os.system(' '.join([args.copy_tool, src, dst])) == 0

if args.copy_tool:
    install_cp = custom_cp # e.g. to use install -p -D -m 755 on Linux
else:
    install_cp = shutil.copy

bin_dir = jp(args.prefix, "bin")
lib_dir = jp(args.prefix, "lib")
inc_dir = jp(args.prefix, 'include')
doc_dir = jp(args.prefix, 'share', 'doc', 'tbb')
if is_win:
    os.environ["OS"] = "Windows_NT" # make sure TBB will interpret it corretly
    libext = '.dll'
    libpref = ''
    dll_dir = bin_dir
else:
    libext = '.dylib' if is_mac else '.so.2'
    libpref = 'lib'
    dll_dir = lib_dir

tbb_names = ["tbb", "tbbmalloc", "tbbmalloc_proxy"]

##############################################################

os.chdir(args.tbbroot)
if is_win and args.msbuild:
    preview_release_dir = release_dir = jp(args.tbbroot, 'build', 'vs'+args.vs, args.vs_platform, 'Release')
    if not args.no_rebuild or not os.path.isdir(release_dir):
        assert os.system('msbuild /m /p:Platform=%s /p:Configuration=Release %s build/vs%s/makefile.sln'% \
                        (args.vs_platform, args.build_args, args.vs)) == 0
    preview_debug_dir = debug_dir = jp(args.tbbroot, 'build', 'vs'+args.vs, args.vs_platform, 'Debug')
    if not args.no_rebuild or not os.path.isdir(debug_dir):
        assert os.system('msbuild /m /p:Platform=%s /p:Configuration=Debug %s build/vs%s/makefile.sln'% \
                        (args.vs_platform, args.build_args, args.vs)) == 0
else:
    release_dir = jp(args.tbbroot, 'build', args.build_prefix+'_release')
    debug_dir = jp(args.tbbroot, 'build', args.build_prefix+'_debug')
    if not args.no_rebuild or not (os.path.isdir(release_dir) and os.path.isdir(debug_dir)):
        assert os.system('%s -j tbb_build_prefix=%s %s'% \
                        (args.make_tool, args.build_prefix, args.build_args)) == 0
    preview_release_dir = jp(args.tbbroot, 'build', args.build_prefix+'_preview_release')
    preview_debug_dir = jp(args.tbbroot, 'build', args.build_prefix+'_preview_debug')
    if not args.no_rebuild or not (os.path.isdir(preview_release_dir) and os.path.isdir(preview_debug_dir)):
        assert os.system('%s -j tbb_build_prefix=%s_preview %s tbb_cpf=1 tbb'% \
                        (args.make_tool, args.build_prefix, args.build_args)) == 0


filemap = OrderedDict()
def append_files(files, dst):
    global filemap
    filemap.update(dict(zip(files, [dst]*len(files))))

if args.install_libs:
    files = [jp(release_dir, libpref+f+libext) for f in tbb_names]
    append_files(files, dll_dir)

if args.install_devel:
    dll_files = [jp(debug_dir, libpref+f+'_debug'+libext) for f in tbb_names] # adding debug libraries
    if not is_win or not args.msbuild:
        dll_files += [jp(preview_release_dir, libpref+"tbb_preview"+libext),
                      jp(preview_debug_dir, libpref+"tbb_preview_debug"+libext)]
    if is_win:
        dll_files += sum( [glob(jp(d, 'tbb*.pdb')) for d in             # copying debug info
                          (release_dir, debug_dir, preview_release_dir, preview_debug_dir)], [])
    if is_lin:
        dll_files += sum( [glob(jp(d, 'libtbb*.so')) for d in           # copying linker scripts
                          (release_dir, debug_dir, preview_release_dir, preview_debug_dir)], [])
        # symlinks .so -> .so.2 should not be created instead
        # since linking with -ltbb when using links can result in
        # incorrect dependence upon unversioned .so files
    append_files(dll_files, dll_dir)
    if is_win:
        lib_files = sum([glob(jp(d,e)) for d in (release_dir, debug_dir) for e in ('*.lib', '*.def')], [])
        append_files(lib_files, lib_dir) # copying linker libs and defs
    for rootdir, dirnames, filenames in os.walk(jp(args.tbbroot,'include')):
        files = [jp(rootdir, f) for f in filenames if not '.html' in f]
        append_files(files, jp(inc_dir, rootdir.split('include')[1][1:]))

if args.install_python:
    assert os.system('%s -j tbb_build_prefix=%s %s python'% \
                    (args.make_tool, args.build_prefix, args.build_args)) == 0
    if is_lin:
        append_files([jp(release_dir, 'libirml.so.1')], dll_dir)

if args.install_docs:
    files = [jp(args.tbbroot, *f) for f in (
            ('CHANGES',),
            ('LICENSE',),
            ('README',),
            ('README.md',),
            ('doc','Release_Notes.txt'),
            )]
    append_files(files, doc_dir)

for f in filemap.keys():
    assert os.path.exists(f)
    assert os.path.isfile(f)

if filemap:
    print("Copying to prefix =", args.prefix)
for f, dest in filemap.items():
    if not os.path.isdir(dest):
        os.makedirs(dest)
    print("+ %s to $prefix%s"%(f,dest.replace(args.prefix, '')))
    install_cp(f, dest)

print("done")
