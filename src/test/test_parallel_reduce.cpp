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

#define COMPILATION_UNIT_TEST_PARALLEL_REDUCE_CPP 1

#include <numeric> // std::accumulate()
#include "tbb/parallel_reduce.h"
#include "tbb/atomic.h"
#include "harness_assert.h"

#if __TBB_CPP11_DECLTYPE_PRESENT && !__TBB_CPP11_LAMBDAS_PRESENT
#error // semigroup overloads are not getting tested
#endif

static tbb::atomic<long> ForkCount;
static tbb::atomic<long> MinimalBodyCount;

//! Class with public interface that is exactly minimal requirements for Range concept
class MinimalRange {
    size_t begin, end;
    friend class MinimalBody;
    explicit MinimalRange( size_t i ) : begin(0), end(i) {}
    friend void Flog( int nthread, bool interference );
public:
    MinimalRange( MinimalRange& r, tbb::split ) : end(r.end) {
        begin = r.end = (r.begin+r.end)/2;
    }
    bool is_divisible() const {return end-begin>=2;}
    bool empty() const {return begin==end;}
};

//! Class with public interface that is exactly minimal requirements for Body of a parallel_reduce
class MinimalBody : tbb::internal::no_copy {
private:
    friend void Flog( int nthread, bool interference );
    //! Parent that created this body via split operation.  NULL if original body.
    MinimalBody* parent;
    //! Total number of index values processed by body and its children.
    size_t sum;
    //! Number of join operations done so far on this body and its children.
    long join_count;
    //! Range that has been processed so far by this body and its children.
    size_t begin, end;
    //! True if body has not yet been processed at least once by operator().
    bool is_new;
    //! 1 if body was created by split; 0 if original body.
    int forked;
    MinimalBody()
            : parent(NULL), sum(0), join_count(0), begin(~size_t(0)), end(~size_t(0)), is_new(true), forked(0) {
        ++MinimalBodyCount;
    }
public:
    ~MinimalBody() {
        forked = 0xDEADBEEF;
        sum=0xDEADBEEF;
        join_count=0xDEADBEEF;
        --MinimalBodyCount;
    }
    MinimalBody( MinimalBody& other, tbb::split )
            : parent(&other), sum(0), join_count(0), begin(~size_t(0)), end(~size_t(0)), is_new(true), forked(1) {
        ++MinimalBodyCount;
        ++ForkCount;
    }
    void join( MinimalBody& s ) {
        ASSERT( s.forked==1, NULL );
        ASSERT( this!=&s, NULL );
        ASSERT( this==s.parent, NULL );
        ASSERT( end==s.begin, NULL );
        end = s.end;
        sum += s.sum;
        join_count += s.join_count + 1;
        s.forked = 2;
    }
    void operator()( const MinimalRange& r ) {
        for( size_t k=r.begin; k<r.end; ++k )
            ++sum;
        if( is_new ) {
            is_new = false;
            begin = r.begin;
        } else
            ASSERT( end==r.begin, NULL );
        end = r.end;
    }
};

#include "harness.h"
#include "tbb/tick_count.h"

void Flog( int nthread, bool interference=false ) {
    for (int mode = 0;  mode < 4; mode++) {
        tbb::tick_count T0 = tbb::tick_count::now();
        long join_count = 0;
        tbb::affinity_partitioner ap;
        for( size_t i=0; i<=1000; ++i ) {
            MinimalBody body;
            ASSERT( MinimalBodyCount==1, NULL );
            switch (mode) {
                case 0:
                    tbb::parallel_reduce( MinimalRange(i), body );
                    break;
                case 1:
                    tbb::parallel_reduce( MinimalRange(i), body, tbb::simple_partitioner() );
                    break;
                case 2:
                    tbb::parallel_reduce( MinimalRange(i), body, tbb::auto_partitioner() );
                    break;
                case 3:
                    tbb::parallel_reduce( MinimalRange(i), body, ap );
                    break;
            }
            join_count += body.join_count;
            ASSERT( MinimalBodyCount==1, NULL );
            ASSERT( body.sum==i, NULL );
            ASSERT( body.begin==(i==0 ? ~size_t(0) : 0), NULL );
            ASSERT( body.end==(i==0 ? ~size_t(0) : i), NULL );
        }
        tbb::tick_count T1 = tbb::tick_count::now();
        REMARK("time=%g join_count=%ld ForkCount=%ld nthread=%d%s\n",
                   (T1-T0).seconds(), join_count, long(ForkCount), nthread, interference ? " with interference":"");
    }
}

