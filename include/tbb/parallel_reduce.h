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

#ifndef __TBB_parallel_reduce_H
#define __TBB_parallel_reduce_H

#define __TBB_parallel_reduce_H_include_area
#include "internal/_warning_suppress_enable_notice.h"

// 0: direct use of aligned_space
// 1: std::optional backported on top of aligned_space (to C++11, already assumed by the relevant code)
// 2: std::optional
// Note that, while 1/2 is more elegant than 0, for each contiguous range a placement new is replaced with
// an assignment to an empty std::optional, at the cost of an extra move-constructor/destructor pair.
// This means that the caller must provide a suitable type or incur a penalty for non-optimal move-construction,
// and therefore it seems better to avoid using std::optional.
// TODO: is this an implementation issue with compiler and/or std::optional, or inevitable?
#define __TBB_PARALLEL_REDUCE_EXPERIMENTALLY_USE_OPTIONAL 0

#include <new>
#if __TBB_PARALLEL_REDUCE_EXPERIMENTALLY_USE_OPTIONAL == 2
#include <optional>
#endif
#include "task.h"
#include "aligned_space.h"
#include "partitioner.h"
#include "tbb_profiling.h"

// Overloads for semigroups are of the form "auto f(...) -> decltype(...)", with "decltype(...)" needed prior to C++14.
#define __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS __TBB_CPP11_DECLTYPE_PRESENT

#if COMPILATION_UNIT_TEST_PARALLEL_REDUCE_CPP
extern tbb::atomic<unsigned long> counter_aeo;
#endif

namespace tbb {

namespace interface9 {
//! @cond INTERNAL
namespace internal {

    using namespace tbb::internal;

    /** Values for reduction_context. */
    enum {
        root_task, left_child, right_child
    };

    /** Represented as a char, not enum, for compactness. */
    typedef char reduction_context;

    //! Task type used to combine the partial results of parallel_reduce.
    /** @ingroup algorithms */
    template<typename Body>
    class finish_reduce: public flag_task {
        bool has_right_zombie;
        const reduction_context my_context;
        //! Pointer to body, or NULL if the left child has not yet finished.
        Body* my_body;
        aligned_space<Body> zombie_space;
        finish_reduce( reduction_context context_ ) :
            has_right_zombie(false), // TODO: substitute by flag_task::child_stolen?
            my_context(context_),
            my_body(NULL)
        {
        }
        ~finish_reduce() {
            if( has_right_zombie )
                zombie_space.begin()->~Body();
        }
        task* execute() __TBB_override {
            if( has_right_zombie ) {
                // Right child was stolen.
                Body* s = zombie_space.begin();
                my_body->join( *s );
                // Body::join() won't be called if canceled. Defer destruction to destructor
            }
            if( my_context==left_child )
                itt_store_word_with_release( static_cast<finish_reduce*>(parent())->my_body, my_body );
            return NULL;
        }
        template<typename Range, typename Body_, typename Partitioner>
        friend class start_reduce;
    };

    //! allocate right task with new parent
    void allocate_sibling(task* start_reduce_task, task *tasks[], size_t start_bytes, size_t finish_bytes);

    //! Task type used to split the work of parallel_reduce.
    /** @ingroup algorithms */
    template<typename Range, typename Body, typename Partitioner>
    class start_reduce: public task {
        typedef finish_reduce<Body> finish_type;
        Body* my_body;
        Range my_range;
        typename Partitioner::task_partition_type my_partition;
        reduction_context my_context;
        task* execute() __TBB_override;
        //! Update affinity info, if any
        void note_affinity( affinity_id id ) __TBB_override {
            my_partition.note_affinity( id );
        }
        template<typename Body_>
        friend class finish_reduce;

public:
        //! Constructor used for root task
        start_reduce( const Range& range, Body* body, Partitioner& partitioner ) :
            my_body(body),
            my_range(range),
            my_partition(partitioner),
            my_context(root_task)
        {
        }
        //! Splitting constructor used to generate children.
        /** parent_ becomes left child.  Newly constructed object is right child. */
        start_reduce( start_reduce& parent_, typename Partitioner::split_type& split_obj ) :
            my_body(parent_.my_body),
            my_range(parent_.my_range, split_obj),
            my_partition(parent_.my_partition, split_obj),
            my_context(right_child)
        {
            my_partition.set_affinity(*this);
            parent_.my_context = left_child;
        }
        //! Construct right child from the given range as response to the demand.
        /** parent_ remains left child.  Newly constructed object is right child. */
        start_reduce( start_reduce& parent_, const Range& r, depth_t d ) :
            my_body(parent_.my_body),
            my_range(r),
            my_partition(parent_.my_partition, split()),
            my_context(right_child)
        {
            my_partition.set_affinity(*this);
            my_partition.align_depth( d ); // TODO: move into constructor of partitioner
            parent_.my_context = left_child;
        }
        static void run( const Range& range, Body& body, Partitioner& partitioner ) {
            if( !range.empty() ) {
#if !__TBB_TASK_GROUP_CONTEXT || TBB_JOIN_OUTER_TASK_GROUP
                task::spawn_root_and_wait( *new(task::allocate_root()) start_reduce(range, &body, partitioner) );
#else
                // Bound context prevents exceptions from body to affect nesting or sibling algorithms,
                // and allows users to handle exceptions safely by wrapping parallel_for in the try-block.
                task_group_context context(PARALLEL_REDUCE);
                task::spawn_root_and_wait( *new(task::allocate_root(context)) start_reduce(range, &body, partitioner) );
#endif /* __TBB_TASK_GROUP_CONTEXT && !TBB_JOIN_OUTER_TASK_GROUP */
            }
        }
#if __TBB_TASK_GROUP_CONTEXT
        static void run( const Range& range, Body& body, Partitioner& partitioner, task_group_context& context ) {
            if( !range.empty() )
                task::spawn_root_and_wait( *new(task::allocate_root(context)) start_reduce(range, &body, partitioner) );
        }
#endif /* __TBB_TASK_GROUP_CONTEXT */
        //! Run body for range
        void run_body( Range &r ) { (*my_body)( r ); }

