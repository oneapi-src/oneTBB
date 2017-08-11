#
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

from __future__ import print_function

import multiprocessing.pool
import ctypes
import atexit
import sys
import os
 
from .api import  *
from .api import __all__ as api__all
from .pool import *
from .pool import __all__ as pool__all

__all__ = ["Monkey"] + api__all + pool__all

__doc__ = """
Python API for Intel(R) Threading Building Blocks library (Intel(R) TBB)
extended with standard Python's pools implementation and monkey-patching.

Command-line interface example:
$  python -m tbb $your_script.py
Runs your_script.py in context of tbb.Monkey
"""


ipc_enabled = False
libirml = "libirml.so.1"


def _test(arg=None):
    """Some tests"""
    import platform
    if platform.system() == "Linux":
        ctypes.CDLL(libirml)
    from .test import test
    test(arg)
    print("done")


def tbb_process_pool_worker27(inqueue, outqueue, initializer=None, initargs=(),
                            maxtasks=None):
    from multiprocessing.pool import worker
    worker(inqueue, outqueue, initializer, initargs, maxtasks)
    if ipc_enabled:
        try:
            librml = ctypes.CDLL(libirml)
            librml.release_resources()
        except:
            print("Warning: Can not load ", libirml, file=sys.stderr)


class TBBProcessPool27(multiprocessing.pool.Pool):
    def _repopulate_pool(self):
        """Bring the number of pool processes up to the specified number,
        for use after reaping workers which have exited.
        """
        from multiprocessing.util import debug

        for i in range(self._processes - len(self._pool)):
            w = self.Process(target=tbb_process_pool_worker27,
                             args=(self._inqueue, self._outqueue,
                                   self._initializer,
                                   self._initargs, self._maxtasksperchild)
                            )
            self._pool.append(w)
            w.name = w.name.replace('Process', 'PoolWorker')
            w.daemon = True
            w.start()
            debug('added worker')

    def __del__(self):
        self.close()
        for p in self._pool:
            p.join()

    def __exit__(self):
        self.close()
        for p in self._pool:
            p.join()


def tbb_process_pool_worker35(inqueue, outqueue, initializer=None, initargs=(),
                            maxtasks=None, wrap_exception=False):
    from multiprocessing.pool import worker
    worker(inqueue, outqueue, initializer, initargs, maxtasks, wrap_exception)
    if ipc_enabled:
        try:
            librml = ctypes.CDLL(libirml)
            librml.release_resources()
        except:
            print("Warning: Can not load ", libirml, file=sys.stderr)


class TBBProcessPool35(multiprocessing.pool.Pool):
    def _repopulate_pool(self):
        """Bring the number of pool processes up to the specified number,
        for use after reaping workers which have exited.
        """
        from multiprocessing.util import debug

        for i in range(self._processes - len(self._pool)):
            w = self.Process(target=tbb_process_pool_worker35,
                             args=(self._inqueue, self._outqueue,
                                   self._initializer,
                                   self._initargs, self._maxtasksperchild,
                                   self._wrap_exception)
                            )
            self._pool.append(w)
            w.name = w.name.replace('Process', 'PoolWorker')
            w.daemon = True
            w.start()
            debug('added worker')

    def __del__(self):
        self.close()
        for p in self._pool:
            p.join()

    def __exit__(self):
        self.close()
        for p in self._pool:
            p.join()