#include "tbb/blocked_range.h"

#if _MSC_VER
    typedef tbb::internal::uint64_t ValueType;
#else
    typedef uint64_t ValueType;
#endif

// These macros illustrate possible signatures for reduction operations,
// specifically for those relatively unusual reduction types where the
// difference with simply returning and taking by value is significant.
// Currently, not all combinations are tested at the same time: the test relies on
// herd immunity, so to say, to cover both values of __TBB_CPP11_RVALUE_REF_PRESENT,
// and the pre-C++11 guarantee-based usage is not tested with a C++11 compiler (TODO?).
#if !__TBB_CPP11_RVALUE_REF_PRESENT
#if TBB_PARALLEL_REDUCE_PRE_CPP11_RVALUE_REF_FORM_GUARANTEE
#define TESTING_GUARANTEE_BASED_SIGNATURE 1
#else
#error // time to embrace move semantics
#endif
#else
#define TESTING_GUARANTEE_BASED_SIGNATURE 0
#endif
    
#if TESTING_GUARANTEE_BASED_SIGNATURE
#define OPERATION_PARAMETER_TYPE(Type)       Type&
#define    OPERATION_RESULT_TYPE(Type) const Type&
#else
#define OPERATION_PARAMETER_TYPE(Type)       Type&&
#define    OPERATION_RESULT_TYPE(Type)       Type
#endif

// Only significant with TESTING_GUARANTEE_BASED_SIGNATURE,
// and just for curiosity's sake (could also be eliminated
// by choosing the code for value 1 and deleting the rest).
#define USING_LEFT_PARAMETER_FOR_SUM 1

// Used as: #define M(what) [...] / M_COUNTERS / #undef M
#define M_COUNTERS                                       \
  /* object life cycle for Accumulated:               */ \
  M(con   ) /*      constructor from arguments        */ \
  M(cc1   ) /* copy-constructor from identity         */ \
  M(cc2   ) /* copy-constructor from other            */ \
  M(ca1   ) /* copy-assignment  from itself           */ \
  M(ca2   ) /* copy-assignment  from another instance */ \
  M(mc    ) /* move-constructor                       */ \
  M(ma    ) /* move-assignment                        */ \
  M(des   ) /*      destructor                        */ \
  /* algorithm steps for Body:                        */ \
  M(range1) /*      range       from identity         */ \
  M(range2) /*      range       from other            */ \
  M(binary)                                              \
  M(aeo   ) /* assignment to empty optional           */

#define M(what) tbb::atomic<unsigned long> counter_##what;
M_COUNTERS
#undef M

void resetCounters() {
    #define M(what) counter_##what.store<tbb::relaxed>(0);
    M_COUNTERS
    #undef M
}

// These possible values for "used_with" in inspectCounters() are used:
// - as a column in the diagnostic output (==> same length), and
// - to select specific assertions based on pointer value (==> use only these constants).
// There is no with_function_object_semigroup because:
// - with_function_object_monoid is only relevant before C++11,
// - semigroup support requires at least C++11.
const char* const with_function_object_monoid = "function monoid   ";
#if __TBB_CPP11_LAMBDAS_PRESENT // otherwise not used
const char* const with_lambda_monoid          = "lambda   monoid   ";
#endif
const char* const with_lambda_semigroup       = "lambda   semigroup";