        //! spawn right task, serves as callback for partitioner
        // TODO: remove code duplication from 'offer_work' methods
        void offer_work(typename Partitioner::split_type& split_obj) {
            task *tasks[2];
            allocate_sibling(static_cast<task*>(this), tasks, sizeof(start_reduce), sizeof(finish_type));
            new((void*)tasks[0]) finish_type(my_context);
            new((void*)tasks[1]) start_reduce(*this, split_obj);
            spawn(*tasks[1]);
        }
        //! spawn right task, serves as callback for partitioner
        void offer_work(const Range& r, depth_t d = 0) {
            task *tasks[2];
            allocate_sibling(static_cast<task*>(this), tasks, sizeof(start_reduce), sizeof(finish_type));
            new((void*)tasks[0]) finish_type(my_context);
            new((void*)tasks[1]) start_reduce(*this, r, d);
            spawn(*tasks[1]);
        }
    };

    //! allocate right task with new parent
    // TODO: 'inline' here is to avoid multiple definition error but for sake of code size this should not be inlined
    inline void allocate_sibling(task* start_reduce_task, task *tasks[], size_t start_bytes, size_t finish_bytes) {
        tasks[0] = &start_reduce_task->allocate_continuation().allocate(finish_bytes);
        start_reduce_task->set_parent(tasks[0]);
        tasks[0]->set_ref_count(2);
        tasks[1] = &tasks[0]->allocate_child().allocate(start_bytes);
    }

    template<typename Range, typename Body, typename Partitioner>
    task* start_reduce<Range, Body, Partitioner>::execute() {
        my_partition.check_being_stolen( *this );
        if( my_context==right_child ) {
            finish_type* parent_ptr = static_cast<finish_type*>(parent());
            if( !itt_load_word_with_acquire(parent_ptr->my_body) ) { // TODO: replace by is_stolen_task() or by parent_ptr->ref_count() == 2???
                my_body = new( parent_ptr->zombie_space.begin() ) Body(*my_body, split());
                parent_ptr->has_right_zombie = true;
            }
        } else __TBB_ASSERT(my_context==root_task, NULL);// because left leaf spawns right leafs without recycling
        my_partition.execute(*this, my_range);
        if( my_context==left_child ) {
            finish_type* parent_ptr = static_cast<finish_type*>(parent());
            __TBB_ASSERT(my_body!=parent_ptr->zombie_space.begin(), NULL);
            itt_store_word_with_release(parent_ptr->my_body, my_body );
        }
        return NULL;
    }

    //! Task type used to combine the partial results of parallel_deterministic_reduce.
    /** @ingroup algorithms */
    template<typename Body>
    class finish_deterministic_reduce: public task {
        Body &my_left_body;
        Body my_right_body;

        finish_deterministic_reduce( Body &body ) :
            my_left_body( body ),
            my_right_body( body, split() )
        {
        }
        task* execute() __TBB_override {
            my_left_body.join( my_right_body );
            return NULL;
        }
        template<typename Range, typename Body_, typename Partitioner>
        friend class start_deterministic_reduce;
    };

    //! Task type used to split the work of parallel_deterministic_reduce.
    /** @ingroup algorithms */
    template<typename Range, typename Body, typename Partitioner>
    class start_deterministic_reduce: public task {
        typedef finish_deterministic_reduce<Body> finish_type;
        Body &my_body;
        Range my_range;
        typename Partitioner::task_partition_type my_partition;
        task* execute() __TBB_override;

