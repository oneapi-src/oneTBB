/*
    Copyright (c) 2005-2023 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <common/test.h>
#include <common/utils.h>
#include <common/vector_types.h>
#include <common/custom_allocators.h>
#include <common/container_move_support.h>

#include <tbb/concurrent_queue.h>
#include <unordered_set>

//! \file test_concurrent_queue.cpp
//! \brief Test for [containers.concurrent_queue containers.concurrent_bounded_queue] specification

static constexpr std::size_t MaxThread = 4;

template<typename CQ, typename T>
struct TestQueueElements {
    CQ& queue;
    const std::size_t nthread;
    TestQueueElements( CQ& q, std::size_t n ) : queue(q), nthread(n) {}
    void operator()( std::size_t k ) const {
        for (std::size_t i=0; i < 1000; ++i) {
            if( (i&0x1)==0 ) {
                CHECK(T(k) < T(nthread));
                queue.push(T(k));
            } else {
                // Pop item from queue
                T item = 0;
                queue.try_pop(item);
                CHECK(item <= T(nthread));
            }
        }
    }
};

//! Test concurrent queue with primitive data type
template<typename CQ, typename T>
void TestPrimitiveTypes(std::size_t nthread, T exemplar) {
    CQ queue;
    for (std::size_t i = 0; i < 100; ++i) {
        queue.push(exemplar);
    }
    utils::NativeParallelFor(nthread, TestQueueElements<CQ, T>(queue, nthread));
}

void TestQueueWorksWithPrimitiveTypes() {
    TestPrimitiveTypes<tbb::concurrent_queue<char>, char>(MaxThread, (char)1);
    TestPrimitiveTypes<tbb::concurrent_queue<int>, int>(MaxThread, (int)-12);
    TestPrimitiveTypes<tbb::concurrent_queue<float>, float>(MaxThread, (float)-1.2f);
    TestPrimitiveTypes<tbb::concurrent_queue<double>, double>(MaxThread, (double)-4.3);
    TestPrimitiveTypes<tbb::concurrent_bounded_queue<char>, char>(MaxThread, (char)1);
    TestPrimitiveTypes<tbb::concurrent_bounded_queue<int>, int>(MaxThread, (int)-12);
    TestPrimitiveTypes<tbb::concurrent_bounded_queue<float>, float>(MaxThread, (float)-1.2f);
    TestPrimitiveTypes<tbb::concurrent_bounded_queue<double>, double>(MaxThread, (double)-4.3);
}

#if HAVE_m128 || HAVE_m256
//! Test concurrent queue with vector types
/** Type Queue should be a queue of ClassWithSSE/ClassWithAVX. */
template<typename ClassWithVectorType, typename Queue>
void TestVectorTypes() {
    Queue q1;
    for (int i = 0; i < 100; ++i) {
        // VC8 does not properly align a temporary value; to work around, use explicit variable
        ClassWithVectorType bar(i);
        q1.push(bar);
    }

    // Copy the queue
    Queue q2 = q1;
    // Check that elements of the copy are correct
    typename Queue::const_iterator ci = q2.unsafe_begin();
    for (int i=0; i < 100; ++i ) {
        CHECK((ci != q2.unsafe_end()));
        ClassWithVectorType foo = *ci;
        ClassWithVectorType bar(i);
        CHECK((*ci == bar));
        ++ci;
    }

    for (int i = 0; i < 101; ++i) {
        ClassWithVectorType tmp;
        bool b = q1.try_pop(tmp);
        CHECK((b == (i < 100)));
        ClassWithVectorType bar(i);
        CHECK((!b || tmp==bar));
    }
}
#endif /* HAVE_m128 || HAVE_m256 */