// TODO: make member function?
void inspectCounters(int nthreads, const char* what_partitioner, const char* used_with, std::size_t size) {
    const unsigned long counter_range =
        counter_range1.load<tbb::relaxed>() +
        counter_range2.load<tbb::relaxed>() ;

    // trace counter values:
    #define M(what) #what "=%3lu "
    const char* format = "nthreads=%i %s %s:  " M_COUNTERS "(average %g)\n";
    #undef M
    #define M(what) counter_##what.load<tbb::relaxed>(),
    REPORT( format, nthreads, what_partitioner, used_with, M_COUNTERS static_cast<double>(size) / counter_range); // TODO: change to REMARK
    #undef M

    // TODO: This assumes copy elision for constructing result in "Accumulated result = tbb::parallel_reduce([...]);",
    //       and maybe other situations as well (?).
    if (used_with != with_lambda_semigroup) {
    ASSERT( counter_con ==                                  0, NULL ); // ParallelSumTester::I is not involved in the counters
    ASSERT( counter_cc1 ==                 counter_binary + 1, NULL ); // during reduce_body_lambda_monoid split (same number as joins), plus 1 for initial argument
    } else {
    ASSERT( counter_con ==                 counter_binary + 1, NULL ); // once per smallest segment
    ASSERT( counter_cc1 ==                                  0, NULL ); // this is for semigroups, so...
    }
#if TESTING_GUARANTEE_BASED_SIGNATURE
    ASSERT( counter_cc2 ==                                  1, NULL ); // final body.result()
#if USING_LEFT_PARAMETER_FOR_SUM // advised
    ASSERT( counter_ca1 == counter_range + counter_binary    , NULL );
    ASSERT( counter_ca2 ==                                  0, NULL );
#else // and this is why (even though counter_binary is typically a small fraction of counter_range)
    ASSERT( counter_ca1 == counter_range                     , NULL );
    ASSERT( counter_ca2 ==                 counter_binary    , NULL );
#endif
    ASSERT( counter_mc  ==                                  0, NULL );
    ASSERT( counter_ma  ==                                  0, NULL );
#else // #if TESTING_GUARANTEE_BASED_SIGNATURE
    ASSERT( counter_cc2 ==                                  0, NULL );
    ASSERT( counter_ca1 ==                                  0, NULL );
    ASSERT( counter_ca2 ==                                  0, NULL );
    if (used_with != with_lambda_semigroup) {
    ASSERT( counter_mc  == counter_range + counter_binary + 1, NULL ); // 1 for final body.result()
    ASSERT( counter_ma  == counter_range + counter_binary    , NULL ); // ca1 before C++11, here split into mc and ma
    } else {
#if __TBB_PARALLEL_REDUCE_EXPERIMENTALLY_USE_OPTIONAL == 0
    ASSERT( counter_aeo == 0                                 , NULL );
#else
    ASSERT( counter_aeo >  0                                 , NULL );
#endif
    ASSERT( counter_mc  == counter_range + counter_aeo       , NULL );
    ASSERT( counter_ma  ==                 counter_binary    , NULL );
    }
#endif // #if TESTING_GUARANTEE_BASED_SIGNATURE

    ASSERT( counter_range1 ==              counter_binary + 1, NULL );
    if (used_with == with_lambda_semigroup) {
    ASSERT( counter_range2 ==                               0, NULL );
    }

    ASSERT( counter_des == counter_con + counter_cc1 + counter_cc2 + counter_mc, NULL );
}

struct Accumulated {
    tbb::blocked_range<ValueType*> m_range;
    ValueType m_value;
    explicit Accumulated(const tbb::blocked_range<ValueType*>& range, ValueType value) : m_range(range), m_value(value) {
        ++counter_con;
    }
    ~Accumulated() { ++counter_des; }
    Accumulated           (const Accumulated&  that) : m_range(that.m_range), m_value(that.m_value) {
        if (m_range.begin() == nullptr && m_range.end() == nullptr && m_value == 0) ++counter_cc1; else ++counter_cc2;
    }
    Accumulated& operator=(const Accumulated&  that) {
        if (this == &that) ++counter_ca1; else ++counter_ca2;
        this->m_range = that.m_range;
        this->m_value = that.m_value;
        return *this;
    }
#if __TBB_CPP11_RVALUE_REF_PRESENT
    Accumulated           (      Accumulated&& that) : m_range(that.m_range), m_value(that.m_value) {
        ++counter_mc;
        that.invalidate();
    }
    Accumulated& operator=(      Accumulated&& that) {
        ++counter_ma;
        ASSERT( this != &that, NULL );
        this->m_range = that.m_range;
        this->m_value = that.m_value;
        that.invalidate();
        return *this;
    }
#endif
    //! Invalidate after use in move construction/assignment to help detect any continued use after std::move().
    void invalidate() {
        m_range = tbb::blocked_range<ValueType*>(reinterpret_cast<ValueType*>(-1), reinterpret_cast<ValueType*>(-1));
        m_value = static_cast<ValueType>(-1);
    }
};

struct Sum {
    OPERATION_RESULT_TYPE(Accumulated)
    operator() ( OPERATION_PARAMETER_TYPE(Accumulated) v1, OPERATION_PARAMETER_TYPE(Accumulated) v2 ) const {
        ++counter_binary;
        ASSERT(v1.m_range.begin() < v1.m_range.end()                                         , "");
        ASSERT(                     v1.m_range.end() == v2.m_range.begin()                   , "");
        ASSERT(                                         v2.m_range.begin() < v2.m_range.end(), "");

#if USING_LEFT_PARAMETER_FOR_SUM
        v1.m_range = tbb::blocked_range<ValueType*>(v1.m_range.begin(), v2.m_range.end());
        v1.m_value += v2.m_value;
        return tbb::internal::move(v1);
#else
        v2.m_range = tbb::blocked_range<ValueType*>(v1.m_range.begin(), v2.m_range.end());
        v2.m_value += v1.m_value;
        return tbb::internal::move(v2);
#endif
    }
};