        //! Constructor used for root task
        start_deterministic_reduce( const Range& range, Body& body, Partitioner& partitioner ) :
            my_body( body ),
            my_range( range ),
            my_partition( partitioner )
        {
        }
        //! Splitting constructor used to generate children.
        /** parent_ becomes left child.  Newly constructed object is right child. */
        start_deterministic_reduce( start_deterministic_reduce& parent_, finish_type& c, typename Partitioner::split_type& split_obj ) :
            my_body( c.my_right_body ),
            my_range( parent_.my_range, split_obj ),
            my_partition( parent_.my_partition, split_obj )
        {
        }

public:
        static void run( const Range& range, Body& body, Partitioner& partitioner ) {
            if( !range.empty() ) {
#if !__TBB_TASK_GROUP_CONTEXT || TBB_JOIN_OUTER_TASK_GROUP
                task::spawn_root_and_wait( *new(task::allocate_root()) start_deterministic_reduce(range, &body, partitioner) );
#else
                // Bound context prevents exceptions from body to affect nesting or sibling algorithms,
                // and allows users to handle exceptions safely by wrapping parallel_for in the try-block.
                task_group_context context(PARALLEL_REDUCE);
                task::spawn_root_and_wait( *new(task::allocate_root(context)) start_deterministic_reduce(range, body, partitioner) );
#endif /* __TBB_TASK_GROUP_CONTEXT && !TBB_JOIN_OUTER_TASK_GROUP */
            }
        }
#if __TBB_TASK_GROUP_CONTEXT
        static void run( const Range& range, Body& body, Partitioner& partitioner, task_group_context& context ) {
            if( !range.empty() )
                task::spawn_root_and_wait( *new(task::allocate_root(context)) start_deterministic_reduce(range, body, partitioner) );
        }
#endif /* __TBB_TASK_GROUP_CONTEXT */

        void offer_work( typename Partitioner::split_type& split_obj) {
            task* tasks[2];
            allocate_sibling(static_cast<task*>(this), tasks, sizeof(start_deterministic_reduce), sizeof(finish_type));
            new((void*)tasks[0]) finish_type(my_body);
            new((void*)tasks[1]) start_deterministic_reduce(*this, *static_cast<finish_type*>(tasks[0]), split_obj);
            spawn(*tasks[1]);
        }

        void run_body( Range &r ) { my_body(r); }
    };

    template<typename Range, typename Body, typename Partitioner>
    task* start_deterministic_reduce<Range, Body, Partitioner>::execute() {
        my_partition.execute(*this, my_range);
        return NULL;
    }
} // namespace internal
//! @endcond
} //namespace interfaceX

// Until revoked, this guarantees calls to be of the form "x = binary_operation(std::move(x), std::move(y));"
// (with x on the left) and "x = range_operation(range, std::move(x));" (or both without std::move() before C++11),
// allowing the operations to modify the parameter referencing "x" in place and then return it by reference,
// so that the reduction type (or any of its components) can benefit from elision of any expensive self-assignment,
// i.e., where the copy assignment operator's implementation is explicitly predicated on "this != &that".
// For more details about usage and benefits see the unit tests.
// Usage of this guarantee is deprecated for code that can always rely on C++11 move semantics instead,
// although code that does use it and that remains under your direct control can probably delay implementation
// of the alternative until revocation is observed (#else #error).
// Note that the interface could have allowed to manipulate the argument in-place without returning it,
// but that's water under the bridge now.
#define TBB_PARALLEL_REDUCE_PRE_CPP11_RVALUE_REF_FORM_GUARANTEE 1

//! @cond INTERNAL
namespace internal {
    using interface9::internal::start_reduce;
    using interface9::internal::start_deterministic_reduce;

    //! Auxiliary class for parallel_reduce; for internal use only.
    /** The adaptor class that implements \ref parallel_reduce_body_req "parallel_reduce Body"
        using given \ref parallel_reduce_lambda_req closure objects (or plain functions or function objects).
     **/
    /** @ingroup algorithms */
    template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation>
    class reduce_body_lambda_monoid : no_copy {

//FIXME: decide if my_identity, my_range_operation, and my_binary_operation should be copied or referenced
//       (might require some performance measurements)