void TestQueueWorksWithSSE() {

#if HAVE_m128
    TestVectorTypes<ClassWithSSE, tbb::concurrent_queue<ClassWithSSE> >();
    TestVectorTypes<ClassWithSSE, tbb::concurrent_bounded_queue<ClassWithSSE> >();
#endif /* HAVE_m128 */
#if HAVE_m256
    if( have_AVX() ) {
        TestVectorTypes<ClassWithAVX, tbb::concurrent_queue<ClassWithAVX> >();
        TestVectorTypes<ClassWithAVX, tbb::concurrent_bounded_queue<ClassWithAVX> >();
    }
#endif /* HAVE_m256 */
}
#if TBB_USE_EXCEPTIONS
    int rnd_elem = -1;
    int global_counter = -1;

struct throw_element {
    throw_element() = default;
    throw_element(const throw_element&) {
        if (global_counter++ == rnd_elem) {
            throw std::exception{};
        }
    }

    throw_element& operator= (const throw_element&) = default;
};

template <typename Queue>
void CopyWithThrowElement() {
    utils::FastRandom<> rnd(42);

    Queue source;

    constexpr size_t queue_size = 100000;
    for (std::size_t i = 0; i < queue_size; ++i) {
        source.emplace();
    }

    for (std::size_t i = 0; i < 100; ++i) {
        global_counter = 0;
        rnd_elem = rnd.get() % queue_size;

        REQUIRE_THROWS_AS( [&] {
            Queue copy(source);
            utils::suppress_unused_warning(copy);
        }(), std::exception);
    }
}
#endif // TBB_USE_EXCEPTIONS

//! Test work with different fypes
//! \brief \ref error_guessing
TEST_CASE("testing work with different fypes") {
    TestQueueWorksWithPrimitiveTypes();
}

//! Test work with vector types
//! \brief \ref error_guessing
TEST_CASE("testing vector types") {
    TestQueueWorksWithSSE();
}

#if TBB_USE_EXCEPTIONS
//! \brief \ref regression \ref error_guessing
TEST_CASE("Test exception in allocation") {
    using allocator_type = StaticSharedCountingAllocator<std::allocator<int>>;
    using queue_type = tbb::concurrent_queue<int, allocator_type>;

    queue_type src_queue;
    for (int i = 0; i < 100000; ++i) {
        src_queue.push(i);
    }

    allocator_type::set_limits(1);

    REQUIRE_THROWS_AS( [] {
        queue_type queue1;
        queue1.push(1);
    }(), const std::bad_alloc);

    for (std::size_t i = 1; i < 1000; ++i) {
        allocator_type::init_counters();
        allocator_type::set_limits(1);
        REQUIRE_THROWS_AS( [&] {
            queue_type queue2(src_queue);
            utils::suppress_unused_warning(queue2);
        }(), const std::bad_alloc);
    }
}

//! \brief \ref regression \ref error_guessing
TEST_CASE("Test exception in allocation") {
    CopyWithThrowElement<tbb::concurrent_queue<throw_element>>();
    CopyWithThrowElement<tbb::concurrent_bounded_queue<throw_element>>();
}

#endif // TBB_USE_EXCEPTIONS

struct TrackableItem {
    static std::unordered_set<TrackableItem*> object_addresses;
#if TBB_USE_EXCEPTIONS
    static std::size_t global_count_for_exceptions;
#endif

    TrackableItem() {
#if TBB_USE_EXCEPTIONS
        if (global_count_for_exceptions++ % 3 == 0) throw 1;
#endif
        bool res = object_addresses.emplace(this).second;
        CHECK(res);
    }

    ~TrackableItem() {
        auto it = object_addresses.find(this);
        CHECK(it != object_addresses.end());
        object_addresses.erase(it);
    }
};

template <typename Container>
void fill_and_catch(Container& q, std::size_t elements_count) {
    CHECK(TrackableItem::object_addresses.size() == 0);
    for (std::size_t i = 0; i < elements_count; ++i) {
#if TBB_USE_EXCEPTIONS
        try {
#endif
            q.emplace();
#if TBB_USE_EXCEPTIONS
        } catch (int exception) {
            CHECK(exception == 1);
        }
#endif
    }
#if TBB_USE_EXCEPTIONS
    CHECK(TrackableItem::object_addresses.size() == 2 * elements_count / 3);
#else
    CHECK(TrackableItem::object_addresses.size() == elements_count);
#endif
}