struct Accumulator_monoid {
    OPERATION_RESULT_TYPE(Accumulated)
    operator() ( const tbb::blocked_range<ValueType*>& r, OPERATION_PARAMETER_TYPE(Accumulated) value ) const {
        ASSERT(nullptr != r.begin()                     , "");
        ASSERT(           r.begin() < r.end()           , "");
        ASSERT(                       r.end() != nullptr, ""); // redundant
        ASSERT((value.m_range.begin() == nullptr) == (value.m_range.end() == nullptr), "");
        ASSERT((value.m_range.begin() == nullptr) == (value.m_value       == 0      ), "");
        if (value.m_range.begin() == nullptr) {
            ++counter_range1;
            value.m_range = r;
        } else {
            ++counter_range2;
            ASSERT(value.m_range.begin() < value.m_range.end()             , "");
            ASSERT(                        value.m_range.end() == r.begin(), "");
            value.m_range = tbb::blocked_range<ValueType*>(value.m_range.begin(), r.end());
        }
        value.m_value = std::accumulate(r.begin(), r.end(), value.m_value);
        return tbb::internal::move(value);
    }
};

//! Like Accumulator_monoid but for semigroup usage (no parameter with initial value).
struct Accumulator_semigroup {
    Accumulated operator() ( const tbb::blocked_range<ValueType*>& r ) const {
        ASSERT(nullptr != r.begin()                     , "");
        ASSERT(           r.begin() < r.end()           , "");
        ASSERT(                       r.end() != nullptr, ""); // redundant
        ++counter_range1;
        return Accumulated(r, std::accumulate(std::next(r.begin()), r.end(), *r.begin()));
    }
};

//! Type-tag for automatic testing algorithm deduction
struct harness_default_partitioner {};

