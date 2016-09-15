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

#include "tbb/scalable_allocator.h"
#include "tbb/atomic.h"
#define HARNESS_TBBMALLOC_THREAD_SHUTDOWN 1
#include "harness.h"
#include "harness_barrier.h"
#if !__TBB_SOURCE_DIRECTLY_INCLUDED
#include "harness_tbb_independence.h"
#endif

template<typename T>
static inline T alignUp  (T arg, uintptr_t alignment) {
    return T(((uintptr_t)arg+(alignment-1)) & ~(alignment-1));
}

struct PoolSpace: NoCopy {
    size_t pos;
    int    regions;
    size_t bufSize;
    char  *space;

    static const size_t BUF_SIZE = 8*1024*1024;

    PoolSpace(size_t bufSz = BUF_SIZE) :
        pos(0), regions(0),
        bufSize(bufSz), space(new char[bufSize]) {
        memset(space, 0, bufSize);
    }
    ~PoolSpace() {
        delete []space;
    }
};

static PoolSpace *poolSpace;

struct MallocPoolHeader {
    void  *rawPtr;
    size_t userSize;
};

static tbb::atomic<int> liveRegions;

static void *getMallocMem(intptr_t /*pool_id*/, size_t &bytes)
{
    void *rawPtr = malloc(bytes+sizeof(MallocPoolHeader)+1);
    if (!rawPtr)
        return NULL;
    // +1 to check working with unaligned space
    void *ret = (void *)((uintptr_t)rawPtr+sizeof(MallocPoolHeader)+1);

    MallocPoolHeader *hdr = (MallocPoolHeader*)ret-1;
    hdr->rawPtr = rawPtr;
    hdr->userSize = bytes;

    liveRegions++;

    return ret;
}

static int putMallocMem(intptr_t /*pool_id*/, void *ptr, size_t bytes)
{
    MallocPoolHeader *hdr = (MallocPoolHeader*)ptr-1;
    ASSERT(bytes == hdr->userSize, "Invalid size in pool callback.");
    free(hdr->rawPtr);

    liveRegions--;

    return 0;
}

void TestPoolReset()
{
    rml::MemPoolPolicy pol(getMallocMem, putMallocMem);
    rml::MemoryPool *pool;

    pool_create_v1(0, &pol, &pool);
    for (int i=0; i<100; i++) {
        ASSERT(pool_malloc(pool, 8), NULL);
        ASSERT(pool_malloc(pool, 50*1024), NULL);
    }
    int regionsBeforeReset = liveRegions;
    bool ok = pool_reset(pool);
    ASSERT(ok, NULL);
    for (int i=0; i<100; i++) {
        ASSERT(pool_malloc(pool, 8), NULL);
        ASSERT(pool_malloc(pool, 50*1024), NULL);
    }
    ASSERT(regionsBeforeReset == liveRegions,
           "Expected no new regions allocation.");
    ok = pool_destroy(pool);
    ASSERT(ok, NULL);
    ASSERT(!liveRegions, "Expected all regions were released.");
}

class SharedPoolRun: NoAssign {
    static long                 threadNum;
    static Harness::SpinBarrier startB,
                                mallocDone;
    static rml::MemoryPool     *pool;
    static void               **crossThread,
                              **afterTerm;
public:
    static const int OBJ_CNT = 100;

    static void init(int num, rml::MemoryPool *pl, void **crThread, void **aTerm) {
        threadNum = num;
        pool = pl;
        crossThread = crThread;
        afterTerm = aTerm;
        startB.initialize(threadNum);
        mallocDone.initialize(threadNum);
    }

    void operator()( int id ) const {
        const int ITERS = 1000;
        void *local[ITERS];

        startB.wait();
        for (int i=id*OBJ_CNT; i<(id+1)*OBJ_CNT; i++) {
            afterTerm[i] = pool_malloc(pool, i%2? 8*1024 : 9*1024);
            memset(afterTerm[i], i, i%2? 8*1024 : 9*1024);
            crossThread[i] = pool_malloc(pool, i%2? 9*1024 : 8*1024);
            memset(crossThread[i], i, i%2? 9*1024 : 8*1024);
        }

        for (int i=1; i<ITERS; i+=2) {
            local[i-1] = pool_malloc(pool, 6*1024);
            memset(local[i-1], i, 6*1024);
            local[i] = pool_malloc(pool, 16*1024);
            memset(local[i], i, 16*1024);
        }
        mallocDone.wait();
        int myVictim = threadNum-id-1;
        for (int i=myVictim*OBJ_CNT; i<(myVictim+1)*OBJ_CNT; i++)
            pool_free(pool, crossThread[i]);
        for (int i=0; i<ITERS; i++)
            pool_free(pool, local[i]);
    }
};

