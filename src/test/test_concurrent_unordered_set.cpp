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

#include "harness_defs.h"
#if !(__TBB_TEST_SECONDARY && __TBB_CPP11_STD_PLACEHOLDERS_LINKAGE_BROKEN)

#if _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#endif

#define __TBB_EXTRA_DEBUG 1
#include "tbb/concurrent_unordered_set.h"
#include "harness_assert.h"

#if __TBB_TEST_SECONDARY

#include "harness_runtime_loader.h"

#else // __TBB_TEST_SECONDARY
#if __TBB_INITIALIZER_LISTS_PRESENT
// These operator== are used implicitly in  test_initializer_list.h.
// For some unknown reason clang is not able to find the if they a declared after the
// inclusion of test_initializer_list.h.
template<typename container_type>
bool equal_containers( container_type const& lhs, container_type const& rhs );
template<typename T>
bool operator==(tbb::concurrent_unordered_set<T> const& lhs, tbb::concurrent_unordered_set<T> const& rhs) {
    return equal_containers( lhs, rhs );
}

template<typename T>
bool operator==(tbb::concurrent_unordered_multiset<T> const& lhs, tbb::concurrent_unordered_multiset<T> const& rhs) {
    return equal_containers( lhs, rhs );
}
#endif /* __TBB_INITIALIZER_LISTS_PRESENT */
#include "test_concurrent_unordered_common.h"

typedef tbb::concurrent_unordered_set<int, tbb::tbb_hash<int>, std::equal_to<int>, MyAllocator> MySet;
typedef tbb::concurrent_unordered_set<int, degenerate_hash<int>, std::equal_to<int>, MyAllocator> MyDegenerateSet;
typedef tbb::concurrent_unordered_set<check_type<int>, tbb::tbb_hash<check_type<int> >, std::equal_to<check_type<int> >, MyAllocator> MyCheckedSet;
typedef tbb::concurrent_unordered_set<FooWithAssign, tbb::tbb_hash<Foo>, std::equal_to<FooWithAssign>, MyAllocator> MyCheckedStateSet;
typedef tbb::concurrent_unordered_multiset<int, tbb::tbb_hash<int>, std::equal_to<int>, MyAllocator> MyMultiSet;
typedef tbb::concurrent_unordered_multiset<int, degenerate_hash<int>, std::equal_to<int>, MyAllocator> MyDegenerateMultiSet;
typedef tbb::concurrent_unordered_multiset<check_type<int>, tbb::tbb_hash<check_type<int> >, std::equal_to<check_type<int> >, MyAllocator> MyCheckedMultiSet;

#if __TBB_CPP11_RVALUE_REF_PRESENT
struct cu_set_type : unordered_move_traits_base {
    template<typename element_type, typename allocator_type>
    struct apply {
        typedef tbb::concurrent_unordered_set<element_type, tbb::tbb_hash<element_type>, std::equal_to<element_type>, allocator_type > type;
    };

    typedef FooIterator init_iterator_type;
};

struct cu_multiset_type : unordered_move_traits_base {
    template<typename element_type, typename allocator_type>
    struct apply {
        typedef tbb::concurrent_unordered_multiset<element_type, tbb::tbb_hash<element_type>, std::equal_to<element_type>, allocator_type > type;
    };

    typedef FooIterator init_iterator_type;
};
#endif /* __TBB_CPP11_RVALUE_REF_PRESENT */

template <bool defCtorPresent, typename value_type>
void TestTypesSet( const std::list<value_type> &lst ) {
    TypeTester< defCtorPresent, tbb::concurrent_unordered_set<value_type, tbb::tbb_hash<value_type>, Harness::IsEqual>,
        tbb::concurrent_unordered_set< value_type, tbb::tbb_hash<value_type>, Harness::IsEqual, debug_allocator<value_type> > >( lst );
    TypeTester< defCtorPresent, tbb::concurrent_unordered_multiset<value_type, tbb::tbb_hash<value_type>, Harness::IsEqual>,
        tbb::concurrent_unordered_multiset< value_type, tbb::tbb_hash<value_type>, Harness::IsEqual, debug_allocator<value_type> > >( lst );
}

void TestTypes( ) {
    const int NUMBER = 10;

    std::list<int> arrInt;
    for ( int i = 0; i<NUMBER; ++i ) arrInt.push_back( i );
    TestTypesSet</*defCtorPresent = */true>( arrInt );

    std::list< tbb::atomic<int> > arrTbb(NUMBER);
    int seq = 0;
    for ( std::list< tbb::atomic<int> >::iterator it = arrTbb.begin(); it != arrTbb.end(); ++it, ++seq ) *it = seq;
    TestTypesSet</*defCtorPresent = */true>( arrTbb );

#if __TBB_CPP11_REFERENCE_WRAPPER_PRESENT
    std::list< std::reference_wrapper<int> > arrRef;
    for ( std::list<int>::iterator it = arrInt.begin( ); it != arrInt.end( ); ++it )
        arrRef.push_back( std::reference_wrapper<int>(*it) );
    TestTypesSet</*defCtorPresent = */false>( arrRef );
#endif /* __TBB_CPP11_REFERENCE_WRAPPER_PRESENT */

#if __TBB_CPP11_SMART_POINTERS_PRESENT
    std::list< std::shared_ptr<int> > arrShr;
    for ( int i = 0; i<NUMBER; ++i ) arrShr.push_back( std::make_shared<int>( i ) );
    TestTypesSet</*defCtorPresent = */true>( arrShr );

    std::list< std::weak_ptr<int> > arrWk;
    std::copy( arrShr.begin( ), arrShr.end( ), std::back_inserter( arrWk ) );
    TestTypesSet</*defCtorPresent = */true>( arrWk );
#else
    REPORT( "Known issue: C++11 smart pointer tests are skipped.\n" );
#endif /* __TBB_CPP11_SMART_POINTERS_PRESENT */
}
#endif // __TBB_TEST_SECONDARY