class ParallelSumTester: public NoAssign {
public:
    ParallelSumTester(int nthreads)
        : m_nthreads(nthreads)
        , m_array(new ValueType[unsigned(N)])
        , m_range(m_array, m_array + N)
        , m_empty(m_array, m_array)
    {
        for ( ValueType i = 0; i < N; ++i )
            m_array[i] = i + 1;
    }
    ~ParallelSumTester() { delete[] m_array; }
    template<typename Partitioner>
    void CheckParallelReduce(const char* what_partitioner) {
        Partitioner partitioner;
        resetCounters();
        {
            Accumulated result = tbb::parallel_reduce( m_range, I, Accumulator_monoid(), Sum(), partitioner );
            ASSERT( result.m_range.begin() == m_range.begin() && result.m_range.end() == m_range.end() && result.m_value == R, NULL );
        }
        inspectCounters(m_nthreads, what_partitioner, with_function_object_monoid, m_range.size());
#if __TBB_CPP11_LAMBDAS_PRESENT
        resetCounters();
        {
            Accumulated result = tbb::parallel_reduce(
                m_range, I,
                [](const tbb::blocked_range<ValueType*>& r, OPERATION_PARAMETER_TYPE(Accumulated) value) -> Accumulated {
                    return Accumulator_monoid()(r, tbb::internal::move(value));
                },
                [](OPERATION_PARAMETER_TYPE(Accumulated) v1, OPERATION_PARAMETER_TYPE(Accumulated) v2) -> Accumulated {
                    return Sum()(tbb::internal::move(v1), tbb::internal::move(v2));
                },
                partitioner
            );
            ASSERT( result.m_range.begin() == m_range.begin() && result.m_range.end() == m_range.end() && result.m_value == R, NULL );
        }
        inspectCounters(m_nthreads, what_partitioner, with_lambda_monoid, m_range.size());
#if __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS
        resetCounters();
        {
            Accumulated result = tbb::parallel_reduce1(
                m_range,
                [](const tbb::blocked_range<ValueType*>& r) -> Accumulated {
                    return Accumulator_semigroup()(r);
                },
                [](OPERATION_PARAMETER_TYPE(Accumulated) v1, OPERATION_PARAMETER_TYPE(Accumulated) v2) -> Accumulated {
                    return Sum()(tbb::internal::move(v1), tbb::internal::move(v2));
                },
                partitioner
            );
            ASSERT( result.m_range.begin() == m_range.begin() && result.m_range.end() == m_range.end() && result.m_value == R, NULL );
        }
        inspectCounters(m_nthreads, what_partitioner, with_lambda_semigroup, m_range.size());
        try {
            // same call as above, except that the input range is now m_empty instead of m_range
            Accumulated result = tbb::parallel_reduce1(
                m_empty,
                [](const tbb::blocked_range<ValueType*>& r) -> Accumulated {
                    return Accumulator_semigroup()(r);
                },
                [](OPERATION_PARAMETER_TYPE(Accumulated) v1, OPERATION_PARAMETER_TYPE(Accumulated) v2) -> Accumulated {
                    return Sum()(tbb::internal::move(v1), tbb::internal::move(v2));
                },
                partitioner
            );
            ASSERT( false, "empty input range without identity argument must throw invalid_argument exception" );
        } catch (const std::invalid_argument& exception) {
            // expected
        } catch (...) {
            ASSERT( false, "empty input range without identity argument must throw invalid_argument exception" );
        }
#endif /* __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS */
#endif /* __TBB_CPP11_LAMBDAS_PRESENT */
    }
    template<>
    void CheckParallelReduce<harness_default_partitioner>(const char* what_partitioner) {
        resetCounters();
        {
            Accumulated result = tbb::parallel_reduce( m_range, I, Accumulator_monoid(), Sum() );
            ASSERT( result.m_range.begin() == m_range.begin() && result.m_range.end() == m_range.end() && result.m_value == R, NULL );
        }
        inspectCounters(m_nthreads, what_partitioner, with_function_object_monoid, m_range.size());
#if __TBB_CPP11_LAMBDAS_PRESENT
        resetCounters();
        {
            Accumulated result = tbb::parallel_reduce(
                m_range, I,
                [](const tbb::blocked_range<ValueType*>& r, OPERATION_PARAMETER_TYPE(Accumulated) value) -> Accumulated {
                    return Accumulator_monoid()(r, tbb::internal::move(value));
                },
                [](OPERATION_PARAMETER_TYPE(Accumulated) v1, OPERATION_PARAMETER_TYPE(Accumulated) v2) -> Accumulated {
                    return Sum()(tbb::internal::move(v1), tbb::internal::move(v2));
                }
            );
            ASSERT( result.m_range.begin() == m_range.begin() && result.m_range.end() == m_range.end() && result.m_value == R, NULL );
        }
        inspectCounters(m_nthreads, " default partitioner", with_lambda_monoid, m_range.size());
#if __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS
        resetCounters();
        {
            Accumulated result = tbb::parallel_reduce1(
                m_range,
                [](const tbb::blocked_range<ValueType*>& r) -> Accumulated {
                    return Accumulator_semigroup()(r);
                },
                [](OPERATION_PARAMETER_TYPE(Accumulated) v1, OPERATION_PARAMETER_TYPE(Accumulated) v2) -> Accumulated {
                    return Sum()(tbb::internal::move(v1), tbb::internal::move(v2));
                }
            );
            ASSERT( result.m_range.begin() == m_range.begin() && result.m_range.end() == m_range.end() && result.m_value == R, NULL );
        }
        inspectCounters(m_nthreads, " default partitioner", with_lambda_semigroup, m_range.size());
        // TODO: also test empty input range with default partitioner?
#endif /* __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS */
#endif /* __TBB_CPP11_LAMBDAS_PRESENT */
    }
private:
    const int m_nthreads;
    ValueType* m_array;
    tbb::blocked_range<ValueType*> m_range;
    tbb::blocked_range<ValueType*> m_empty;
    static const ValueType N, R;
    static const Accumulated I;
};

const Accumulated ParallelSumTester::I(tbb::blocked_range<ValueType*>(nullptr, nullptr), 0);
const ValueType ParallelSumTester::N = 1000000;
const ValueType ParallelSumTester::R = N * (N + 1) / 2;

void ParallelSum (int nthreads) {
    ParallelSumTester pst(nthreads);
    pst.CheckParallelReduce<harness_default_partitioner>(" default partitioner");
    pst.CheckParallelReduce<tbb::    simple_partitioner>("  simple_partitioner");
    pst.CheckParallelReduce<tbb::      auto_partitioner>("    auto_partitioner");
    pst.CheckParallelReduce<tbb::  affinity_partitioner>("affinity_partitioner");
    pst.CheckParallelReduce<tbb::    static_partitioner>("  static_partitioner");
}

#include "harness_concurrency_tracker.h"