long                 SharedPoolRun::threadNum;
Harness::SpinBarrier SharedPoolRun::startB,
                     SharedPoolRun::mallocDone;
rml::MemoryPool     *SharedPoolRun::pool;
void               **SharedPoolRun::crossThread,
                   **SharedPoolRun::afterTerm;

// single pool shared by different threads
void TestSharedPool()
{
    rml::MemPoolPolicy pol(getMallocMem, putMallocMem);
    rml::MemoryPool *pool;

    pool_create_v1(0, &pol, &pool);
    void **crossThread = new void*[MaxThread * SharedPoolRun::OBJ_CNT];
    void **afterTerm = new void*[MaxThread * SharedPoolRun::OBJ_CNT];

    for (int p=MinThread; p<=MaxThread; p++) {
        SharedPoolRun::init(p, pool, crossThread, afterTerm);
        SharedPoolRun thr;

        void *hugeObj = pool_malloc(pool, 10*1024*1024);
        ASSERT(hugeObj, NULL);

        NativeParallelFor( p, thr );

        pool_free(pool, hugeObj);
        for (int i=0; i<p*SharedPoolRun::OBJ_CNT; i++)
            pool_free(pool, afterTerm[i]);
    }
    delete []afterTerm;
    delete []crossThread;

    bool ok = pool_destroy(pool);
    ASSERT(ok, NULL);
    ASSERT(!liveRegions, "Expected all regions were released.");
}

void *CrossThreadGetMem(intptr_t pool_id, size_t &bytes)
{
    if (poolSpace[pool_id].pos + bytes > poolSpace[pool_id].bufSize)
        return NULL;

    void *ret = poolSpace[pool_id].space + poolSpace[pool_id].pos;
    poolSpace[pool_id].pos += bytes;
    poolSpace[pool_id].regions++;

    return ret;
}

int CrossThreadPutMem(intptr_t pool_id, void* /*raw_ptr*/, size_t /*raw_bytes*/)
{
    poolSpace[pool_id].regions--;
    return 0;
}

class CrossThreadRun: NoAssign {
    static long number_of_threads;
    static Harness::SpinBarrier barrier;
    static rml::MemoryPool **pool;
    static char **obj;
public:
    static void initBarrier(unsigned thrds) { barrier.initialize(thrds); }
    static void init(long num) {
        number_of_threads = num;
        pool = new rml::MemoryPool*[number_of_threads];
        poolSpace = new PoolSpace[number_of_threads];
        obj = new char*[number_of_threads];
    }
    static void destroy() {
        for (long i=0; i<number_of_threads; i++)
            ASSERT(!poolSpace[i].regions, "Memory leak detected");
        delete []pool;
        delete []poolSpace;
        delete []obj;
    }
    CrossThreadRun() {}
    void operator()( int id ) const {
        rml::MemPoolPolicy pol(CrossThreadGetMem, CrossThreadPutMem);
        const int objLen = 10*id;

        pool_create_v1(id, &pol, &pool[id]);
        obj[id] = (char*)pool_malloc(pool[id], objLen);
        ASSERT(obj[id], NULL);
        memset(obj[id], id, objLen);

        {
            const size_t lrgSz = 2*16*1024;
            void *ptrLarge = pool_malloc(pool[id], lrgSz);
            ASSERT(ptrLarge, NULL);
            memset(ptrLarge, 1, lrgSz);

            // consume all small objects
            while (pool_malloc(pool[id], 5*1024))
                ;
            // releasing of large object can give a chance to allocate more
            pool_free(pool[id], ptrLarge);

            ASSERT(pool_malloc(pool[id], 5*1024), NULL);
        }

        barrier.wait();
        int myPool = number_of_threads-id-1;
        for (int i=0; i<10*myPool; i++)
            ASSERT(myPool==obj[myPool][i], NULL);
        pool_free(pool[myPool], obj[myPool]);
        bool ok = pool_destroy(pool[myPool]);
        ASSERT(ok, NULL);
    }
};