        const Value&           my_identity;
        const RangeOperation&  my_range_operation;
        const BinaryOperation& my_binary_operation;
        Value                  my_value;
    public:
        reduce_body_lambda_monoid( const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation )
            : my_identity        (identity)
            , my_range_operation (range_operation)
            , my_binary_operation(binary_operation)
            , my_value           (identity)
        { }
        reduce_body_lambda_monoid( reduce_body_lambda_monoid& other, tbb::split )
            : my_identity        (other.my_identity)
            , my_range_operation (other.my_range_operation)
            , my_binary_operation(other.my_binary_operation)
            , my_value           (other.my_identity)
        { }
        void operator()(Range& range) {
            // implements part of TBB_PARALLEL_REDUCE_PRE_CPP11_RVALUE_REF_FORM_GUARANTEE
            my_value = my_range_operation(range, tbb::internal::move(my_value));
        }
        void join( reduce_body_lambda_monoid& rhs ) {
            // implements part of TBB_PARALLEL_REDUCE_PRE_CPP11_RVALUE_REF_FORM_GUARANTEE
            my_value = my_binary_operation(tbb::internal::move(my_value), tbb::internal::move(rhs.my_value));
        }
        Value& result() {
            return my_value;
        }
    };

#if __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS

    // With a Value that is costly to create, it might be more beneficial to use either a plain Body
    // or an overload with an identity parameter (perhaps even by extending the source domain if needed).
    // TODO: investigate the circumstances in which this might be so and formulate concrete advice

#if __TBB_PARALLEL_REDUCE_EXPERIMENTALLY_USE_OPTIONAL == 1
    // backported std::optional (only the features that are actually used)
    template<typename Value>
    class __TBB_optional : internal::no_copy {
        Value*               my_value;
        aligned_space<Value> my_value_space;
    public:
        __TBB_optional() : my_value(nullptr) {}
        ~__TBB_optional() {
            if (my_value) {
                my_value->~Value();
            } else {
                __TBB_ASSERT(false, NULL); // in this context, all instances happen to be assigned to
            }
        }
        __TBB_optional& operator=(Value&& value) {
            if (my_value) {
                __TBB_ASSERT(false, NULL); // in this context, all assignments happen to be to empty instances...
                my_value->~Value(); // ...but otherwise this would be necessary and sufficient
            }
            my_value = new( my_value_space.begin() ) Value(std::move(value));
            return *this;
        }
        Value& operator*() {
            // TODO: test "my_value != nullptr"?
            return *my_value;
        }
        operator bool() const {
            return my_value != nullptr;
        }
    };
#elif __TBB_PARALLEL_REDUCE_EXPERIMENTALLY_USE_OPTIONAL == 2
    template<typename T>
    using __TBB_optional = std::optional<T>;
#endif

    //! Auxiliary class for parallel_reduce; for internal use only.
    /** The adaptor class that implements \ref parallel_reduce_body_req "parallel_reduce Body"
        using given \ref parallel_reduce_lambda_req closure objects (or plain functions or function objects).
     **/
    /** @ingroup algorithms */
    template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation>
    class reduce_body_lambda_semigroup : no_copy {

//FIXME: decide if my_range_operation and my_binary_operation should be copied or referenced
//       (might require some performance measurements)