class RotOp {
public:
    typedef int Type;
    int operator() ( int x, int i ) const {
        return ( x<<1 ) ^ i;
    }
    int join( int x, int y ) const {
        return operator()( x, y );
    }
};

template <class Op>
struct ReduceBody {
    typedef typename Op::Type result_type;
    result_type my_value;

    ReduceBody() : my_value() {}
    ReduceBody( ReduceBody &, tbb::split ) : my_value() {}

    void operator() ( const tbb::blocked_range<int>& r ) {
        Harness::ConcurrencyTracker ct;
        for ( int i = r.begin(); i != r.end(); ++i ) {
            Op op;
            my_value = op(my_value, i);
        }
    }

    void join( const ReduceBody& y ) {
        Op op;
        my_value = op.join(my_value, y.my_value);
    }
};

template<typename Body, typename Partitioner>
struct parallel_deterministic_reduce_with_body {
    template<typename Range>
    static typename Body::result_type run( const Range& range ) {
        Body body;
        tbb::parallel_deterministic_reduce(range, body, Partitioner());
        return body.my_value;
    }
};

template<typename Body>
struct parallel_deterministic_reduce_with_body<Body, harness_default_partitioner> {
    template<typename Range>
    static typename Body::result_type run( const Range& range ) {
        Body body;
        tbb::parallel_deterministic_reduce(range, body);
        return body.my_value;
    }
};

template<typename ResultType, typename Partitioner>
struct parallel_deterministic_reduce_with_lambda_monoid {
    template<typename Range, typename RangeOperation, typename BinaryOperation>
    static ResultType run( const Range& range, RangeOperation range_operation, BinaryOperation binary_operation ) {
        return tbb::parallel_deterministic_reduce(range, ResultType(), range_operation, binary_operation, Partitioner());
    }
};

template<typename ResultType>
struct parallel_deterministic_reduce_with_lambda_monoid<ResultType, harness_default_partitioner> {
    template<typename Range, typename RangeOperation, typename BinaryOperation>
    static ResultType run(const Range& range, RangeOperation range_operation, BinaryOperation binary_operation) {
        return tbb::parallel_deterministic_reduce(range, ResultType(), range_operation, binary_operation);
    }
};

#if __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS

template<typename ResultType, typename Partitioner>
struct parallel_deterministic_reduce_with_lambda_semigroup {
    template<typename Range, typename RangeOperation, typename BinaryOperation>
    static ResultType run( const Range& range, RangeOperation range_operation, BinaryOperation binary_operation ) {
        return tbb::parallel_deterministic_reduce1(range, range_operation, binary_operation, Partitioner());
    }
};

template<typename ResultType>
struct parallel_deterministic_reduce_with_lambda_semigroup<ResultType, harness_default_partitioner> {
    template<typename Range, typename RangeOperation, typename BinaryOperation>
    static ResultType run(const Range& range, RangeOperation range_operation, BinaryOperation binary_operation) {
        return tbb::parallel_deterministic_reduce1(range, range_operation, binary_operation);
    }
};

#endif /* __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS */

//! Define overloads of parallel_deterministic_reduce that accept "undesired" types of partitioners
//! (they are executed, but the result is ignored).
namespace unsupported {

    template<typename Range, typename Body>
    void parallel_deterministic_reduce(const Range&, Body&, const tbb::auto_partitioner&) { }

    template<typename Range, typename Body>
    void parallel_deterministic_reduce(const Range&, Body&, tbb::affinity_partitioner&) { }

    template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation>
    Value parallel_deterministic_reduce(const Range&, const Value& identity, const RangeOperation&, const BinaryOperation&, const tbb::auto_partitioner&) {
        return identity;
    }

    template<typename Range, typename Value, typename RangeOperation, typename BinaryOperation>
    Value parallel_deterministic_reduce(const Range&, const Value& identity, const RangeOperation&, const BinaryOperation&, tbb::affinity_partitioner&) {
        return identity;
    }

#if __TBB_CPP11_DECLTYPE_PRESENT

    template<typename Range, typename RangeOperation, typename BinaryOperation>
    auto parallel_deterministic_reduce(const Range& range, const RangeOperation& range_operation, const BinaryOperation&, const tbb::auto_partitioner&)
    -> decltype(range_operation(range)) {
        return range_operation(range);
    }

    template<typename Range, typename RangeOperation, typename BinaryOperation>
    auto parallel_deterministic_reduce(const Range& range, const RangeOperation& range_operation, const BinaryOperation&, tbb::affinity_partitioner&)
    -> decltype(range_operation(range)) {
        return range_operation(range);
    }

#endif /* __TBB_CPP11_DECLTYPE_PRESENT */

}