std::unordered_set<TrackableItem*> TrackableItem::object_addresses;
#if TBB_USE_EXCEPTIONS
std::size_t TrackableItem::global_count_for_exceptions = 0;
#endif

template <typename Container>
void test_tracking_dtors_on_clear() {
    static_assert(std::is_same<typename Container::value_type, TrackableItem>::value, "Incorrect test setup");
    const std::size_t elements_count = 100000;
    {
        Container q;
        fill_and_catch(q, elements_count);

        q.clear();
        
        CHECK(q.empty());
        CHECK(TrackableItem::object_addresses.empty());
#if TBB_USE_EXCEPTIONS
        TrackableItem::global_count_for_exceptions = 0;
#endif
    }
    {
        {
            Container q;
            fill_and_catch(q, elements_count);
        } // Dtor of q would be called here
        CHECK(TrackableItem::object_addresses.empty());
#if TBB_USE_EXCEPTIONS
        TrackableItem::global_count_for_exceptions = 0;
#endif
    }
}

//! \brief \ref regression \ref error_guessing
TEST_CASE("Test clear and dtor with TrackableItem") {
    test_tracking_dtors_on_clear<oneapi::tbb::concurrent_queue<TrackableItem>>();
    test_tracking_dtors_on_clear<oneapi::tbb::concurrent_bounded_queue<TrackableItem>>();
}

template<typename CQ, typename T>
void test_queue_helper(){
    int size = 5;
    T vec_1(size, 0), vec_2(size, 0), vec_3(size, 0), vec_4(size, 0);
    srand(static_cast<unsigned>(time(0)));
    generate(vec_1.begin(), vec_1.end(), rand);
    generate(vec_2.begin(), vec_2.end(), rand);
    generate(vec_3.begin(), vec_3.end(), rand);
    generate(vec_4.begin(), vec_4.end(), rand);
    
    CQ q1, q2;
    CQ q4({vec_1, vec_2, vec_3});
    CQ q3 = {vec_4, vec_2, vec_3};

    q1 = q3;
    q2 = std::move(q3);

    CHECK(q1 != q4);
    q1.swap(q4);
    CHECK(q2 == q4);
}

//basic test for copy, move and swap
TEST_CASE("Test iterator queue"){
    test_queue_helper<tbb::concurrent_queue<std::vector<int>>, std::vector<int>>();
    test_queue_helper<tbb::concurrent_bounded_queue<std::vector<int>>, std::vector<int>>();
}

template <typename queue_type, typename allocator_type>
void TestMoveQueue(){
    allocator_type::set_limits(300);
    queue_type q1, q2;
    move_support_tests::Foo obj;
    size_t n1(15), n2(7);

    allocator_type::init_counters();
    for(size_t i =0; i < n1; i++)
      q1.push(obj);
    size_t q1_items_constructed = allocator_type::items_constructed;
    size_t q1_items_allocated =  allocator_type::items_allocated;

    allocator_type::init_counters();
    for(size_t i =0; i < n2; i++)
      q2.push(obj);
    size_t q2_items_allocated =  allocator_type::items_allocated;

    allocator_type::init_counters();
    q1 = std::move(q2);

    CHECK(allocator_type::items_freed == q1_items_allocated);
    CHECK(allocator_type::items_destroyed == q1_items_constructed);
    CHECK(allocator_type::items_allocated <= q2_items_allocated);
}

//move assignment test for equal allocator
TEST_CASE("Test move queue"){
    using allocator_type = StaticSharedCountingAllocator<std::allocator<move_support_tests::Foo>>;
    TestMoveQueue<tbb::concurrent_queue<move_support_tests::Foo, allocator_type>, allocator_type>();
    TestMoveQueue<tbb::concurrent_bounded_queue<move_support_tests::Foo, allocator_type>, allocator_type>();
}