long CrossThreadRun::number_of_threads;
Harness::SpinBarrier CrossThreadRun::barrier;
rml::MemoryPool **CrossThreadRun::pool;
char **CrossThreadRun::obj;

// pools created, used and destored by different threads
void TestCrossThreadPools()
{
    for (int p=MinThread; p<=MaxThread; p++) {
        CrossThreadRun::initBarrier(p);
        CrossThreadRun::init(p);
        NativeParallelFor( p, CrossThreadRun() );
        for (int i=0; i<p; i++)
            ASSERT(!poolSpace[i].regions, "Region leak detected");
        CrossThreadRun::destroy();
    }
}

// buffer is too small to pool be created, but must not leak resourses
void TestTooSmallBuffer()
{
    poolSpace = new PoolSpace(8*1024);

    rml::MemPoolPolicy pol(CrossThreadGetMem, CrossThreadPutMem);
    rml::MemoryPool *pool;
    pool_create_v1(0, &pol, &pool);
    bool ok = pool_destroy(pool);
    ASSERT(ok, NULL);
    ASSERT(!poolSpace[0].regions, "No leaks.");

    delete poolSpace;
}

class FixedPoolHeadBase : NoAssign {
    size_t   size;
    intptr_t used;
    char    *data;
public:
    FixedPoolHeadBase(size_t s) : size(s), used(false) {
        data = new char[size];
    }
    void *useData(size_t &bytes) {
        intptr_t wasUsed = __TBB_FetchAndStoreW(&used, true);
        ASSERT(!wasUsed, "The buffer must not be used twice.");
        bytes = size;
        return data;
    }
    ~FixedPoolHeadBase() {
        delete []data;
    }
};

template<size_t SIZE>
class FixedPoolHead : FixedPoolHeadBase {
public:
    FixedPoolHead() : FixedPoolHeadBase(SIZE) { }
};

static void *fixedBufGetMem(intptr_t pool_id, size_t &bytes)
{
    return ((FixedPoolHeadBase*)pool_id)->useData(bytes);
}

class FixedPoolUse: NoAssign {
    static Harness::SpinBarrier startB;
    rml::MemoryPool *pool;
    size_t reqSize;
    int iters;
public:
    FixedPoolUse(unsigned threads, rml::MemoryPool *p, size_t sz, int it) :
        pool(p), reqSize(sz), iters(it) {
        startB.initialize(threads);
    }
    void operator()( int /*id*/ ) const {
        startB.wait();
        for (int i=0; i<iters; i++) {
            void *o = pool_malloc(pool, reqSize);
            ASSERT(o, NULL);
            pool_free(pool, o);
        }
    }
};

Harness::SpinBarrier FixedPoolUse::startB;

class FixedPoolNomem: NoAssign {
    Harness::SpinBarrier *startB;
    rml::MemoryPool *pool;
public:
    FixedPoolNomem(Harness::SpinBarrier *b, rml::MemoryPool *p) :
        startB(b), pool(p) {}
    void operator()(int id) const {
        startB->wait();
        void *o = pool_malloc(pool, id%2? 64 : 128*1024);
        ASSERT(!o, "All memory must be consumed.");
    }
};

class FixedPoolSomeMem: NoAssign {
    Harness::SpinBarrier *barrier;
    rml::MemoryPool *pool;
public:
    FixedPoolSomeMem(Harness::SpinBarrier *b, rml::MemoryPool *p) :
        barrier(b), pool(p) {}
    void operator()(int id) const {
        barrier->wait();
        Harness::Sleep(2*id);
        void *o = pool_malloc(pool, id%2? 64 : 128*1024);
        barrier->wait();
        pool_free(pool, o);
    }
};

bool haveEnoughSpace(rml::MemoryPool *pool, size_t sz)
{
    if (void *p = pool_malloc(pool, sz)) {
        pool_free(pool, p);
        return true;
    }
    return false;
}

