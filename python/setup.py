#!/usr/bin/env python
#
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


# System imports
from __future__ import print_function
import platform
import os

from distutils.core import *
from distutils.command.build import build

if any(i in os.environ for i in ["CC", "CXX"]):
    if "CC" not in os.environ:
        os.environ['CC'] = os.environ['CXX']
    if "CXX" not in os.environ:
        os.environ['CXX'] = os.environ['CC']
    if platform.system() == 'Linux':
        os.environ['LDSHARED'] = os.environ['CXX'] + " -shared"

intel_compiler = os.getenv('CC', '') in ['icl', 'icpc', 'icc']
try:
    tbb_root = os.environ['TBBROOT']
    print("Using TBBROOT=", tbb_root)
except:
    tbb_root = '.'
    if not intel_compiler:
        print("Warning: TBBROOT env var is not set and Intel's compiler is not used. It might lead\n"
              "    !!!: to compile/link problems. Source tbbvars.sh/.csh file to set environment")
use_compiler_tbb = intel_compiler and tbb_root == '.'
if use_compiler_tbb:
    print("Using Intel TBB from Intel's compiler")
if platform.system() == 'Windows':
    if intel_compiler:
        os.environ['DISTUTILS_USE_SDK'] = '1'  # Enable environment settings in distutils
        os.environ['MSSdk'] = '1'
        print("Using compiler settings from environment")
    tbb_flag = ['/Qtbb'] if use_compiler_tbb else []
    compile_flags = ['/Qstd=c++11'] if intel_compiler else []
else:
    tbb_flag = ['-tbb'] if use_compiler_tbb else []
    compile_flags = ['-std=c++11', '-Wno-unused-variable']

_tbb = Extension("_TBB", ["tbb.i"],
        include_dirs=[os.path.join(tbb_root, 'include')] if not use_compiler_tbb else [],
        swig_opts   =['-c++', '-O', '-threads'] + (  # add '-builtin' later
              ['-I' + os.path.join(tbb_root, 'include')] if not use_compiler_tbb else []),
        extra_compile_args=compile_flags + tbb_flag,
        extra_link_args=tbb_flag,
        libraries   =['tbb'] if not use_compiler_tbb else [],
        library_dirs=[os.path.join(tbb_root, 'lib', 'intel64', 'gcc4.4'),  # for Linux
                      os.path.join(tbb_root, 'lib'),                       # for MacOS
                      os.path.join(tbb_root, 'lib', 'intel64', 'vc_mt'),   # for Windows
                     ] if not use_compiler_tbb else [],
        language    ='c++',
        )


class TBBBuild(build):
    sub_commands = [  # define build order
        ('build_ext', build.has_ext_modules),
        ('build_py', build.has_pure_modules),
    ]


setup(  name        ="TBB",
        description ="Python API for Intel TBB",
        long_description="Python API to Intel(R) Threading Building Blocks library (Intel(R) TBB) "
                         "extended with standard Pool implementation and monkey-patching",
        url         ="https://www.threadingbuildingblocks.org/",
        author      ="Intel Corporation",
        author_email="Anton.Malakhov@intel.com",
        license     ="BSD License",
        version     ="0.1",
        classifiers =[
            'Development Status :: 4 - Beta',
            'Environment :: Console',
            'Environment :: Plugins',
            'Intended Audience :: Developers',
            'Intended Audience :: System Administrators',
            'Intended Audience :: Other Audience',
            'Intended Audience :: Science/Research',
            'License :: OSI Approved :: BSD License',
            'Operating System :: MacOS :: MacOS X',
            'Operating System :: Microsoft :: Windows',
            'Operating System :: POSIX',
            'Operating System :: Unix',
            'Programming Language :: Python',
            'Programming Language :: Python :: 2',
            'Programming Language :: Python :: 3',
            'Programming Language :: C++',
            'Topic :: System :: Hardware :: Symmetric Multi-processing',
            'Topic :: Software Development :: Libraries',
          ],
        keywords='tbb multiprocessing multithreading composable parallelism',
        ext_modules=[_tbb],
        py_modules=['TBB'],
        cmdclass={'build': TBBBuild}
)
