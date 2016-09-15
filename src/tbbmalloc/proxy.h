/*
    Copyright 2005-2015 Intel Corporation.  All Rights Reserved.

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

#ifndef _TBB_malloc_proxy_H_
#define _TBB_malloc_proxy_H_

#define MALLOC_UNIXLIKE_OVERLOAD_ENABLED __linux__
#define MALLOC_ZONE_OVERLOAD_ENABLED __APPLE__

// MALLOC_UNIXLIKE_OVERLOAD_ENABLED depends on MALLOC_CHECK_RECURSION stuff
// TODO: limit MALLOC_CHECK_RECURSION to *_OVERLOAD_ENABLED only
#if __linux__ || __APPLE__ || __sun || __FreeBSD__ || MALLOC_UNIXLIKE_OVERLOAD_ENABLED
#define MALLOC_CHECK_RECURSION 1
#endif

#include <stddef.h>

extern "C" {
    void * scalable_malloc(size_t size);
    void * scalable_calloc(size_t nobj, size_t size);
    void   scalable_free(void *ptr);
    void * scalable_realloc(void* ptr, size_t size);
    void * scalable_aligned_malloc(size_t size, size_t alignment);
    void * scalable_aligned_realloc(void* ptr, size_t size, size_t alignment);
    int    scalable_posix_memalign(void **memptr, size_t alignment, size_t size);
    size_t scalable_msize(void *ptr);
    void   __TBB_malloc_safer_free( void *ptr, void (*original_free)(void*));
    void * __TBB_malloc_safer_realloc( void *ptr, size_t, void* );
    void * __TBB_malloc_safer_aligned_realloc( void *ptr, size_t, size_t, void* );
    size_t __TBB_malloc_safer_msize( void *ptr, size_t (*orig_msize_crt80d)(void*));
    size_t __TBB_malloc_safer_aligned_msize( void *ptr, size_t, size_t, size_t (*orig_msize_crt80d)(void*,size_t,size_t));

#if MALLOC_ZONE_OVERLOAD_ENABLED
    void   __TBB_malloc_free_definite_size(void *object, size_t size);
#endif
} // extern "C"

// Struct with original free() and _msize() pointers
struct orig_ptrs {
    void   (*orig_free) (void*);  
    size_t (*orig_msize)(void*); 
};

#endif /* _TBB_malloc_proxy_H_ */