        const RangeOperation&  my_range_operation;
        const BinaryOperation& my_binary_operation;
        #if !__TBB_PARALLEL_REDUCE_EXPERIMENTALLY_USE_OPTIONAL
        Value*                 my_value;
        aligned_space<Value>   my_value_space;
        #else
        __TBB_optional<Value>  my_value;
        #endif
    public:
        reduce_body_lambda_semigroup( const RangeOperation& range_operation, const BinaryOperation& binary_operation )
            : my_range_operation (range_operation)
            , my_binary_operation(binary_operation)
            , my_value           () // nullptr/nullopt
        { }
        reduce_body_lambda_semigroup( reduce_body_lambda_semigroup& other, tbb::split )
            : my_range_operation (other.my_range_operation)
            , my_binary_operation(other.my_binary_operation)
            , my_value           () // nullptr/nullopt
        { }
        ~reduce_body_lambda_semigroup() {
            #if !__TBB_PARALLEL_REDUCE_EXPERIMENTALLY_USE_OPTIONAL
            if (my_value) my_value->~Value();
            #endif
        }
        void operator()(Range& range) {
            if (range.empty()) {
                // TODO: can this happen? could the user have provided a defective Range that causes this?
                //       should we throw ("fail early"), or defer to my_range_operation to either return identity or throw?
                throw std::runtime_error("Unexpected: empty range in reduce_body_lambda_semigroup::operator()");
            } else if (!my_value) {
                #if !__TBB_PARALLEL_REDUCE_EXPERIMENTALLY_USE_OPTIONAL
                my_value = new( my_value_space.begin() ) Value(my_range_operation(range));
                #else
                #if COMPILATION_UNIT_TEST_PARALLEL_REDUCE_CPP
                ++counter_aeo;
                #endif
                my_value =                                     my_range_operation(range) ;
                #endif
            } else {
                *my_value = my_binary_operation(tbb::internal::move(*my_value), my_range_operation(range));
            }
        }
        void join( reduce_body_lambda_semigroup& rhs ) {
            __TBB_ASSERT(my_value && rhs.my_value, NULL); // TODO: explain, or else test to throw logic_error anyway?
            *my_value = my_binary_operation(tbb::internal::move(*my_value), tbb::internal::move(*rhs.my_value));
        }
        Value& result() {
            if (my_value) {
                return *my_value;
            } else {
                throw std::invalid_argument("range must not be empty"); // TODO: or rather catch before this adapter is ever invoked?
            }
        }
    };

#endif /* __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS */

} // namespace internal
//! @endcond

// Requirements on Range concept are documented in blocked_range.h

/** \page parallel_reduce_body_req Requirements on parallel_reduce body
    Class \c Body implementing the concept of parallel_reduce body must define:
    - \code Body::Body( Body&, split ); \endcode        Splitting constructor.
                                                        Must be able to run concurrently with operator() and method \c join
    - \code Body::~Body(); \endcode                     Destructor
    - \code void Body::operator()( Range& r ); \endcode Function call operator applying body to range \c r
                                                        and accumulating the result
    - \code void Body::join( Body& b ); \endcode        Join results.
                                                        The result in \c b should be merged into the result of \c this
**/

/** \page parallel_reduce_lambda_req Requirements on parallel_reduce anonymous function objects (lambda functions)
    TO BE DOCUMENTED
**/

/** \name parallel_reduce
    See also requirements on \ref range_req "Range" and \ref parallel_reduce_body_req "parallel_reduce Body". **/
//@{

//! Parallel iteration with reduction and default partitioner.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_reduce( const Range& range, Body& body ) {
    parallel_reduce( range, body, __TBB_DEFAULT_PARTITIONER() );
}

//! Parallel iteration with reduction and isolated-action partitioner
/** @ingroup algorithms **/
template<typename Range, typename Body, typename Partitioner>
typename internal::is_isolated_action_partitioner_t<Partitioner, void>::type
parallel_reduce( const Range& range, Body& body, const Partitioner& partitioner ) {
    internal::start_reduce<Range, Body, const Partitioner>::run( range, body, partitioner );
}

//! Parallel iteration with reduction and affinity_partitioner
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_reduce( const Range& range, Body& body, affinity_partitioner& partitioner ) {
    internal::start_reduce<Range, Body, affinity_partitioner>::run( range, body, partitioner );
}

#if __TBB_TASK_GROUP_CONTEXT

//! Parallel iteration with reduction, default partitioner and user-supplied context.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_reduce( const Range& range, Body& body, task_group_context& context ) {
    parallel_reduce( range, body, __TBB_DEFAULT_PARTITIONER(), context );
}

//! Parallel iteration with reduction, isolated-action partitioner and user-supplied context
/** @ingroup algorithms **/
template<typename Range, typename Body, typename Partitioner>
typename internal::is_isolated_action_partitioner_t<Partitioner, void>::type
parallel_reduce( const Range& range, Body& body, const Partitioner& partitioner, task_group_context& context ) {
    internal::start_reduce<Range, Body, const Partitioner>::run( range, body, partitioner, context );
}

//! Parallel iteration with reduction, affinity_partitioner and user-supplied context
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_reduce( const Range& range, Body& body, affinity_partitioner& partitioner, task_group_context& context ) {
    internal::start_reduce<Range, Body, affinity_partitioner>::run( range, body, partitioner, context );
}

#endif /* __TBB_TASK_GROUP_CONTEXT */

/** parallel_reduce overloads that work with anonymous function objects with identity parameter
    (see also \ref parallel_reduce_lambda_req "requirements on parallel_reduce anonymous function objects"). **/

//! Parallel iteration with reduction and default partitioner.
/** @ingroup algorithms **/
template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation>
Value parallel_reduce( const Range& range, const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation ) {
    return parallel_reduce( range, identity, range_operation, binary_operation, __TBB_DEFAULT_PARTITIONER() );
}

//! Parallel iteration with reduction and isolated-action partitioner
/** @ingroup algorithms **/
template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation, typename Partitioner>
typename internal::is_isolated_action_partitioner_t<Partitioner, Value>::type
parallel_reduce( const Range& range, const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                 const Partitioner& partitioner ) {
    return __TBB_impl_parallel_reduce( range, identity, range_operation, binary_operation, partitioner );
}

//! Parallel iteration with reduction and affinity_partitioner
/** @ingroup algorithms **/
template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation>
Value parallel_reduce( const Range& range, const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                       affinity_partitioner& partitioner ) {
    return __TBB_impl_parallel_reduce( range, identity, range_operation, binary_operation, partitioner );
}

