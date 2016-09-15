%pythonbegin %{
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


# Based on the software developed by:
# Copyright (c) 2008,2016 david decotigny (Pool of threads)
# Copyright (c) 2006-2008, R Oudkerk (multiprocessing.Pool)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of author nor the names of any contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

from __future__ import print_function
%}
%begin %{
/* Defines Python wrappers for Intel(R) Threading Building Blocks (Intel TBB).*/
%}
%module TBB

#if SWIG_VERSION < 0x030001
#error SWIG version 3.0.6 or newer is required for correct functioning
#endif

%{
#include <tbb/tbb.h>
using namespace tbb;

class PyCaller : public swig::SwigPtr_PyObject {
public:
    // icpc 2013 does not support simple using SwigPtr_PyObject::SwigPtr_PyObject;
    PyCaller(const PyCaller& s) : SwigPtr_PyObject(s) {}
    PyCaller(PyObject *p, bool initial = true) : SwigPtr_PyObject(p, initial) {}

    void operator()() const {
        SWIG_PYTHON_THREAD_BEGIN_BLOCK;
        PyObject* r = PyObject_CallFunctionObjArgs((PyObject*)*this, NULL);
        if(r) Py_DECREF(r);
        SWIG_PYTHON_THREAD_END_BLOCK;
    }
};

struct ArenaPyCaller {
    task_arena *my_arena;
    PyObject *my_callable;
    ArenaPyCaller(task_arena *a, PyObject *c) : my_arena(a), my_callable(c) {
        SWIG_PYTHON_THREAD_BEGIN_BLOCK;
        Py_XINCREF(c);
        SWIG_PYTHON_THREAD_END_BLOCK;
    }
    void operator()() const {
        my_arena->execute(PyCaller(my_callable, false));
    }
};

%}

namespace tbb {
    class task_scheduler_init {
    public:
        //! Typedef for number of threads that is automatic.
        static const int automatic = -1;
        //! Argument to initialize() or constructor that causes initialization to be deferred.
        static const int deferred = -2;
        task_scheduler_init( int max_threads=automatic, 
                             size_t thread_stack_size=0 );
        ~task_scheduler_init();
        void initialize( int max_threads=automatic );
        void terminate();
        static int default_num_threads();
        bool is_active() const;
    };

    class task_arena {
    public:
        static const int automatic = -1;
        static int current_thread_index();
        task_arena(int max_concurrency = automatic, unsigned reserved_for_masters = 1);
        task_arena(const task_arena &s);
        ~task_arena();
        void initialize();
        void initialize(int max_concurrency, unsigned reserved_for_masters = 1);
        void terminate();
        bool is_active();
        %extend {
        void enqueue( PyObject *c ) { $self->enqueue(PyCaller(c)); }
        void execute( PyObject *c ) { $self->execute(PyCaller(c)); }
        };
    };

    class task_group {
    public:
        task_group();
        ~task_group();
        void wait(); 
        bool is_canceling();
        void cancel();
        %extend {
        void run( PyObject *c ) { $self->run(PyCaller(c)); }
        void run( PyObject *c, task_arena *a ) { $self->run(ArenaPyCaller(a, c)); }
        };
    };

}

// Python part of the module
%pythoncode "tbb.src.py"