class Monkey:
    """
    Context manager which replaces standard multiprocessing.pool
    implementations with tbb.pool using monkey-patching. It also enables TBB
    threading for Intel(R) Math Kernel Library (Intel(R) MKL). For example:

        with tbb.Monkey():
            run_my_numpy_code()

    It allows multiple parallel tasks to be executed on the same thread pool
    and coordinate number of threads across multiple processes thus avoiding
    overheads from oversubscription.
    """
    _items   = {'ThreadPool'  : None,
                'Pool' : None}
    _modules = {'ThreadPool'  : None,
                'Pool' : None}

    def __init__(self, max_num_threads=None):
        """
        Create context manager for running under TBB scheduler.
        :param max_num_threads: if specified, limits maximal number of threads
        """
        if max_num_threads:
            self.ctl = global_control(global_control.max_allowed_parallelism, int(max_num_threads))

    def _patch(self, class_name, module_name, object):
        self._modules[class_name] = __import__(module_name, globals(),
                                               locals(), [class_name])
        if self._modules[class_name] == None:
            return
        oldattr = getattr(self._modules[class_name], class_name, None)
        if oldattr == None:
            self._modules[class_name] = None
            return
        self._items[class_name] = oldattr
        setattr(self._modules[class_name], class_name, object)

    def __enter__(self):
        self.env = os.getenv('MKL_THREADING_LAYER')
        os.environ['MKL_THREADING_LAYER'] = 'TBB'
        if sys.version_info.major == 2 and sys.version_info.minor >= 7:
            self._patch("Pool", "multiprocessing.pool", TBBProcessPool27)
        elif sys.version_info.major == 3 and sys.version_info.minor >= 5:
            self._patch("Pool", "multiprocessing.pool", TBBProcessPool35)
        self._patch("ThreadPool", "multiprocessing.pool", Pool)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self.env is None:
            del os.environ['MKL_THREADING_LAYER']
        else:
            os.environ['MKL_THREADING_LAYER'] = self.env
        for name in self._items.keys():
            setattr(self._modules[name], name, self._items[name])


def init_sem_name():
    try:
        librml = ctypes.CDLL(libirml)
        librml.set_active_sem_name()
        librml.set_stop_sem_name()
    except Exception as e:
        print("Warning: Can not initialize name of shared semaphores:", e,
              file=sys.stderr)


def tbb_atexit():
    if ipc_enabled:
        try:
            librml = ctypes.CDLL(libirml)
            librml.release_semaphores()
        except:
            print("Warning: Can not release shared semaphores",
                  file=sys.stderr)


def _main():
    # Run the module specified as the next command line argument
    # python -m TBB user_app.py
    global ipc_enabled

    import platform
    import argparse
    parser = argparse.ArgumentParser(prog="python -m tbb", description="""
                Run your Python script in context of tbb.Monkey, which
                replaces standard Python pools and threading layer of
                Intel(R) Math Kernel Library by implementation based on
                Intel(R) Threading Building Blocks. It enables multiple parallel
                tasks to be executed on the same thread pool and coordinate
                number of threads across multiple processes thus avoiding
                overheads from oversubscription.
             """, formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    if platform.system() == "Linux":
        parser.add_argument('--ipc', action='store_true',
                        help="Enable inter-precess (IPC) synchronization")
    parser.add_argument('-p', '--max-num-threads', default=default_num_threads(), type=int,
                        help="Initialize TBB with P max number of threads per process", metavar='P')
    #parser.add_argument('-a', '--replace-allocator', action='store_true',
    #                    help="Enable TBB malloc")
    #parser.add_argument('--huge-pages', action='store_true',
    #                    help="Enable huge pages for TBB malloc")
    parser.add_argument('-m', action='store_true', dest='module',
                        help="Executes following as a module")
    parser.add_argument('name', help="Script or module name")
    parser.add_argument('args', nargs=argparse.REMAINDER,
                        help="Command line arguments")
    args = parser.parse_args()
    sys.argv = [args.name] + args.args

    ipc_enabled = platform.system() == "Linux" and args.ipc
    os.environ["IPC_ENABLE"] = "1" if ipc_enabled else "0"
    if ipc_enabled:
        atexit.register(tbb_atexit)
        init_sem_name()

    if not os.environ.get("KMP_BLOCKTIME"): # TODO move
        os.environ["KMP_BLOCKTIME"] = "0"
    if '_' + args.name in globals():
        return globals()['_' + args.name](*args.args)
    else:
        import runpy
        runf = runpy.run_module if args.module else runpy.run_path
        with Monkey(max_num_threads=args.max_num_threads):
            runf(args.name, run_name='__main__')