void TestFixedBufferPool()
{
    const int ITERS = 7;
    const size_t MAX_OBJECT = 7*1024*1024;
    void *ptrs[ITERS];
    rml::MemPoolPolicy pol(fixedBufGetMem, NULL, 0, /*fixedSizePool=*/true,
                           /*keepMemTillDestroy=*/false);
    rml::MemoryPool *pool;
    {
        FixedPoolHead<MAX_OBJECT + 1024*1024> head;

        pool_create_v1((intptr_t)&head, &pol, &pool);
        {
            NativeParallelFor( 1, FixedPoolUse(1, pool, MAX_OBJECT, 2) );

            for (int i=0; i<ITERS; i++) {
                ptrs[i] = pool_malloc(pool, MAX_OBJECT/ITERS);
                ASSERT(ptrs[i], NULL);
            }
            for (int i=0; i<ITERS; i++)
                pool_free(pool, ptrs[i]);

            NativeParallelFor( 1, FixedPoolUse(1, pool, MAX_OBJECT, 1) );
        }
        // each thread asks for an MAX_OBJECT/p/2 object,
        // /2 is to cover fragmentation
        for (int p=MinThread; p<=MaxThread; p++)
            NativeParallelFor( p, FixedPoolUse(p, pool, MAX_OBJECT/p/2, 10000) );
        {
            const int p=128;
            NativeParallelFor( p, FixedPoolUse(p, pool, MAX_OBJECT/p/2, 1) );
        }
        {
            size_t maxSz;
            const int p = 512;
            Harness::SpinBarrier barrier(p);

            // Find maximal useful object size. Start with MAX_OBJECT/2,
            // as the pool might be fragmented by BootStrapBlocks consumed during
            // FixedPoolRun.
            size_t l, r;
            ASSERT(haveEnoughSpace(pool, MAX_OBJECT/2), NULL);
            for (l = MAX_OBJECT/2, r = MAX_OBJECT + 1024*1024; l < r-1; ) {
                size_t mid = (l+r)/2;
                if (haveEnoughSpace(pool, mid))
                    l = mid;
                else
                    r = mid;
            }
            maxSz = l;
            ASSERT(!haveEnoughSpace(pool, maxSz+1), "Expect to find boundary value.");
            // consume all available memory
            void *largeObj = pool_malloc(pool, maxSz);
            ASSERT(largeObj, NULL);
            void *o = pool_malloc(pool, 64);
            if (o) // pool fragmented, skip FixedPoolNomem
                pool_free(pool, o);
            else
                NativeParallelFor( p, FixedPoolNomem(&barrier, pool) );
            pool_free(pool, largeObj);
            // keep some space unoccupied
            largeObj = pool_malloc(pool, maxSz-512*1024);
            ASSERT(largeObj, NULL);
            NativeParallelFor( p, FixedPoolSomeMem(&barrier, pool) );
            pool_free(pool, largeObj);
        }
        bool ok = pool_destroy(pool);
        ASSERT(ok, NULL);
    }
    // check that fresh untouched pool can successfully fulfil requests from 128 threads
    {
        FixedPoolHead<MAX_OBJECT + 1024*1024> head;
        pool_create_v1((intptr_t)&head, &pol, &pool);
        int p=128;
        NativeParallelFor( p, FixedPoolUse(p, pool, MAX_OBJECT/p/2, 1) );
        bool ok = pool_destroy(pool);
        ASSERT(ok, NULL);
    }
}

static size_t currGranularity;

static void *getGranMem(intptr_t /*pool_id*/, size_t &bytes)
{
    ASSERT(!(bytes%currGranularity), "Region size mismatch granularity.");
    return malloc(bytes);
}

static int putGranMem(intptr_t /*pool_id*/, void *ptr, size_t bytes)
{
    ASSERT(!(bytes%currGranularity), "Region size mismatch granularity.");
    free(ptr);
    return 0;
}

void TestPoolGranularity()
{
    rml::MemPoolPolicy pol(getGranMem, putGranMem);
    const size_t grans[] = {4*1024, 2*1024*1024, 6*1024*1024, 10*1024*1024};

    for (unsigned i=0; i<sizeof(grans)/sizeof(grans[0]); i++) {
        pol.granularity = currGranularity = grans[i];
        rml::MemoryPool *pool;

        pool_create_v1(0, &pol, &pool);
        for (int sz=500*1024; sz<16*1024*1024; sz+=101*1024) {
            void *p = pool_malloc(pool, sz);
            ASSERT(p, "Can't allocate memory in pool.");
            pool_free(pool, p);
        }
        bool ok = pool_destroy(pool);
        ASSERT(ok, NULL);
    }
}