#if !__TBB_TEST_SECONDARY
#define INITIALIZATION_TIME_TEST_NAMESPACE            initialization_time_test
#define TEST_INITIALIZATION_TIME_OPERATIONS_NAME      test_initialization_time_operations
void test_initialization_time_operations_external();
#else
#define INITIALIZATION_TIME_TEST_NAMESPACE            initialization_time_test_external
#define TEST_INITIALIZATION_TIME_OPERATIONS_NAME      test_initialization_time_operations_external
#endif

namespace INITIALIZATION_TIME_TEST_NAMESPACE {
    tbb::concurrent_unordered_set<int> static_init_time_set;
    int any_non_zero_value = 89432;
    bool static_init_time_inserted = (static_init_time_set.insert( any_non_zero_value )).second;
    bool static_init_time_found = ((static_init_time_set.find( any_non_zero_value )) != static_init_time_set.end( ));
}
void TEST_INITIALIZATION_TIME_OPERATIONS_NAME( ) {
    using namespace INITIALIZATION_TIME_TEST_NAMESPACE;
#define LOCATION ",in function: " __TBB_STRING(TEST_INITIALIZATION_TIME_OPERATIONS_NAME)
    ASSERT( static_init_time_inserted, "failed to insert an item during initialization of global objects" LOCATION );
    ASSERT( static_init_time_found, "failed to find an item during initialization of global objects" LOCATION );

    bool static_init_time_found_in_main = ((static_init_time_set.find( any_non_zero_value )) != static_init_time_set.end( ));
    ASSERT( static_init_time_found_in_main, "failed to find during main() an item inserted during initialization of global objects" LOCATION );
#undef LOCATION
}

#if !__TBB_TEST_SECONDARY
int TestMain() {
    test_machine( );

    test_basic<MySet>( "concurrent unordered Set" );
    test_basic<MyDegenerateSet>( "concurrent unordered degenerate Set" );
    test_concurrent<MySet>("concurrent unordered Set");
    test_concurrent<MyDegenerateSet>( "concurrent unordered degenerate Set" );
    test_basic<MyMultiSet>("concurrent unordered MultiSet");
    test_basic<MyDegenerateMultiSet>("concurrent unordered degenerate MultiSet");
    test_concurrent<MyMultiSet>( "concurrent unordered MultiSet" );
    test_concurrent<MyDegenerateMultiSet>("concurrent unordered degenerate MultiSet");
    test_concurrent<MyMultiSet>( "concurrent unordered MultiSet asymptotic", true );

    { Check<MyCheckedSet::value_type> checkit; test_basic<MyCheckedSet>( "concurrent_unordered_set (checked)" ); }
    { Check<MyCheckedSet::value_type> checkit; test_concurrent<MyCheckedSet>( "concurrent unordered set (checked)" ); }
    test_basic<MyCheckedStateSet>("concurrent unordered set (checked element state)", tbb::internal::true_type());

    { Check<MyCheckedMultiSet::value_type> checkit; test_basic<MyCheckedMultiSet>("concurrent_unordered_multiset (checked)"); }
    { Check<MyCheckedMultiSet::value_type> checkit; test_concurrent<MyCheckedMultiSet>( "concurrent unordered multiset (checked)" ); }

    test_initialization_time_operations( );
#if !__TBB_CPP11_STD_PLACEHOLDERS_LINKAGE_BROKEN
    test_initialization_time_operations_external( );
#else
    REPORT( "Known issue: global objects initialization time tests skipped.\n" );
#endif //!__TBB_CPP11_STD_PLACEHOLDERS_LINKING_BROKEN

#if __TBB_INITIALIZER_LISTS_PRESENT
    TestInitList< tbb::concurrent_unordered_set<int>,
                  tbb::concurrent_unordered_multiset<int> >( {1,2,3,4,5} );
#endif

#if __TBB_CPP11_RVALUE_REF_PRESENT
    test_rvalue_ref_support<cu_set_type>( "concurrent unordered set" );
    test_rvalue_ref_support<cu_multiset_type>( "concurrent unordered multiset" );
#endif /* __TBB_CPP11_RVALUE_REF_PRESENT */

    TestTypes();

    return Harness::Done;
}
#endif //#if !__TBB_TEST_SECONDARY
#endif //!(__TBB_TEST_SECONDARY && __TBB_CPP11_STD_PLACEHOLDERS_LINKAGE_BROKEN)