template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation, typename Partitioner>
Value __TBB_impl_parallel_reduce( const Range& range, const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                                  Partitioner& partitioner ) {
    internal::reduce_body_lambda_monoid<Range, Value, RangeOperation, BinaryOperation> body(identity, range_operation, binary_operation);
    internal::start_reduce<Range, internal::reduce_body_lambda_monoid<Range, Value, RangeOperation, BinaryOperation>, Partitioner>
                ::run( range, body, partitioner );
    return tbb::internal::move(body.result());
}

#if __TBB_TASK_GROUP_CONTEXT

//! Parallel iteration with reduction, default partitioner and user-supplied context.
/** @ingroup algorithms **/
template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation>
Value parallel_reduce( const Range& range, const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                       task_group_context& context ) {
    return parallel_reduce( range, identity, range_operation, binary_operation, __TBB_DEFAULT_PARTITIONER(), context );
}

//! Parallel iteration with reduction, isolated-action partitioner and user-supplied context
/** @ingroup algorithms **/
template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation, typename Partitioner>
typename internal::is_isolated_action_partitioner_t<Partitioner, Value>::type
parallel_reduce( const Range& range, const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                 const Partitioner& partitioner, task_group_context& context ) {
    return __TBB_impl_parallel_reduce( range, identity, range_operation, binary_operation, partitioner, context );
}

//! Parallel iteration with reduction, affinity_partitioner and user-supplied context
/** @ingroup algorithms **/
template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation>
Value parallel_reduce( const Range& range, const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                       affinity_partitioner& partitioner, task_group_context& context ) {
    return __TBB_impl_parallel_reduce( range, identity, range_operation, binary_operation, partitioner, context );
}

template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation, typename Partitioner>
Value __TBB_impl_parallel_reduce( const Range& range, const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                                  Partitioner& partitioner, task_group_context& context ) {
    internal::reduce_body_lambda_monoid<Range, Value, RangeOperation, BinaryOperation> body(identity, range_operation, binary_operation);
    internal::start_reduce<Range, internal::reduce_body_lambda_monoid<Range, Value, RangeOperation, BinaryOperation>, Partitioner>
                ::run( range, body, partitioner, context );
    return tbb::internal::move(body.result());
}

#endif /* __TBB_TASK_GROUP_CONTEXT */

#if __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS

// TODO: require that both operations return the same type?
//       or more freely that the return type of range_operation is convertible to the return type of binary_operation,
//       and then return the type that binary_operation returns?

/** parallel_reduce overloads that work with anonymous function objects without identity parameter
    (see also \ref parallel_reduce_lambda_req "requirements on parallel_reduce anonymous function objects"). **/

//! Parallel iteration with reduction and default partitioner.
/** @ingroup algorithms **/
template<typename Range, typename RangeOperation, typename BinaryOperation>
auto parallel_reduce1( const Range& range, const RangeOperation& range_operation, const BinaryOperation& binary_operation )
-> decltype(range_operation(range))
{
    return parallel_reduce1( range, range_operation, binary_operation, __TBB_DEFAULT_PARTITIONER() );
}

//! Parallel iteration with reduction and isolated-action partitioner
/** @ingroup algorithms **/
template<typename Range, typename RangeOperation, typename BinaryOperation, typename Partitioner>
auto parallel_reduce1( const Range& range, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                       const Partitioner& partitioner )
-> typename internal::is_isolated_action_partitioner_t<Partitioner, decltype(range_operation(range))>::type
{
    return __TBB_impl_parallel_reduce1( range, range_operation, binary_operation, partitioner );
}

//! Parallel iteration with reduction and affinity_partitioner
/** @ingroup algorithms **/
template<typename Range, typename RangeOperation, typename BinaryOperation>
auto parallel_reduce1( const Range& range, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                       affinity_partitioner& partitioner )
-> decltype(range_operation(range))
{
    return __TBB_impl_parallel_reduce1( range, range_operation, binary_operation, partitioner );
}

template<typename Range, typename RangeOperation, typename BinaryOperation, typename Partitioner>
auto __TBB_impl_parallel_reduce1( const Range& range, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                                  Partitioner& partitioner )
-> decltype(range_operation(range))
{
    internal::reduce_body_lambda_semigroup<Range, decltype(range_operation(range)), RangeOperation, BinaryOperation> body(range_operation, binary_operation);
    internal::start_reduce<Range, internal::reduce_body_lambda_semigroup<Range, decltype(range_operation(range)), RangeOperation, BinaryOperation>, Partitioner>
                ::run( range, body, partitioner );
    return tbb::internal::move(body.result());
}