#include "test_partitioner.h"

//! Check that other types of partitioners are not supported (auto, affinity)
//! In the case of "unsupported" API unexpectedly sneaking into namespace tbb,
//! this test should result in a compilation error due to overload resolution ambiguity
static void TestUnsupportedPartitioners() {
    using namespace tbb;
    using namespace unsupported;
    test_partitioner_utils::DummyReduceBody body;
    parallel_deterministic_reduce(blocked_range<int>(0, 10), body, tbb::auto_partitioner());

    tbb::affinity_partitioner ap;
    parallel_deterministic_reduce(blocked_range<int>(0, 10), body, ap);

#if __TBB_CPP11_LAMBDAS_PRESENT
    parallel_deterministic_reduce(
        blocked_range<int>(0, 10),
        0,
        [](const blocked_range<int>&, int init)->int { return init; },
        [](int x, int y)->int { return x + y; },
        tbb::auto_partitioner()
    );
    parallel_deterministic_reduce(
        blocked_range<int>(0, 10),
        0,
        [](const blocked_range<int>&, int init)->int { return init; },
        [](int x, int y)->int { return x + y; },
        ap
    );
#if __TBB_CPP11_DECLTYPE_PRESENT
    parallel_deterministic_reduce(
        blocked_range<int>(0, 10),
        [](const blocked_range<int>&)->int { return 0; },
        [](int x, int y)->int { return x + y; },
        tbb::auto_partitioner()
    );
    parallel_deterministic_reduce(
        blocked_range<int>(0, 10),
        [](const blocked_range<int>&)->int { return 0; },
        [](int x, int y)->int { return x + y; },
        ap
    );
#endif /* __TBB_CPP11_DECLTYPE_PRESENT */
#endif /* __TBB_CPP11_LAMBDAS_PRESENT */
}

template <class Partitioner>
void TestDeterministicReductionFor() {
    const int N = 1000;
    const tbb::blocked_range<int> range(0, N);
    typedef ReduceBody<RotOp> BodyType;
    BodyType::result_type R1 =
        parallel_deterministic_reduce_with_body<BodyType, Partitioner>::run(range);
    for ( int i=0; i<100; ++i ) {
        BodyType::result_type R2 =
            parallel_deterministic_reduce_with_body<BodyType, Partitioner>::run(range);
        ASSERT( R1 == R2, "parallel_deterministic_reduce behaves differently from run to run" );
#if __TBB_CPP11_LAMBDAS_PRESENT
        typedef RotOp::Type Type;
        Type R3 = parallel_deterministic_reduce_with_lambda_monoid<Type, Partitioner>::run(
            range,
            [](const tbb::blocked_range<int>& br, Type value) -> Type {
                ASSERT(value == Type(), NULL); // each subrange is evaluated by its own Body
                Harness::ConcurrencyTracker ct;
                for ( int ii = br.begin(); ii != br.end(); ++ii ) {
                    RotOp op;
                    value = op(value, ii);
                }
                return value;
            },
            [](const Type& v1, const Type& v2) -> Type {
                RotOp op;
                return op.join(v1, v2);
            }
        );
        ASSERT( R1 == R3, "lambda-based parallel_deterministic_reduce behaves differently from run to run" );
#if __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS
        Type R4 = parallel_deterministic_reduce_with_lambda_semigroup<Type, Partitioner>::run(
            range,
            [](const tbb::blocked_range<int>& br) -> Type {
                Type value = Type(); // see "ASSERT(value == Type(), NULL);" above
                Harness::ConcurrencyTracker ct;
                for ( int ii = br.begin(); ii != br.end(); ++ii ) {
                    RotOp op;
                    value = op(value, ii);
                }
                return value;
            },
            [](const Type& v1, const Type& v2) -> Type {
                RotOp op;
                return op.join(v1, v2);
            }
        );
        ASSERT( R1 == R4, "lambda-based parallel_deterministic_reduce behaves differently from run to run" );
#endif /* __TBB_PARALLEL_REDUCE_PROVIDE_SEMIGROUP_OVERLOADS */
#endif /* __TBB_CPP11_LAMBDAS_PRESENT */
    }
}

void TestDeterministicReduction () {
    TestDeterministicReductionFor<tbb::simple_partitioner>();
    TestDeterministicReductionFor<tbb::static_partitioner>();
    TestDeterministicReductionFor<harness_default_partitioner>();
    ASSERT_WARNING((Harness::ConcurrencyTracker::PeakParallelism() > 1), "no parallel execution\n");
}