static size_t putMemCalls, getMemCalls;

static void *getMemPolicy(intptr_t /*pool_id*/, size_t &bytes)
{
    getMemCalls++;
    return malloc(bytes);
}

static int putMemPolicy(intptr_t /*pool_id*/, void *ptr, size_t /*bytes*/)
{
    putMemCalls++;
    free(ptr);
    return 0;
}

void TestPoolKeepTillDestroy()
{
    const int ITERS = 50*1024;
    void *ptrs[2*ITERS+1];
    rml::MemPoolPolicy pol(getMemPolicy, putMemPolicy);
    rml::MemoryPool *pool;

    // 1st create default pool that returns memory back to callback,
    // then use keepMemTillDestroy policy
    for (int keep=0; keep<2; keep++) {
        getMemCalls = putMemCalls = 0;
        if (keep)
            pol.keepAllMemory = 1;
        pool_create_v1(0, &pol, &pool);
        for (int i=0; i<2*ITERS; i+=2) {
            ptrs[i] = pool_malloc(pool, 7*1024);
            ptrs[i+1] = pool_malloc(pool, 10*1024);
        }
        ptrs[2*ITERS] = pool_malloc(pool, 8*1024*1024);
        ASSERT(!putMemCalls, NULL);
        for (int i=0; i<2*ITERS; i++)
            pool_free(pool, ptrs[i]);
        pool_free(pool, ptrs[2*ITERS]);
        size_t totalPutMemCalls = putMemCalls;
        if (keep)
            ASSERT(!putMemCalls, NULL);
        else {
            ASSERT(putMemCalls, NULL);
            putMemCalls = 0;
        }
        size_t currGetCalls = getMemCalls;
        pool_malloc(pool, 8*1024*1024);
        if (keep)
            ASSERT(currGetCalls == getMemCalls, "Must not lead to new getMem call");
        size_t currPuts = putMemCalls;
        bool ok = pool_reset(pool);
        ASSERT(ok, NULL);
        ASSERT(currPuts == putMemCalls, "Pool is not releasing memory during reset.");
        ok = pool_destroy(pool);
        ASSERT(ok, NULL);
        ASSERT(putMemCalls, NULL);
        totalPutMemCalls += putMemCalls;
        ASSERT(getMemCalls == totalPutMemCalls, "Memory leak detected.");
    }

}

static bool memEqual(char *buf, size_t size, int val)
{
    bool memEq = true;
    for (size_t k=0; k<size; k++)
        if (buf[k] != val)
             memEq = false;
    return memEq;
}

void TestEntries()
{
    const int SZ = 4;
    const int ALGN = 4;
    size_t size[SZ] = {8, 8000, 9000, 100*1024};
    size_t algn[ALGN] = {8, 64, 4*1024, 8*1024*1024};

    rml::MemPoolPolicy pol(getGranMem, putGranMem);
    currGranularity = 1; // not check granularity in the test
    rml::MemoryPool *pool;

    pool_create_v1(0, &pol, &pool);
    for (int i=0; i<SZ; i++)
        for (int j=0; j<ALGN; j++) {
            char *p = (char*)pool_aligned_malloc(pool, size[i], algn[j]);
            ASSERT(p && 0==((uintptr_t)p & (algn[j]-1)), NULL);
            memset(p, j, size[i]);

            size_t curr_algn = algn[rand() % ALGN];
            size_t curr_sz = size[rand() % SZ];
            char *p1 = (char*)pool_aligned_realloc(pool, p, curr_sz, curr_algn);
            ASSERT(p1 && 0==((uintptr_t)p1 & (curr_algn-1)), NULL);
            ASSERT(memEqual(p1, min(size[i], curr_sz), j), NULL);

            memset(p1, j+1, curr_sz);
            size_t curr_sz1 = size[rand() % SZ];
            char *p2 = (char*)pool_realloc(pool, p1, curr_sz1);
            ASSERT(p2, NULL);
            ASSERT(memEqual(p2, min(curr_sz1, curr_sz), j+1), NULL);

            pool_free(pool, p2);
        }

    bool ok = pool_destroy(pool);
    ASSERT(ok, NULL);

    bool fail = rml::pool_destroy(NULL);
    ASSERT(!fail, NULL);
    fail = rml::pool_reset(NULL);
    ASSERT(!fail, NULL);
}