#if __TBB_TASK_GROUP_CONTEXT

//! Parallel iteration with reduction, default partitioner and user-supplied context.
/** @ingroup algorithms **/
template<typename Range, typename RangeOperation, typename BinaryOperation>
auto parallel_reduce1( const Range& range, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                       task_group_context& context )
-> decltype(range_operation(range))
{
    return parallel_reduce1( range, range_operation, binary_operation, __TBB_DEFAULT_PARTITIONER(), context );
}

//! Parallel iteration with reduction, isolated-action partitioner and user-supplied context
/** @ingroup algorithms **/
template<typename Range, typename RangeOperation, typename BinaryOperation, typename Partitioner>
auto parallel_reduce1( const Range& range, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                       const Partitioner& partitioner, task_group_context& context )
-> typename internal::is_isolated_action_partitioner_t<Partitioner, decltype(range_operation(range))>::type
{
    return __TBB_impl_parallel_reduce1( range, range_operation, binary_operation, partitioner, context );
}

//! Parallel iteration with reduction, affinity_partitioner and user-supplied context
/** @ingroup algorithms **/
template<typename Range, typename RangeOperation, typename BinaryOperation>
auto parallel_reduce1( const Range& range, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                       affinity_partitioner& partitioner, task_group_context& context )
-> decltype(range_operation(range))
{
    return __TBB_impl_parallel_reduce1( range, range_operation, binary_operation, partitioner, context );
}

template<typename Range, typename RangeOperation, typename BinaryOperation, typename Partitioner>
auto __TBB_impl_parallel_reduce1( const Range& range, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                                  Partitioner& partitioner, task_group_context& context )
-> decltype(range_operation(range))
{
    internal::reduce_body_lambda_semigroup<Range, decltype(range_operation(range)), RangeOperation, BinaryOperation> body(range_operation, binary_operation);
    internal::start_reduce<Range, internal::reduce_body_lambda_semigroup<Range, decltype(range_operation(range)), RangeOperation, BinaryOperation>, Partitioner>
                ::run( range, body, partitioner, context );
    return tbb::internal::move(body.result());
}

#endif /* __TBB_TASK_GROUP_CONTEXT */

#endif /* __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS */

//! Parallel iteration with deterministic reduction and default simple_partitioner.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_deterministic_reduce( const Range& range, Body& body ) {
    parallel_deterministic_reduce(range, body, simple_partitioner());
}

//! Parallel iteration with deterministic reduction and deterministic partitioner.
/** @ingroup algorithms **/
template<typename Range, typename Body, typename Partitioner>
typename internal::is_deterministic_partitioner_t<Partitioner, void>::type
parallel_deterministic_reduce( const Range& range, Body& body, const Partitioner& partitioner ) {
    internal::start_deterministic_reduce<Range, Body, const Partitioner>::run(range, body, partitioner);
}

#if __TBB_TASK_GROUP_CONTEXT

//! Parallel iteration with deterministic reduction, default simple_partitioner and user-supplied context.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_deterministic_reduce( const Range& range, Body& body, task_group_context& context ) {
    parallel_deterministic_reduce( range, body, simple_partitioner(), context );
}

//! Parallel iteration with deterministic reduction, deterministic partitioner and user-supplied context.
/** @ingroup algorithms **/
template<typename Range, typename Body, typename Partitioner>
typename internal::is_deterministic_partitioner_t<Partitioner, void>::type
parallel_deterministic_reduce( const Range& range, Body& body, const Partitioner& partitioner, task_group_context& context ) {
    internal::start_deterministic_reduce<Range, Body, const Partitioner>::run(range, body, partitioner, context);
}

#endif /* __TBB_TASK_GROUP_CONTEXT */

/** parallel_reduce overloads that work with anonymous function objects with identity parameter
    (see also \ref parallel_reduce_lambda_req "requirements on parallel_reduce anonymous function objects"). **/

//! Parallel iteration with deterministic reduction and default simple_partitioner.
// TODO: consider making static_partitioner the default
/** @ingroup algorithms **/
template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation>
Value parallel_deterministic_reduce( const Range& range, const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation ) {
    return parallel_deterministic_reduce(range, identity, range_operation, binary_operation, simple_partitioner());
}

//! Parallel iteration with deterministic reduction and deterministic partitioner.
/** @ingroup algorithms **/
template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation, typename Partitioner>
typename internal::is_deterministic_partitioner_t<Partitioner, Value>::type
parallel_deterministic_reduce( const Range& range, const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation, const Partitioner& partitioner ) {
    internal::reduce_body_lambda_monoid<Range, Value, RangeOperation, BinaryOperation> body(identity, range_operation, binary_operation);
    internal::start_deterministic_reduce<Range, internal::reduce_body_lambda_monoid<Range, Value, RangeOperation, BinaryOperation>, const Partitioner>
                ::run(range, body, partitioner);
    return tbb::internal::move(body.result());
}

