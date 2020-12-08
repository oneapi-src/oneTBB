/*
    Copyright (c) 2005-2020 Intel Corporation

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

#ifndef __TBB_parallel_for_each_H
#define __TBB_parallel_for_each_H

#include "detail/_config.h"
#include "detail/_namespace_injection.h"
#include "detail/_exception.h"
#include "detail/_task.h"
#include "detail/_aligned_space.h"
#include "detail/_small_object_pool.h"

#include "parallel_for.h"
#include "task_group.h" // task_group_context

#include <iterator>
#include <type_traits>

namespace tbb {
namespace detail {
namespace d1 {

template<typename Body, typename Item> class feeder_impl;

//! Class the user supplied algorithm body uses to add new tasks
template<typename Item>
class feeder {
    feeder() {}
    feeder(const feeder&) = delete;
    void operator=( const feeder&) = delete;

    virtual ~feeder () {}
    virtual void internal_add_copy(const Item& item) = 0;
    virtual void internal_add_move(Item&& item) = 0;

    template<typename Body_, typename Item_> friend class detail::d1::feeder_impl;
public:
    //! Add a work item to a running parallel_for_each.
    void add(const Item& item) {internal_add_copy(item);}
    void add(Item&& item) {internal_add_move(std::move(item));}
};

/** Selects one of the two possible forms of function call member operator.
    @ingroup algorithms **/
template<class Body, typename Item>
class parallel_for_each_operator_selector {
public:
    //! Hack for resolve ambiguity between calls that may be forwarded or just passed by value
    //! in this case overload with forward declaration must have higher priority.
    using first_priority = int;
    using second_priority = double;

    template<typename ItemArg, typename FeederArg>
    static auto select_call (const Body& body, ItemArg&& item, FeederArg&, first_priority)
    -> decltype(body(std::forward<ItemArg>(item)), void()) {
        #if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
        // Suppression of Microsoft non-standard extension warnings
        #pragma warning (push)
        #pragma warning (disable: 4239)
        #endif

        body(std::forward<ItemArg>(item));

        #if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
        #pragma warning (push)
        #endif
    }

    template<typename ItemArg, typename FeederArg>
    static auto select_call (const Body& body, ItemArg&& item, FeederArg& feeder, first_priority)
    -> decltype(body(std::forward<ItemArg>(item), feeder), void()) {
        #if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
        // Suppression of Microsoft non-standard extension warnings
        #pragma warning (push)
        #pragma warning (disable: 4239)
        #endif

        body(std::forward<ItemArg>(item), feeder);

        #if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
        #pragma warning (push)
        #endif
    }

    template<typename ItemArg, typename FeederArg>
    static auto select_call (const Body& body, ItemArg&& item, FeederArg&, second_priority)
    -> decltype(body(item), void()) {
        body(item);
    }

    template<typename ItemArg, typename FeederArg>
    static auto select_call (const Body& body, ItemArg&& item, FeederArg& feeder, second_priority)
    -> decltype(body(item, feeder), void()){
        body(item, feeder);
    }
public:
    template<typename ItemArg, typename FeederArg>
    static void call(const Body& body, ItemArg&& item, FeederArg& feeder) {
        select_call(body, std::forward<ItemArg>(item), feeder, first_priority{});
    }
};

template<typename Body, typename Item>
struct feeder_item_task: public task {
    using feeder_type = feeder_impl<Body, Item>;

    template <typename ItemType>
    feeder_item_task(ItemType&& input_item, feeder_type& feeder, small_object_allocator& alloc) :
        item(std::forward<ItemType>(input_item)),
        my_feeder(feeder),
        my_allocator(alloc)
    {}

    void finalize(const execution_data& ed) {
        my_feeder.my_wait_context.release();
        my_allocator.delete_object(this, ed);
    }

    task* execute(execution_data& ed) override {
        parallel_for_each_operator_selector<Body, Item>::call(my_feeder.my_body, std::move(item), my_feeder);
        finalize(ed);
        return nullptr;
    }