template<class T>
struct stateful_allocator
{
    typedef T value_type;
    stateful_allocator () = default;
    int state;
    template<class U>
    constexpr stateful_allocator (const stateful_allocator <U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n)
    {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_array_new_length();

        if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
        {
            return p;
        }

        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        static_cast<void>(n);
        std::free(p);
    }
};

template<class T, class U>
bool operator==(const stateful_allocator <T>& lhs, const stateful_allocator <U>& rhs) { return &lhs.state == &rhs.state; }

template<class T, class U>
bool operator!=(const stateful_allocator <T>& lhs, const stateful_allocator <U>& rhs) { return &lhs.state != &rhs.state; }

template <typename queue_type, typename allocator_type>
void TestMoveQueueUnequal(){
    allocator_type::set_limits(300);
    queue_type q1, q2;
    move_support_tests::Foo obj;
    size_t n1(15), n2(7);

    allocator_type::init_counters();
    for(size_t i =0; i < n1; i++)
      q1.push(obj);
    size_t q1_items_constructed = allocator_type::items_constructed;
    size_t q1_items_allocated =  allocator_type::items_allocated;

    allocator_type::init_counters();
    for(size_t i =0; i < n2; i++)
      q2.push(obj);
    size_t q2_items_allocated =  allocator_type::items_allocated;

    allocator_type::init_counters();
    q1 = std::move(q2);

    CHECK(allocator_type::items_allocated <= q2_items_allocated);
    REQUIRE_MESSAGE( (allocator_type::items_allocated - q2_items_allocated) <= 0, "More then excepted memory allocated ?" );
    
    REQUIRE_MESSAGE(std::all_of(q1.unsafe_begin(), q1.unsafe_end(), is_state_predicate<move_support_tests::Foo::MoveInitialized>()),
		    "Container did not move construct some elements");
    REQUIRE_MESSAGE(std::all_of(q2.unsafe_begin(), q2.unsafe_end(), is_state_predicate<move_support_tests::Foo::MovedFrom>()),
		    		    "Container did not move all the elements");
}

//move assignment test for unequal allocator
TEST_CASE("Test move queue-unequal allocator"){
    using allocator_type = StaticSharedCountingAllocator<stateful_allocator<move_support_tests::Foo>>;
    TestMoveQueueUnequal<tbb::concurrent_queue<move_support_tests::Foo, allocator_type>, allocator_type>();
    TestMoveQueueUnequal<tbb::concurrent_bounded_queue<move_support_tests::Foo, allocator_type>, allocator_type>();
}

template<typename Container>
void test_check_move_equal_allocator(Container& src, Container& dst){
    REQUIRE_MESSAGE(src.get_allocator() == dst.get_allocator(), "Incorrect test setup: allocators should be equal");
    REQUIRE_MESSAGE(&*(src.unsafe_begin()) ==  &*(dst.unsafe_begin()), "Container move actually changed element locations, while should not");
    REQUIRE_MESSAGE(std::equal(dst.unsafe_begin(), dst.unsafe_end(), src.unsafe_begin()), "Elements are not equal");
}

template<typename Container>
void test_check_move_unequal_allocator(Container& src, Container& dst, Container& cpy){
    REQUIRE_MESSAGE(src.get_allocator() != dst.get_allocator(), "Incorrect test setup: allocators should be unequal");
    REQUIRE_MESSAGE(&*(src.unsafe_begin()) !=  &*(dst.unsafe_begin()), "Container did not change element locations for unequal allocators");
    REQUIRE_MESSAGE(std::equal(dst.unsafe_begin(), dst.unsafe_end(), cpy.unsafe_begin()), "Elements are not equal");
}