#if __TBB_TASK_GROUP_CONTEXT

//! Parallel iteration with deterministic reduction, default simple_partitioner and user-supplied context.
/** @ingroup algorithms **/
template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation>
Value parallel_deterministic_reduce( const Range& range, const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                                     task_group_context& context ) {
    return parallel_deterministic_reduce(range, identity, range_operation, binary_operation, simple_partitioner(), context);
}

//! Parallel iteration with deterministic reduction, deterministic partitioner and user-supplied context.
/** @ingroup algorithms **/
template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation, typename Partitioner>
typename internal::is_deterministic_partitioner_t<Partitioner, Value>::type
parallel_deterministic_reduce( const Range& range, const Value& identity, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                               const Partitioner& partitioner, task_group_context& context ) {
    internal::reduce_body_lambda_monoid<Range, Value, RangeOperation, BinaryOperation> body(identity, range_operation, binary_operation);
    internal::start_deterministic_reduce<Range, internal::reduce_body_lambda_monoid<Range, Value, RangeOperation, BinaryOperation>, const Partitioner>
                ::run(range, body, partitioner, context);
    return tbb::internal::move(body.result());
}

#endif /* __TBB_TASK_GROUP_CONTEXT */

#if __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS

/** parallel_reduce overloads that work with anonymous function objects without identity parameter
    (see also \ref parallel_reduce_lambda_req "requirements on parallel_reduce anonymous function objects"). **/

//! Parallel iteration with deterministic reduction and default simple_partitioner.
// TODO: consider making static_partitioner the default
/** @ingroup algorithms **/
template<typename Range, typename RangeOperation, typename BinaryOperation>
auto parallel_deterministic_reduce1( const Range& range, const RangeOperation& range_operation, const BinaryOperation& binary_operation )
-> decltype(range_operation(range))
{
    return parallel_deterministic_reduce1(range, range_operation, binary_operation, simple_partitioner());
}

//! Parallel iteration with deterministic reduction and deterministic partitioner.
/** @ingroup algorithms **/
template<typename Range, typename RangeOperation, typename BinaryOperation, typename Partitioner>
auto parallel_deterministic_reduce1( const Range& range, const RangeOperation& range_operation, const BinaryOperation& binary_operation, const Partitioner& partitioner )
-> typename internal::is_deterministic_partitioner_t<Partitioner, decltype(range_operation(range))>::type
{
    internal::reduce_body_lambda_semigroup<Range, decltype(range_operation(range)), RangeOperation, BinaryOperation> body(range_operation, binary_operation);
    internal::start_deterministic_reduce<Range, internal::reduce_body_lambda_semigroup<Range, decltype(range_operation(range)), RangeOperation, BinaryOperation>, const Partitioner>
                ::run(range, body, partitioner);
    return tbb::internal::move(body.result());
}

#if __TBB_TASK_GROUP_CONTEXT

//! Parallel iteration with deterministic reduction, default simple_partitioner and user-supplied context.
/** @ingroup algorithms **/
template<typename Range, typename RangeOperation, typename BinaryOperation>
auto parallel_deterministic_reduce1( const Range& range, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                                     task_group_context& context )
-> decltype(range_operation(range))
{
    return parallel_deterministic_reduce1(range, range_operation, binary_operation, simple_partitioner(), context);
}

//! Parallel iteration with deterministic reduction, deterministic partitioner and user-supplied context.
/** @ingroup algorithms **/
template<typename Range, typename RangeOperation, typename BinaryOperation, typename Partitioner>
auto parallel_deterministic_reduce1( const Range& range, const RangeOperation& range_operation, const BinaryOperation& binary_operation,
                                     const Partitioner& partitioner, task_group_context& context )
-> typename internal::is_deterministic_partitioner_t<Partitioner, decltype(range_operation(range))>::type
{
    internal::reduce_body_lambda_semigroup<Range, decltype(range_operation(range)), RangeOperation, BinaryOperation> body(range_operation, binary_operation);
    internal::start_deterministic_reduce<Range, internal::reduce_body_lambda_semigroup<Range, decltype(range_operation(range)), RangeOperation, BinaryOperation>, const Partitioner>
                ::run(range, body, partitioner, context);
    return tbb::internal::move(body.result());
}

#endif /* __TBB_TASK_GROUP_CONTEXT */

#endif /* __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS */

//@}

} // namespace tbb

#include "internal/_warning_suppress_disable_notice.h"
#undef __TBB_parallel_reduce_H_include_area

#endif /* __TBB_parallel_reduce_H */