void TestPoolCreation()
{
    using namespace rml;

    putMemCalls = getMemCalls = 0;

    MemPoolPolicy nullPolicy(NULL, putMemPolicy),
        emptyFreePolicy(getMemPolicy, NULL),
        okPolicy(getMemPolicy, putMemPolicy);
    MemoryPool *pool;

    MemPoolError res = pool_create_v1(0, &nullPolicy, &pool);
    ASSERT(res==INVALID_POLICY, "pool with empty pAlloc can't be created");
    res = pool_create_v1(0, &emptyFreePolicy, &pool);
    ASSERT(res==INVALID_POLICY, "pool with empty pFree can't be created");
    ASSERT(!putMemCalls && !getMemCalls, "no callback calls are expected");
    res = pool_create_v1(0, &okPolicy, &pool);
    ASSERT(res==POOL_OK, NULL);
    bool ok = pool_destroy(pool);
    ASSERT(ok, NULL);
    ASSERT(putMemCalls == getMemCalls, "no leaks after pool_destroy");

    // test creation of many pools
    // each pool consumes ~2MB of address space, so limit number
    // of pools on 32-bit systems
    const size_t MAX_POOLS = 4==sizeof(void*)? 500 : 16*1000;
    MemoryPool *pools[MAX_POOLS];
    size_t created;

    for (created=0; created<MAX_POOLS; created++) {
        putMemCalls = getMemCalls = 0;
        res = pool_create_v1(created, &okPolicy, pools+created);
        if (res!=POOL_OK) {
            ASSERT(!getMemCalls && !putMemCalls, "No leak after fail.");
            break;
        }
        void *o = pool_malloc(pools[created], 1024);
        ASSERT(o, "Created pool must be useful.");
        ASSERT(1==getMemCalls && !putMemCalls, "Single callback request expected.");
        pool_free(pools[created], o);
    }
    // 32 is guess for number of pools that is acceptable everywere
    ASSERT(created>=32, "Expect that reasonable number of pools can be created at any system.");
    for (size_t i=0; i<created; i++) {
        ok = pool_destroy(pools[i]);
        ASSERT(ok, NULL);
    }
    // check that failure during pool creation doesn't lead to leaks
    for (size_t i=0; i<created; i++) {
        res = pool_create_v1(i, &okPolicy, pools+i);
        ASSERT(res==POOL_OK, "2nd iteration of pool creation must be possible.");
        void *o = pool_malloc(pools[i], 1024);
        ASSERT(o, "Created pool must be useful.");
        pool_free(pools[i], o);
    }
    for (size_t i=0; i<created; i++) {
        ok = pool_destroy(pools[i]);
        ASSERT(ok, NULL);
    }
}

struct AllocatedObject {
    rml::MemoryPool *pool;
};

// TODO: extend testing of pool_identify() in concurrent environment
void TestPoolDetection()
{
    using namespace rml;
    const int POOLS = 4;
    rml::MemPoolPolicy pol(fixedBufGetMem, NULL, 0, /*fixedSizePool=*/true,
                           /*keepMemTillDestroy=*/false);
    rml::MemoryPool *pools[POOLS];
    const size_t BUF_SIZE = 1024*1024;
    FixedPoolHead<BUF_SIZE> head[POOLS];
    AllocatedObject *objs[POOLS];

    for (int i=0; i<POOLS; i++)
        pool_create_v1((intptr_t)(head+i), &pol, &pools[i]);
    // if object somehow released to different pools, subsequent allocation
    // from affected pools became impossible
    for (int k=0; k<10; k++) {
        for (int i=0; i<POOLS; i++) {
            objs[i] = (AllocatedObject*)pool_malloc(pools[i], BUF_SIZE/2);
            ASSERT(objs[i], NULL);
            MemoryPool *act_pool = pool_identify(objs[i]);
            ASSERT(act_pool == pools[i], NULL);
            objs[i]->pool = act_pool;
            for (size_t total=0; total<2*BUF_SIZE; total+=256) {
                AllocatedObject *o = (AllocatedObject*)pool_malloc(pools[i], 256);
                ASSERT(o, NULL);
                act_pool = pool_identify(o);
                ASSERT(act_pool == pools[i], NULL);
                o->pool = act_pool;
                pool_free(act_pool, o);
            }
        }
        for (int i=0; i<POOLS; i++) {
            rml::MemoryPool *p = pool_identify(objs[i]);
            ASSERT(p == objs[i]->pool, NULL);
            pool_free(p, objs[i]);
        }
    }
    for (int i=0; i<POOLS; i++) {
        bool ok = pool_destroy(pools[i]);
        ASSERT(ok, NULL);
    }
}