    task* cancel(execution_data& ed) override {
        finalize(ed);
        return nullptr;
    }

    Item item;
    feeder_type& my_feeder;
    small_object_allocator my_allocator;
}; // class feeder_item_task

/** Implements new task adding procedure.
    @ingroup algorithms **/
template<class Body, typename Item>
class feeder_impl : public feeder<Item> {
    // Avoiding use of copy constructor in a virtual method if the type does not support it
    void internal_add_copy_impl(std::true_type, const Item& item) {
        using feeder_task = feeder_item_task<Body, Item>;
        small_object_allocator alloc;
        auto task = alloc.new_object<feeder_task>(item, *this, alloc);

        my_wait_context.reserve();
        spawn(*task, my_execution_context);
    }

    void internal_add_copy_impl(std::false_type, const Item&) {
        __TBB_ASSERT(false, "Overloading for r-value reference doesn't work or it's not movable and not copyable object");
    }

    void internal_add_copy(const Item& item) override {
        internal_add_copy_impl(typename std::is_copy_constructible<Item>::type(), item);
    }

    void internal_add_move(Item&& item) override {
        using feeder_task = feeder_item_task<Body, Item>;
        small_object_allocator alloc{};
        auto task = alloc.new_object<feeder_task>(std::move(item), *this, alloc);

        my_wait_context.reserve();
        spawn(*task, my_execution_context);
    }
public:
    feeder_impl(task_group_context &context, const Body& body)
      : my_wait_context(0)
      , my_body(body)
      , my_execution_context(context)
    {}

    wait_context my_wait_context;
    const Body& my_body;
    task_group_context& my_execution_context;
}; // class feeder_impl

/** Execute computation under one element of the range
    @ingroup algorithms **/
template<typename Iterator, typename Body, typename Item>
struct for_each_iteration_task: public task {
    using feeder_type = feeder_impl<Body, Item>;

    for_each_iteration_task(Iterator input_item_ptr, feeder_type& feeder, wait_context& wait_context) :
        item_ptr(input_item_ptr), my_feeder(feeder), parent_wait_context(wait_context)
    {}

    void finalize() {
        parent_wait_context.release();
    }

    task* execute(execution_data&) override {
        parallel_for_each_operator_selector<Body, Item>::call(my_feeder.my_body, std::move(*item_ptr), my_feeder);
        finalize();
        return nullptr;
    }

    task* cancel(execution_data&) override {
        finalize();
        return nullptr;
    }

    Iterator item_ptr;
    feeder_type& my_feeder;
    wait_context& parent_wait_context;
}; // class do_iteration_task_iter

/** Split one block task to several(max_block_size) iteration tasks for input iterators
    @ingroup algorithms **/
template <typename Body, typename Item>
struct input_block_handling_task : public task {
    static constexpr size_t max_block_size = 4;

    using feeder_type = feeder_impl<Body, Item>;
    using iteration_task = for_each_iteration_task<Item*, Body, Item>;

    input_block_handling_task(feeder_type& feeder, small_object_allocator& alloc)
        :my_size(0), my_wait_context(0), my_feeder(feeder), my_allocator(alloc)
    {
        auto* item_it = block_iteration_space.begin();
        for (auto* it = task_pool.begin(); it != task_pool.end(); ++it) {
            new (it) iteration_task(item_it++, feeder, my_wait_context);
        }
    }

    void finalize(const execution_data& ed) {
        my_feeder.my_wait_context.release();
        my_allocator.delete_object(this, ed);
    }

    task* execute(execution_data& ed) override {
        __TBB_ASSERT( my_size > 0, "Negative size was passed to task");
        for (std::size_t counter = 1; counter < my_size; ++counter) {
            my_wait_context.reserve();
            spawn(*(task_pool.begin() + counter), my_feeder.my_execution_context);
        }
        my_wait_context.reserve();
        execute_and_wait(*task_pool.begin(), my_feeder.my_execution_context,
                         my_wait_context,     my_feeder.my_execution_context);

        // deallocate current task after children execution
        finalize(ed);
        return nullptr;
    }

