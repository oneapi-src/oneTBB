/*
    Copyright (c) 2005-2022 Intel Corporation

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

#include <tbb/concurrent_queue.h>

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

class MinimalisticObject {
public:
    struct flag {};
    static constexpr std::size_t default_obj = 42;

    MinimalisticObject() = delete;
    MinimalisticObject(flag) : underlying_obj(default_obj) {}

    MinimalisticObject(const MinimalisticObject&) = delete;
    MinimalisticObject& operator=(const MinimalisticObject&) = delete;

    std::size_t get_obj() const { return underlying_obj; }

protected:
    std::size_t underlying_obj;
    friend struct MoveAssignableMinimalisticObject;
};

constexpr std::size_t MinimalisticObject::default_obj;

struct MoveAssignableMinimalisticObject : MinimalisticObject {
public:
    using MinimalisticObject::MinimalisticObject;

    MoveAssignableMinimalisticObject& operator=(MoveAssignableMinimalisticObject&& other) {
        if (this != &other) {
            underlying_obj = other.underlying_obj;
            other.underlying_obj = 0;
        }
        return *this;
    }
};

template <typename Container>
void test_basics(Container& container, std::size_t desired_size) {
    CHECK(!container.empty());

    std::size_t counter = 0;
    for (auto it = container.unsafe_begin(); it != container.unsafe_end(); ++it) {
        CHECK(it->get_obj() == MinimalisticObject::default_obj);
        ++counter;
    }
    CHECK(counter == desired_size);

    container.clear();
    CHECK(container.empty());
}

template <template <class...> typename Container>
void test_with_minimalistic_objects() {
    // Test with MinimalisticObject and no pop operations
    const std::size_t elements_count = 100;
    {
        Container<MinimalisticObject> default_container;

        for (std::size_t i = 0; i < elements_count; ++i) {
            default_container.emplace(MinimalisticObject::flag{});
        }
        test_basics(default_container, elements_count);
    }
    // Test with MoveAssignableMinimalisticObject with pop operation
    {
        Container<MoveAssignableMinimalisticObject> default_container;

        for (std::size_t i = 0; i < elements_count; ++i) {
            default_container.emplace(MinimalisticObject::flag{});
        }
        test_basics(default_container, elements_count);

        // Refill again
        for (std::size_t i = 0; i < elements_count; ++i) {
            default_container.emplace(MinimalisticObject::flag{});
        }

        MoveAssignableMinimalisticObject result(MinimalisticObject::flag{});

        std::size_t element_counter = 0;
        while(!default_container.empty()) {
            CHECK(default_container.try_pop(result));
            ++element_counter;
        }

        CHECK(element_counter == elements_count);
        CHECK(default_container.empty());
    }
}

//! \brief \ref regression \ref error_guessing
TEST_CASE("Test with minimalistic object type") {
    test_with_minimalistic_objects<oneapi::tbb::concurrent_queue>();
    test_with_minimalistic_objects<oneapi::tbb::concurrent_bounded_queue>();
}