void TestLazyBootstrap()
{
    rml::MemPoolPolicy pol(getMemPolicy, putMemPolicy);
    rml::MemoryPool *pool;
    const size_t sizes[] = {8, 9*1024, 0};

    for (int i=0; sizes[i]; i++) {
        putMemCalls = getMemCalls = 0;
        pool_create_v1(0, &pol, &pool);
        ASSERT(!putMemCalls && !getMemCalls,
               "memory from the pool must not be requested during pool creation.");
        void *o = pool_malloc(pool, sizes[i]);
        ASSERT(o, "Created pool must be useful.");
        ASSERT(1==getMemCalls && !putMemCalls, "Single callback request expected.");
        pool_free(pool, o);
        bool ok = pool_destroy(pool);
        ASSERT(ok, NULL);
        ASSERT(getMemCalls==putMemCalls, "No leak.");
    }
}

class NoLeakOnDestroyRun: NoAssign {
    rml::MemoryPool      *pool;
    Harness::SpinBarrier *barrier;
public:
    NoLeakOnDestroyRun(rml::MemoryPool *p, Harness::SpinBarrier *b) : pool(p), barrier(b) {}
    void operator()(int id) const {
        void *p = pool_malloc(pool, id%2? 8 : 9000);
        ASSERT(p && liveRegions, NULL);
        barrier->wait();
        if (!id) {
            bool ok = pool_destroy(pool);
            ASSERT(ok, NULL);
            ASSERT(!liveRegions, "Expected all regions were released.");
        }
        // other threads must wait till pool destruction,
        // to not call thread destruction cleanup before this
        barrier->wait();
    }
};

void TestNoLeakOnDestroy()
{
    liveRegions = 0;
    for (int p=MinThread; p<=MaxThread; p++) {
        rml::MemPoolPolicy pol(getMallocMem, putMallocMem);
        Harness::SpinBarrier barrier(p);
        rml::MemoryPool *pool;

        pool_create_v1(0, &pol, &pool);
        NativeParallelFor(p, NoLeakOnDestroyRun(pool, &barrier));
    }
}


static int putMallocMemError(intptr_t /*pool_id*/, void *ptr, size_t bytes)
{
    MallocPoolHeader *hdr = (MallocPoolHeader*)ptr-1;
    ASSERT(bytes == hdr->userSize, "Invalid size in pool callback.");
    free(hdr->rawPtr);

    liveRegions--;

    return -1;
}

void TestDestroyFailed()
{
    rml::MemPoolPolicy pol(getMallocMem, putMallocMemError);
    rml::MemoryPool *pool;
    pool_create_v1(0, &pol, &pool);
    void *ptr = pool_malloc(pool, 16);
    ASSERT(ptr, NULL);
    bool fail = pool_destroy(pool);
    ASSERT(fail==false, "putMemPolicyError callback returns error, "
           "expect pool_destroy() failure");
}

int TestMain () {
    TestTooSmallBuffer();
    TestPoolReset();
    TestSharedPool();
    TestCrossThreadPools();
    TestFixedBufferPool();
    TestPoolGranularity();
    TestPoolKeepTillDestroy();
    TestEntries();
    TestPoolCreation();
    TestPoolDetection();
    TestLazyBootstrap();
    TestNoLeakOnDestroy();
    TestDestroyFailed();

    return Harness::Done;
}