    task* cancel(execution_data& ed) override {
        finalize(ed);
        return nullptr;
    }

    ~input_block_handling_task() {
        for(std::size_t counter = 0; counter < max_block_size; ++counter) {
            (task_pool.begin() + counter)->~iteration_task();
            (block_iteration_space.begin() + counter)->~Item();
        }
    }

    aligned_space<Item, max_block_size> block_iteration_space;
    aligned_space<iteration_task, max_block_size> task_pool;
    std::size_t my_size;
    wait_context my_wait_context;
    feeder_type& my_feeder;
    small_object_allocator my_allocator;
}; // class input_block_execution_task

/** Split one block task to several(max_block_size) iteration tasks for forward iterators
    @ingroup algorithms **/
template <typename Iterator, typename Body, typename Item>
struct forward_block_handling_task : public task {
    static constexpr size_t max_block_size = 4;

    using feeder_type = feeder_impl<Body, Item>;
    using iteration_task = for_each_iteration_task<Iterator, Body, Item>;

    forward_block_handling_task(Iterator first, std::size_t size, feeder_type& feeder, small_object_allocator& alloc)
        : my_size(size), my_wait_context(0), my_feeder(feeder), my_allocator(alloc)
    {
        auto* task_it = task_pool.begin();
        for (std::size_t i = 0; i < size; i++) {
            new (task_it++) iteration_task(first, feeder, my_wait_context);
            ++first;
        }
    }

    void finalize(const execution_data& ed) {
        my_feeder.my_wait_context.release();
        my_allocator.delete_object(this, ed);
    }

    task* execute(execution_data& ed) override {
        __TBB_ASSERT( my_size > 0, "Negative size was passed to task");
        for(std::size_t counter = 1; counter < my_size; ++counter) {
            my_wait_context.reserve();
            spawn(*(task_pool.begin() + counter), my_feeder.my_execution_context);
        }
        my_wait_context.reserve();
        execute_and_wait(*task_pool.begin(), my_feeder.my_execution_context,
                         my_wait_context,    my_feeder.my_execution_context);

        // deallocate current task after children execution
        finalize(ed);
        return nullptr;
    }

    task* cancel(execution_data& ed) override {
        finalize(ed);
        return nullptr;
    }

    ~forward_block_handling_task() {
        for(std::size_t counter = 0; counter < my_size; ++counter) {
            (task_pool.begin() + counter)->~iteration_task();
        }
    }

    aligned_space<iteration_task, max_block_size> task_pool;
    std::size_t my_size;
    wait_context my_wait_context;
    feeder_type& my_feeder;
    small_object_allocator my_allocator;
}; // class forward_block_handling_task

/** Body for parallel_for algorithm.
  * Allows to redirect operations under random access iterators range to the parallel_for algorithm.
    @ingroup algorithms **/
template <typename Iterator, typename Body, typename Item>
class parallel_for_body_wrapper {
    using feeder_type = feeder_impl<Body, Item>;

    Iterator my_first;
    feeder_type& my_feeder;
public:
    parallel_for_body_wrapper(Iterator first, feeder_type& feeder) : my_first(first), my_feeder(feeder) {}

    void operator()(tbb::blocked_range<std::size_t> range) const {
#if __INTEL_COMPILER
#pragma ivdep
#endif
        for (std::size_t count = range.begin(); count != range.end(); count++) {
            parallel_for_each_operator_selector<Body, Item>::call(my_feeder.my_body, std::move(*(my_first + count)), my_feeder);
        }
    }
}; // class parallel_for_body_wrapper


/** Helper for getting iterators tag including inherited custom tags
    @ingroup algorithms */
template<typename It>
using tag = typename std::iterator_traits<It>::iterator_category;

template<typename It>
using iterator_tag_dispatch = typename
    std::conditional<
        std::is_base_of<std::random_access_iterator_tag, tag<It>>::value,
        std::random_access_iterator_tag,
        typename std::conditional<
            std::is_base_of<std::forward_iterator_tag, tag<It>>::value,
            std::forward_iterator_tag,
            std::input_iterator_tag
        >::type
    >::type;