void test_move_assignment_test_stateless(){
    int n = 5;
    std::vector<int> vect1(n, 10), vect2(n,20), vect3(n, 30);

    tbb::concurrent_queue<std::vector<int>> src({vect1, vect2, vect3});
    tbb::concurrent_queue<std::vector<int>> dst(src.get_allocator());
    dst = std::move(src);

    tbb::concurrent_bounded_queue<std::vector<int>> src_bnd({vect1, vect2, vect3});
    tbb::concurrent_bounded_queue<std::vector<int>> dst_bnd(src_bnd.get_allocator());
    dst_bnd = std::move(src_bnd);

    test_check_move_equal_allocator<tbb::concurrent_queue<std::vector<int>>>(src, dst);  
    REQUIRE_MESSAGE(src.unsafe_size() == dst.unsafe_size(), "Queues are not equal");

    test_check_move_equal_allocator<tbb::concurrent_bounded_queue<std::vector<int>>>(src_bnd, dst_bnd);
    REQUIRE_MESSAGE(src_bnd.size() == dst_bnd.size(), "Queues are not equal");
}

void test_move_assignment_test_stateful(){
    stateful_allocator<int> src_alloc;
    std::vector<int, stateful_allocator<int>> v(8, src_alloc);
    tbb::concurrent_queue<std::vector<int, stateful_allocator<int>>, stateful_allocator<int>> src(src_alloc);
    
    src_alloc.state = 0;
    v.push_back(42);
    v.push_back(82);
    src.push(v);
    src.push(v);

    stateful_allocator<int> dst_alloc;
    dst_alloc.state = 1;
    tbb::concurrent_queue<std::vector<int, stateful_allocator<int>>, stateful_allocator<int>> dst(dst_alloc);
    tbb::concurrent_queue<std::vector<int, stateful_allocator<int>>, stateful_allocator<int>> cpy(src_alloc);
    cpy = src;
    dst = std::move(src);

    tbb::concurrent_bounded_queue<std::vector<int, stateful_allocator<int>>, stateful_allocator<int>> src_bnd(src_alloc);
    tbb::concurrent_bounded_queue<std::vector<int, stateful_allocator<int>>, stateful_allocator<int>> dst_bnd(dst_alloc);
    tbb::concurrent_bounded_queue<std::vector<int, stateful_allocator<int>>, stateful_allocator<int>> cpy_bnd(src_alloc);
    src_bnd.push(v);
    src_bnd.push(v);
    cpy_bnd = src_bnd;
    dst_bnd = std::move(src_bnd);
    
    test_check_move_unequal_allocator<tbb::concurrent_queue<std::vector<int, stateful_allocator<int>>, stateful_allocator<int>>>(src, dst, cpy);
    REQUIRE_MESSAGE(src.unsafe_size() == 0, "Moved from container should not contain any elements");
    REQUIRE_MESSAGE(dst.unsafe_size() == cpy.unsafe_size(), "Queues are not equal");
    REQUIRE_MESSAGE(std::equal(dst.unsafe_begin(), dst.unsafe_end(), cpy.unsafe_begin()), "Elements are not equal");
    
    test_check_move_unequal_allocator<tbb::concurrent_bounded_queue<std::vector<int, stateful_allocator<int>>, stateful_allocator<int>>>(src_bnd, dst_bnd, cpy_bnd);
    REQUIRE_MESSAGE(src_bnd.size() == 0, "Moved from container should not contain any elements");
    REQUIRE_MESSAGE(dst_bnd.size() == cpy_bnd.size(), "Queues are not equal");
    REQUIRE_MESSAGE(std::equal(dst_bnd.unsafe_begin(), dst_bnd.unsafe_end(), cpy_bnd.unsafe_begin()), "Elements are not equal");
}

//move assignment test for equal and unequal allocator
TEST_CASE("concurrent_queue") {
    test_move_assignment_test_stateless();
    test_move_assignment_test_stateful();
}