#include "tbb/task_scheduler_init.h"
#include "harness_cpu.h"

namespace interaction_with_range_and_partitioner {

// Test checks compatibility of parallel_reduce algorithm with various range implementations

void test() {
    using namespace test_partitioner_utils::interaction_with_range_and_partitioner;

    test_partitioner_utils::DummyReduceBody body;
    tbb::affinity_partitioner dummy_affinity_partitioner;

    // Each Range*() constructor takes assert_in_nonproportional resp. assert_in_proportional boolean arguments.
    // TODO explain: why are there two arguments if the second is always the opposite of the first?

    parallel_reduce              (Range1(true, false), body, dummy_affinity_partitioner  );
    parallel_reduce              (Range2(true, false), body, dummy_affinity_partitioner  );
    parallel_reduce              (Range3(true, false), body, dummy_affinity_partitioner  );
    parallel_reduce              (Range4(false, true), body, dummy_affinity_partitioner  );
    parallel_reduce              (Range5(false, true), body, dummy_affinity_partitioner  );
    parallel_reduce              (Range6(false, true), body, dummy_affinity_partitioner  );

    parallel_reduce              (Range1(true, false), body, tbb::   static_partitioner());
    parallel_reduce              (Range2(true, false), body, tbb::   static_partitioner());
    parallel_reduce              (Range3(true, false), body, tbb::   static_partitioner());
    parallel_reduce              (Range4(false, true), body, tbb::   static_partitioner());
    parallel_reduce              (Range5(false, true), body, tbb::   static_partitioner());
    parallel_reduce              (Range6(false, true), body, tbb::   static_partitioner());

    parallel_reduce              (Range1(false, true), body, tbb::   simple_partitioner());
    parallel_reduce              (Range2(false, true), body, tbb::   simple_partitioner());
    parallel_reduce              (Range3(false, true), body, tbb::   simple_partitioner());
    parallel_reduce              (Range4(false, true), body, tbb::   simple_partitioner());
    parallel_reduce              (Range5(false, true), body, tbb::   simple_partitioner());
    parallel_reduce              (Range6(false, true), body, tbb::   simple_partitioner());

    parallel_reduce              (Range1(false, true), body, tbb::     auto_partitioner());
    parallel_reduce              (Range2(false, true), body, tbb::     auto_partitioner());
    parallel_reduce              (Range3(false, true), body, tbb::     auto_partitioner());
    parallel_reduce              (Range4(false, true), body, tbb::     auto_partitioner());
    parallel_reduce              (Range5(false, true), body, tbb::     auto_partitioner());
    parallel_reduce              (Range6(false, true), body, tbb::     auto_partitioner());

    parallel_deterministic_reduce(Range1(true, false), body, tbb::   static_partitioner());
    parallel_deterministic_reduce(Range2(true, false), body, tbb::   static_partitioner());
    parallel_deterministic_reduce(Range3(true, false), body, tbb::   static_partitioner());
    parallel_deterministic_reduce(Range4(false, true), body, tbb::   static_partitioner());
    parallel_deterministic_reduce(Range5(false, true), body, tbb::   static_partitioner());
    parallel_deterministic_reduce(Range6(false, true), body, tbb::   static_partitioner());

    parallel_deterministic_reduce(Range1(false, true), body, tbb::   simple_partitioner());
    parallel_deterministic_reduce(Range2(false, true), body, tbb::   simple_partitioner());
    parallel_deterministic_reduce(Range3(false, true), body, tbb::   simple_partitioner());
    parallel_deterministic_reduce(Range4(false, true), body, tbb::   simple_partitioner());
    parallel_deterministic_reduce(Range5(false, true), body, tbb::   simple_partitioner());
    parallel_deterministic_reduce(Range6(false, true), body, tbb::   simple_partitioner());
}

} // interaction_with_range_and_partitioner

int TestMain () {
    TestUnsupportedPartitioners();
    if( MinThread<0 ) {
        REPORT("Usage: nthread must be positive\n");
        exit(1);
    }
    for( int p=MinThread; p<=MaxThread; ++p ) {
        tbb::task_scheduler_init init( p );
        Flog(p);
        ParallelSum(p);
        if ( p>=2 )
            TestDeterministicReduction();
        // Test that all workers sleep when no work
        TestCPUUserTime(p);
    }
    interaction_with_range_and_partitioner::test();
    return Harness::Done;
}