/** parallel_for_each algorithm root task - most generic version
  * Splits input range to blocks
    @ingroup algorithms **/
template <typename Iterator, typename Body, typename Item, typename IteratorTag = iterator_tag_dispatch<Iterator>>
class for_each_root_task : public task {
    using feeder_type = feeder_impl<Body, Item>;

    Iterator my_first;
    Iterator my_last;
    feeder_type& my_feeder;
public:
    for_each_root_task(Iterator first, Iterator last, feeder_type& feeder) :
        my_first(first), my_last(last), my_feeder(feeder)
    {
        my_feeder.my_wait_context.reserve();
    }
private:
    task* execute(execution_data& ed) override {
        using block_handling_type = input_block_handling_task<Body, Item>;

        if (my_first == my_last) {
            my_feeder.my_wait_context.release();
            return nullptr;
        }

        my_feeder.my_wait_context.reserve();
        small_object_allocator alloc{};
        auto block_handling_task = alloc.new_object<block_handling_type>(ed, my_feeder, alloc);

        auto* block_iterator = block_handling_task->block_iteration_space.begin();
        for (; !(my_first == my_last) && block_handling_task->my_size < block_handling_type::max_block_size; ++my_first) {
            // Move semantics are automatically used when supported by the iterator
            new (block_iterator++) Item(*my_first);
            ++block_handling_task->my_size;
        }

        // Do not access this after spawn to avoid races
        spawn(*this, my_feeder.my_execution_context);
        return block_handling_task;
    }

    task* cancel(execution_data&) override {
        my_feeder.my_wait_context.release();
        return nullptr;
    }
}; // class for_each_root_task - most generic implementation

/** parallel_for_each algorithm root task - forward iterator based specialization
  * Splits input range to blocks
    @ingroup algorithms **/
template <typename Iterator, typename Body, typename Item>
class for_each_root_task<Iterator, Body, Item, std::forward_iterator_tag> : public task {
    using feeder_type = feeder_impl<Body, Item>;

    Iterator my_first;
    Iterator my_last;
    feeder_type& my_feeder;
public:
    for_each_root_task(Iterator first, Iterator last, feeder_type& feeder) :
        my_first(first), my_last(last), my_feeder(feeder)
    {
        my_feeder.my_wait_context.reserve();
    }
private:
    task* execute(execution_data& ed) override {
        using block_handling_type = forward_block_handling_task<Iterator, Body, Item>;
        if (my_first == my_last) {
            my_feeder.my_wait_context.release();
            return nullptr;
        }

        std::size_t block_size{0};
        Iterator first_block_element = my_first;
        for (; !(my_first == my_last) && block_size < block_handling_type::max_block_size; ++my_first) {
            ++block_size;
        }

        my_feeder.my_wait_context.reserve();
        small_object_allocator alloc{};
        auto block_handling_task = alloc.new_object<block_handling_type>(ed, first_block_element, block_size, my_feeder, alloc);

        // Do not access this after spawn to avoid races
        spawn(*this, my_feeder.my_execution_context);
        return block_handling_task;
    }

    task* cancel(execution_data&) override {
        my_feeder.my_wait_context.release();
        return nullptr;
    }
}; // class for_each_root_task - forward iterator based specialization


/** parallel_for_each algorithm root task - random access iterator based specialization
  * Splits input range to blocks
    @ingroup algorithms **/
template <typename Iterator, typename Body, typename Item>
class for_each_root_task<Iterator, Body, Item, std::random_access_iterator_tag> : public task {
    using feeder_type = feeder_impl<Body, Item>;

    Iterator my_first;
    Iterator my_last;
    feeder_type& my_feeder;
public:
    for_each_root_task(Iterator first, Iterator last, feeder_type& feeder) :
        my_first(first), my_last(last), my_feeder(feeder)
    {
        my_feeder.my_wait_context.reserve();
    }
private:
    task* execute(execution_data&) override {
        tbb::parallel_for(
            tbb::blocked_range<std::size_t>(0, std::distance(my_first, my_last)),
            parallel_for_body_wrapper<Iterator, Body, Item>(my_first, my_feeder)
            , my_feeder.my_execution_context
        );

        my_feeder.my_wait_context.release();
        return nullptr;
    }

