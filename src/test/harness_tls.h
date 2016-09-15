/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

class LimitTLSKeysTo {
#if _WIN32 || _WIN64
#if __TBB_WIN8UI_SUPPORT
    #define TlsAlloc() FlsAlloc(NULL)
    #define TlsFree FlsFree
    #define TLS_OUT_OF_INDEXES FLS_OUT_OF_INDEXES
#endif
    typedef DWORD handle;
#else
    typedef pthread_key_t handle;
#endif
    // for platforms that not limit number of TLS keys, set artifical limit
    static const int LIMIT = 16*1024;
    handle handles[LIMIT];
    int    lastUsedIdx;
public:
    LimitTLSKeysTo(int keep_keys) {
        for (lastUsedIdx=0; lastUsedIdx<LIMIT; lastUsedIdx++) {
#if _WIN32 || _WIN64
            handle h = TlsAlloc();
            if (h==TLS_OUT_OF_INDEXES)
#else
            int setspecific_dummy=10;
            if (pthread_key_create(&handles[lastUsedIdx], NULL)!=0)
#endif
            {
                break;
            }
#if _WIN32 || _WIN64
            handles[lastUsedIdx] = h;
#else
            pthread_setspecific(handles[lastUsedIdx], &setspecific_dummy);
#endif
        }
        lastUsedIdx--;
        ASSERT(lastUsedIdx >= keep_keys-1, "Less TLS keys are available than requested");
        for (; keep_keys>0; keep_keys--, lastUsedIdx--) {
#if _WIN32 || _WIN64
            TlsFree(handles[lastUsedIdx]);
#else
            int ret = pthread_key_delete(handles[lastUsedIdx]);
            ASSERT(!ret, "Can't delete a key");
#endif
        }
        REMARK("%d thread local objects allocated in advance\n", lastUsedIdx+1);
    }
    ~LimitTLSKeysTo() {
        for (int i=0; i<=lastUsedIdx; i++) {
#if _WIN32 || _WIN64
            TlsFree(handles[i]);
#else
            int ret = pthread_key_delete(handles[i]);
            ASSERT(!ret, "Can't delete a key");
#endif
        }
        lastUsedIdx = 0;
    }
};