    task* cancel(execution_data&) override {
        my_feeder.my_wait_context.release();
        return nullptr;
    }
}; // class for_each_root_task - random access iterator based specialization

/** Helper for getting item type. If item type can be deduced from feeder - got it from feeder,
    if feeder is generic - got item type from range.
    @ingroup algorithms */
template<typename Body, typename Item, typename FeederArg>
auto feeder_argument_parser(void (Body::*)(Item, feeder<FeederArg>&) const) -> FeederArg;

template<typename Body, typename>
decltype(feeder_argument_parser<Body>(&Body::operator())) get_item_type_impl(int); // for (T, feeder<T>)
template<typename Body, typename Item> Item get_item_type_impl(...); // stub

template <typename Body, typename Item>
using get_item_type = decltype(get_item_type_impl<Body, Item>(0));

/** Implements parallel iteration over a range.
    @ingroup algorithms */
template<typename Iterator, typename Body>
void run_parallel_for_each( Iterator first, Iterator last, const Body& body, task_group_context& context)
{
    if (!(first == last)) {
        using ItemType = get_item_type<Body, typename std::decay<decltype(*first)>::type>;
        feeder_impl<Body, ItemType> feeder(context, body);

        for_each_root_task<Iterator, Body, ItemType> root_task(first, last, feeder);

        execute_and_wait(root_task, context, feeder.my_wait_context, context);
    }
}

/** \page parallel_for_each_body_req Requirements on parallel_for_each body
    Class \c Body implementing the concept of parallel_for_each body must define:
    - \code
        B::operator()(
                cv_item_type item,
                feeder<item_type>& feeder
        ) const

        OR

        B::operator()( cv_item_type& item ) const
      \endcode                                               Process item.
                                                             May be invoked concurrently  for the same \c this but different \c item.

    - \code item_type( const item_type& ) \endcode
                                                             Copy a work item.
    - \code ~item_type() \endcode                            Destroy a work item
**/

/** \name parallel_for_each
    See also requirements on \ref parallel_for_each_body_req "parallel_for_each Body". **/
//@{
//! Parallel iteration over a range, with optional addition of more work.
/** @ingroup algorithms */
template<typename Iterator, typename Body>
void parallel_for_each(Iterator first, Iterator last, const Body& body) {
    task_group_context context(PARALLEL_FOR_EACH);
    run_parallel_for_each<Iterator, Body>(first, last, body, context);
}

template<typename Range, typename Body>
void parallel_for_each(Range& rng, const Body& body) {
    parallel_for_each(std::begin(rng), std::end(rng), body);
}

template<typename Range, typename Body>
void parallel_for_each(const Range& rng, const Body& body) {
    parallel_for_each(std::begin(rng), std::end(rng), body);
}

//! Parallel iteration over a range, with optional addition of more work and user-supplied context
/** @ingroup algorithms */
template<typename Iterator, typename Body>
void parallel_for_each(Iterator first, Iterator last, const Body& body, task_group_context& context) {
    run_parallel_for_each<Iterator, Body>(first, last, body, context);
}

template<typename Range, typename Body>
void parallel_for_each(Range& rng, const Body& body, task_group_context& context) {
    parallel_for_each(std::begin(rng), std::end(rng), body, context);
}

template<typename Range, typename Body>
void parallel_for_each(const Range& rng, const Body& body, task_group_context& context) {
    parallel_for_each(std::begin(rng), std::end(rng), body, context);
}

} // namespace d1
} // namespace detail
//! @endcond
//@}

inline namespace v1 {
using detail::d1::parallel_for_each;
using detail::d1::feeder;
} // namespace v1

} // namespace tbb

#endif /* __TBB_parallel_for_each_H */
