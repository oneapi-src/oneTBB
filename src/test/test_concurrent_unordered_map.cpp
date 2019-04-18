/*
    Copyright (c) 2005-2019 Intel Corporation

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

#define __TBB_EXTRA_DEBUG 1
#if _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#endif

#include "tbb/concurrent_unordered_map.h"
#if __TBB_INITIALIZER_LISTS_PRESENT
// These operator== are used implicitly in  test_initializer_list.h.
// For some unknown reason clang is not able to find the if they a declared after the
// inclusion of test_initializer_list.h.
template<typename container_type>
bool equal_containers( container_type const& lhs, container_type const& rhs );
template<typename Key, typename Value>
bool operator==( tbb::concurrent_unordered_map<Key, Value> const& lhs, tbb::concurrent_unordered_map<Key, Value> const& rhs ) {
    return equal_containers( lhs, rhs );
}
template<typename Key, typename Value>
bool operator==( tbb::concurrent_unordered_multimap<Key, Value> const& lhs, tbb::concurrent_unordered_multimap<Key, Value> const& rhs ) {
    return equal_containers( lhs, rhs );
}
#endif /* __TBB_INITIALIZER_LISTS_PRESENT */
#include "test_concurrent_unordered_common.h"

typedef tbb::concurrent_unordered_map<int, int, tbb::tbb_hash<int>, std::equal_to<int>, MyAllocator> MyMap;
typedef tbb::concurrent_unordered_map<int, int, degenerate_hash<int>, std::equal_to<int>, MyAllocator> MyDegenerateMap;
typedef tbb::concurrent_unordered_map<int, check_type<int>, tbb::tbb_hash<int>, std::equal_to<int>, MyAllocator> MyCheckedMap;
typedef tbb::concurrent_unordered_map<intptr_t, FooWithAssign, tbb::tbb_hash<intptr_t>, std::equal_to<intptr_t>, MyAllocator> MyCheckedStateMap;
typedef tbb::concurrent_unordered_multimap<int, int, tbb::tbb_hash<int>, std::equal_to<int>, MyAllocator> MyMultiMap;
typedef tbb::concurrent_unordered_multimap<int, int, degenerate_hash<int>, std::equal_to<int>, MyAllocator> MyDegenerateMultiMap;
typedef tbb::concurrent_unordered_multimap<int, check_type<int>, tbb::tbb_hash<int>, std::equal_to<int>, MyAllocator> MyCheckedMultiMap;

template <>
struct SpecialTests <MyMap> {
    static void Test( const char *str ) {
        MyMap cont( 0 );
        const MyMap &ccont( cont );

        // mapped_type& operator[](const key_type& k);
        cont[1] = 2;

        // bool empty() const;
        ASSERT( !ccont.empty( ), "Concurrent container empty after adding an element" );

        // size_type size() const;
        ASSERT( ccont.size( ) == 1, "Concurrent container size incorrect" );

        ASSERT( cont[1] == 2, "Concurrent container value incorrect" );

        // mapped_type& at( const key_type& k );
        // const mapped_type& at(const key_type& k) const;
        ASSERT( cont.at( 1 ) == 2, "Concurrent container value incorrect" );
        ASSERT( ccont.at( 1 ) == 2, "Concurrent container value incorrect" );

        // iterator find(const key_type& k);
        MyMap::const_iterator it = cont.find( 1 );
        ASSERT( it != cont.end( ) && Value<MyMap>::get( *(it) ) == 2, "Element with key 1 not properly found" );
        cont.unsafe_erase( it );
        it = cont.find( 1 );
        ASSERT( it == cont.end( ), "Element with key 1 not properly erased" );

        REMARK( "passed -- specialized %s tests\n", str );
    }
};

void
check_multimap(MyMultiMap &m, int *targets, int tcount, int key) {
    std::vector<bool> vfound(tcount,false);
    std::pair<MyMultiMap::iterator, MyMultiMap::iterator> range = m.equal_range( key );
    for(MyMultiMap::iterator it = range.first; it != range.second; ++it) {
        bool found = false;
        for( int i = 0; i < tcount; ++i) {
            if((*it).second == targets[i]) {
                if(!vfound[i])  { // we can insert duplicate values
                    vfound[i] = found = true;
                    break;
                }
            }
        }
        // just in case an extra value in equal_range...
        ASSERT(found, "extra value from equal range");
    }
    for(int i = 0; i < tcount; ++i) ASSERT(vfound[i], "missing value");
}

template <>
struct SpecialTests <MyMultiMap> {
    static void Test( const char *str ) {
        int one_values[] = { 7, 2, 13, 23, 13 };
        int zero_values[] = { 4, 9, 13, 29, 42, 111};
        int n_zero_values = sizeof(zero_values) / sizeof(int);
        int n_one_values = sizeof(one_values) / sizeof(int);
        MyMultiMap cont( 0 );
        const MyMultiMap &ccont( cont );
        // mapped_type& operator[](const key_type& k);
        cont.insert( std::make_pair( 1, one_values[0] ) );

        // bool empty() const;
        ASSERT( !ccont.empty( ), "Concurrent container empty after adding an element" );

        // size_type size() const;
        ASSERT( ccont.size( ) == 1, "Concurrent container size incorrect" );
        ASSERT( (*(cont.begin( ))).second == one_values[0], "Concurrent container value incorrect" );
        ASSERT( (*(cont.equal_range( 1 )).first).second == one_values[0], "Improper value from equal_range" );
        ASSERT( (cont.equal_range( 1 )).second == cont.end( ), "Improper iterator from equal_range" );

        cont.insert( std::make_pair( 1, one_values[1] ) );

        // bool empty() const;
        ASSERT( !ccont.empty( ), "Concurrent container empty after adding an element" );

        // size_type size() const;
        ASSERT( ccont.size( ) == 2, "Concurrent container size incorrect" );
        check_multimap(cont, one_values, 2, 1);

        // insert the other {1,x} values
        for( int i = 2; i < n_one_values; ++i ) {
            cont.insert( std::make_pair( 1, one_values[i] ) );
        }

        check_multimap(cont, one_values, n_one_values, 1);
        ASSERT( (cont.equal_range( 1 )).second == cont.end( ), "Improper iterator from equal_range" );

        cont.insert( std::make_pair( 0, zero_values[0] ) );

        // bool empty() const;
        ASSERT( !ccont.empty( ), "Concurrent container empty after adding an element" );

        // size_type size() const;
        ASSERT( ccont.size( ) == (size_t)(n_one_values+1), "Concurrent container size incorrect" );
        check_multimap(cont, one_values, n_one_values, 1);
        check_multimap(cont, zero_values, 1, 0);
        ASSERT( (*(cont.begin( ))).second == zero_values[0], "Concurrent container value incorrect" );
        // insert the rest of the zero values
        for( int i = 1; i < n_zero_values; ++i) {
            cont.insert( std::make_pair( 0, zero_values[i] ) );
        }
        check_multimap(cont, one_values, n_one_values, 1);
        check_multimap(cont, zero_values, n_zero_values, 0);

        // clear, reinsert interleaved
        cont.clear();
        int bigger_num = ( n_one_values > n_zero_values ) ? n_one_values : n_zero_values;
        for( int i = 0; i < bigger_num; ++i ) {
            if(i < n_one_values) cont.insert( std::make_pair( 1, one_values[i] ) );
            if(i < n_zero_values) cont.insert( std::make_pair( 0, zero_values[i] ) );
        }
        check_multimap(cont, one_values, n_one_values, 1);
        check_multimap(cont, zero_values, n_zero_values, 0);


        REMARK( "passed -- specialized %s tests\n", str );
    }
};

#if __TBB_RANGE_BASED_FOR_PRESENT
#include "test_range_based_for.h"
// Add the similar test for concurrent_unordered_set.
void TestRangeBasedFor() {
    using namespace range_based_for_support_tests;

    REMARK( "testing range based for loop compatibility \n" );
    typedef tbb::concurrent_unordered_map<int, int> cu_map;
    cu_map a_cu_map;
    const int sequence_length = 100;
    for ( int i = 1; i <= sequence_length; ++i ) {
        a_cu_map.insert( cu_map::value_type( i, i ) );
    }

    ASSERT( range_based_for_accumulate( a_cu_map, pair_second_summer(), 0 ) == gauss_summ_of_int_sequence( sequence_length ), "incorrect accumulated value generated via range based for ?" );
}
#endif /* __TBB_RANGE_BASED_FOR_PRESENT */

#if __TBB_CPP11_RVALUE_REF_PRESENT
struct cu_map_type : unordered_move_traits_base {
    template<typename element_type, typename allocator_type>
    struct apply {
        typedef tbb::concurrent_unordered_map<element_type, element_type, tbb::tbb_hash<element_type>, std::equal_to<element_type>, allocator_type > type;
    };

    typedef FooPairIterator init_iterator_type;
};

struct cu_multimap_type : unordered_move_traits_base {
    template<typename element_type, typename allocator_type>
    struct apply {
        typedef tbb::concurrent_unordered_multimap<element_type, element_type, tbb::tbb_hash<element_type>, std::equal_to<element_type>, allocator_type > type;
    };

    typedef FooPairIterator init_iterator_type;
};
#endif /* __TBB_CPP11_RVALUE_REF_PRESENT */

template <typename Table>
class TestOperatorSquareBrackets : NoAssign {
    typedef typename Table::value_type ValueType;
    Table &my_c;
    const ValueType &my_value;
public:
    TestOperatorSquareBrackets( Table &c, const ValueType &value ) : my_c( c ), my_value( value ) {}
    void operator()() const {
        ASSERT( Harness::IsEqual()(my_c[my_value.first], my_value.second), NULL );
    }
};

template <bool defCtorPresent, typename Key, typename Element, typename Hasher, typename Equality, typename Allocator>
void TestMapSpecificMethods( tbb::concurrent_unordered_map<Key, Element, Hasher, Equality, Allocator> &c,
    const typename tbb::concurrent_unordered_map<Key, Element, Hasher, Equality, Allocator>::value_type &value ) {
    typedef tbb::concurrent_unordered_map<Key, Element, Hasher, Equality, Allocator> Table;
    CallIf<defCtorPresent>()(TestOperatorSquareBrackets<Table>( c, value ));
    ASSERT( Harness::IsEqual()(c.at( value.first ), value.second), NULL );
    const Table &constC = c;
    ASSERT( Harness::IsEqual()(constC.at( value.first ), value.second), NULL );
}

template <bool defCtorPresent, typename ValueType>
void TestTypesMap( const std::list<ValueType> &lst ) {
    typedef typename ValueType::first_type KeyType;
    typedef typename ValueType::second_type ElemType;
    TypeTester< defCtorPresent, tbb::concurrent_unordered_map<KeyType, ElemType, tbb::tbb_hash<KeyType>, Harness::IsEqual>,
        tbb::concurrent_unordered_map< KeyType, ElemType, tbb::tbb_hash<KeyType>, Harness::IsEqual, debug_allocator<ValueType> > >( lst );
    TypeTester< defCtorPresent, tbb::concurrent_unordered_multimap<KeyType, ElemType, tbb::tbb_hash<KeyType>, Harness::IsEqual>,
        tbb::concurrent_unordered_multimap< KeyType, ElemType, tbb::tbb_hash<KeyType>, Harness::IsEqual, debug_allocator<ValueType> > >( lst );
}

void TestTypes() {
    const int NUMBER = 10;

    std::list< std::pair<const int, int> > arrIntInt;
    for ( int i = 0; i < NUMBER; ++i ) arrIntInt.push_back( std::make_pair( i, NUMBER - i ) );
    TestTypesMap</*def_ctor_present = */true>( arrIntInt );

    std::list< std::pair< const int, tbb::atomic<int> > > arrIntTbb;
    for ( int i = 0; i < NUMBER; ++i ) {
        tbb::atomic<int> b;
        b = NUMBER - i;
        arrIntTbb.push_back( std::make_pair( i, b ) );
    }
    TestTypesMap</*defCtorPresent = */true>( arrIntTbb );

#if __TBB_CPP11_REFERENCE_WRAPPER_PRESENT && !__TBB_REFERENCE_WRAPPER_COMPILATION_BROKEN
    std::list< std::pair<const std::reference_wrapper<const int>, int> > arrRefInt;
    for ( std::list< std::pair<const int, int> >::iterator it = arrIntInt.begin(); it != arrIntInt.end(); ++it )
        arrRefInt.push_back( std::make_pair( std::reference_wrapper<const int>( it->first ), it->second ) );
    TestTypesMap</*defCtorPresent = */true>( arrRefInt );

    std::list< std::pair<const int, std::reference_wrapper<int> > > arrIntRef;
    for ( std::list< std::pair<const int, int> >::iterator it = arrIntInt.begin(); it != arrIntInt.end(); ++it ) {
        // Using std::make_pair below causes compilation issues with early implementations of std::reference_wrapper.
        arrIntRef.push_back( std::pair<const int, std::reference_wrapper<int> >( it->first, std::reference_wrapper<int>( it->second ) ) );
    }
    TestTypesMap</*defCtorPresent = */false>( arrIntRef );
#endif /* __TBB_CPP11_REFERENCE_WRAPPER_PRESENT && !__TBB_REFERENCE_WRAPPER_COMPILATION_BROKEN */

#if __TBB_CPP11_SMART_POINTERS_PRESENT
    std::list< std::pair< const std::shared_ptr<int>, std::shared_ptr<int> > > arrShrShr;
    for ( int i = 0; i < NUMBER; ++i ) {
        const int NUMBER_minus_i = NUMBER - i;
        arrShrShr.push_back( std::make_pair( std::make_shared<int>( i ), std::make_shared<int>( NUMBER_minus_i ) ) );
    }
    TestTypesMap</*defCtorPresent = */true>( arrShrShr );

    std::list< std::pair< const std::weak_ptr<int>, std::weak_ptr<int> > > arrWkWk;
    std::copy( arrShrShr.begin(), arrShrShr.end(), std::back_inserter( arrWkWk ) );
    TestTypesMap</*defCtorPresent = */true>( arrWkWk );

#if __TBB_CPP11_RVALUE_REF_PRESENT && __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT
    // Regression test for a problem with excessive requirements of emplace()
    test_emplace_insert<tbb::concurrent_unordered_map< int*, test::unique_ptr<int> >,
                        tbb::internal::false_type>( new int, new int );
    test_emplace_insert<tbb::concurrent_unordered_multimap< int*, test::unique_ptr<int> >,
                        tbb::internal::false_type>( new int, new int );
#endif

#else
    REPORT( "Known issue: C++11 smart pointer tests are skipped.\n" );
#endif /* __TBB_CPP11_SMART_POINTERS_PRESENT */
}

#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT
template <template <typename...> typename TMap>
void TestDeductionGuides() {
    using ComplexType = std::pair<int, std::string>;
    using ComplexTypeConst = std::pair<const int, std::string>;
    std::vector<ComplexType> v;
    auto l = { ComplexTypeConst(1, "one"), ComplexTypeConst(2, "two")};

    // check TMap(InputIterator, InputIterator)
    TMap m0(v.begin(), v.end());
    static_assert(std::is_same<decltype(m0), TMap<int, std::string>>::value);

    // check TMap(InputIterator, InputIterator, size_t)
    TMap m1(v.begin(), v.end(), 1);
    static_assert(std::is_same<decltype(m1), TMap<int, std::string>>::value);


    // check TMap(InputIterator, InputIterator, size_t, Hasher)
    TMap m2(v.begin(), v.end(), 4, std::hash<int>());
    static_assert(std::is_same<decltype(m2), TMap<int, std::string, std::hash<int>>>::value);

    // check TMap(InputIterator, InputIterator, size_t, Hasher, Equality)
    TMap m3(v.begin(), v.end(), 4, std::hash<int>(), std::less<int>());
    static_assert(std::is_same<decltype(m3), TMap<int, std::string, std::hash<int>, std::less<int>>>::value);

    // check TMap(InputIterator, InputIterator, size_t, Hasher, Equality, Allocator)
    TMap m4(v.begin(), v.end(), 4, std::hash<int>(), std::less<int>(), std::allocator<int>());
    static_assert(std::is_same<decltype(m4), TMap<int, std::string, std::hash<int>,
        std::less<int>, std::allocator<int>>>::value);

    // check TMap(InputIterator, InputIterator, size_t, Allocator)
    TMap m5(v.begin(), v.end(), 5, std::allocator<int>());
    static_assert(std::is_same<decltype(m5), TMap<int, std::string, tbb::tbb_hash<int>,
        std::equal_to<int>, std::allocator<int>>>::value);

    // check TMap(InputIterator, InputIterator, size_t, Hasher, Allocator)
    TMap m6(v.begin(), v.end(), 4, std::hash<int>(), std::allocator<int>());
    static_assert(std::is_same<decltype(m6), TMap<int, std::string, std::hash<int>,
        std::equal_to<int>, std::allocator<int>>>::value);

    // check TMap(std::initializer_list)
    TMap m7(l);
    static_assert(std::is_same<decltype(m7), TMap<int, std::string>>::value);

    // check TMap(std::initializer_list, size_t)
    TMap m8(l, 1);
    static_assert(std::is_same<decltype(m8), TMap<int, std::string>>::value);

    // check TMap(std::initializer_list, size_t, Hasher)
    TMap m9(l, 4, std::hash<int>());
    static_assert(std::is_same<decltype(m9), TMap<int, std::string, std::hash<int>>>::value);

    // check TMap(std::initializer_list, size_t, Hasher, Equality)
    TMap m10(l, 4, std::hash<int>(), std::less<int>());
    static_assert(std::is_same<decltype(m10), TMap<int, std::string, std::hash<int>, std::less<int>>>::value);

    // check TMap(std::initializer_list, size_t, Hasher, Equality, Allocator)
    TMap m11(l, 4, std::hash<int>(), std::less<int>(), std::allocator<int>());
    static_assert(std::is_same<decltype(m11), TMap<int, std::string, std::hash<int>,
        std::less<int>, std::allocator<int>>>::value);

    // check TMap(std::initializer_list, size_t, Allocator)
    TMap m12(l, 4, std::allocator<int>());
    static_assert(std::is_same<decltype(m12), TMap<int, std::string, tbb::tbb_hash<int>,
        std::equal_to<int>, std::allocator<int>>>::value);

    // check TMap(std::initializer_list, size_t, Hasher, Allocator)
    TMap m13(l, 4, std::hash<int>(), std::allocator<int>());
    static_assert(std::is_same<decltype(m13), TMap<int, std::string, std::hash<int>,
        std::equal_to<int>, std::allocator<int>>>::value);

    // check TMap(TMap &)
    TMap m14(m1);
    static_assert(std::is_same<decltype(m14), decltype(m1)>::value);

    // check TMap(TMap &, Allocator)
    TMap m15(m5, std::allocator<int>());
    static_assert(std::is_same<decltype(m15), decltype(m5)>::value);

    // check TMap(TMap &&)
    TMap m16(std::move(m1));
    static_assert(std::is_same<decltype(m16), decltype(m1)>::value);

    // check TMap(TMap &&, Allocator)
    TMap m17(std::move(m5), std::allocator<int>());
    static_assert(std::is_same<decltype(m17), decltype(m5)>::value);
}
#endif

#if __TBB_UNORDERED_NODE_HANDLE_PRESENT

void TestMerge(){
    using Map = tbb::concurrent_unordered_map<int, int>;
    using MultiMap = tbb::concurrent_unordered_multimap<int, int>;

    Map map_for_merge1;
    map_for_merge1.insert({1, 10});
    map_for_merge1.insert({2, 20});
    map_for_merge1.insert({3, 30});
    Map map_for_merge2;
    map_for_merge2.insert({1, 40});
    map_for_merge2.insert({2, 20});
    map_for_merge2.insert({9, 90});

    MultiMap multimap_for_merge1;
    multimap_for_merge1.insert({1, 10});
    multimap_for_merge1.insert({1, 10});
    multimap_for_merge1.insert({2, 20});
    multimap_for_merge1.insert({3, 30});
    multimap_for_merge1.insert({4, 40});
    MultiMap multimap_for_merge2;
    multimap_for_merge2.insert({1, 10});
    multimap_for_merge2.insert({2, 50});
    multimap_for_merge2.insert({5, 60});
    multimap_for_merge2.insert({5, 70});

    node_handling::TestMergeTransposition(map_for_merge1, map_for_merge2,
                                          multimap_for_merge1, multimap_for_merge2);

    // Test merge with different hashers
    tbb::concurrent_unordered_map<int, int, degenerate_hash<int>> degenerate_hash_map;
    degenerate_hash_map.insert({1, 10});
    degenerate_hash_map.insert({2, 20});
    degenerate_hash_map.insert({9, 90});

    tbb::concurrent_unordered_multimap<int, int, degenerate_hash<int>> degenerate_hash_multimap;
    degenerate_hash_multimap.insert({1, 10});
    degenerate_hash_multimap.insert({2, 20});
    degenerate_hash_multimap.insert({5, 50});
    degenerate_hash_multimap.insert({5, 60});
    degenerate_hash_multimap.insert({6, 70});

    node_handling::TestMergeOverloads(map_for_merge1, degenerate_hash_map);
    node_handling::TestMergeOverloads(multimap_for_merge1, degenerate_hash_multimap);

    int size = 100000;

    Map map_for_merge3(size);
    for (int i = 0; i<size; i++){
        map_for_merge3.insert({i,i});
    }
    node_handling::TestConcurrentMerge(map_for_merge3);

    MultiMap multimap_for_merge3(size/2);
    for (int i = 0; i<size/2; i++){
        multimap_for_merge3.insert({i,i});
        multimap_for_merge3.insert({i,i});
    }
    node_handling::TestConcurrentMerge(multimap_for_merge3);
}

void TestNodeHandling() {
    tbb::concurrent_unordered_map<int, int> unordered_map;
    for (int i = 1; i<5; i++)
        unordered_map.insert({i,i*10});
    node_handling::NodeHandlingTests(unordered_map, /*new key for test_data*/{5,90});

    tbb::concurrent_unordered_multimap<int, int> unordered_multimap;
    for (int i = 1; i<5; i++)
        unordered_multimap.insert({i,i*10});
    unordered_multimap.insert({2, 30});
    node_handling::NodeHandlingTests(unordered_multimap, /*new key for test_data*/{5,90});
}

#endif /*__TBB_UNORDERED_NODE_HANDLE_PRESENT*/

int TestMain() {
    test_machine();

    test_basic<MyMap>( "concurrent unordered Map" );
    test_basic<MyDegenerateMap>( "concurrent unordered degenerate Map" );
    test_concurrent<MyMap>( "concurrent unordered Map" );
    test_concurrent<MyDegenerateMap>( "concurrent unordered degenerate Map" );
    test_basic<MyMultiMap>( "concurrent unordered MultiMap" );
    test_basic<MyDegenerateMultiMap>( "concurrent unordered degenerate MultiMap" );
    test_concurrent<MyMultiMap>( "concurrent unordered MultiMap" );
    test_concurrent<MyDegenerateMultiMap>( "concurrent unordered degenerate MultiMap" );
    test_concurrent<MyMultiMap>( "concurrent unordered MultiMap asymptotic", true );

    { Check<MyCheckedMap::value_type> checkit; test_basic<MyCheckedMap>( "concurrent unordered map (checked)" ); }
    { Check<MyCheckedMap::value_type> checkit; test_concurrent<MyCheckedMap>( "concurrent unordered map (checked)" ); }
    test_basic<MyCheckedStateMap>("concurrent unordered map (checked state of elements)", tbb::internal::true_type());
    test_concurrent<MyCheckedStateMap>("concurrent unordered map (checked state of elements)");

    { Check<MyCheckedMultiMap::value_type> checkit; test_basic<MyCheckedMultiMap>( "concurrent unordered MultiMap (checked)" ); }
    { Check<MyCheckedMultiMap::value_type> checkit; test_concurrent<MyCheckedMultiMap>( "concurrent unordered MultiMap (checked)" ); }

#if __TBB_INITIALIZER_LISTS_PRESENT
    TestInitList< tbb::concurrent_unordered_map<int, int>,
                  tbb::concurrent_unordered_multimap<int, int> >( {{1,1},{2,2},{3,3},{4,4},{5,5}} );
#endif /* __TBB_INITIALIZER_LISTS_PRESENT */

#if __TBB_RANGE_BASED_FOR_PRESENT
    TestRangeBasedFor();
#endif

#if __TBB_CPP11_RVALUE_REF_PRESENT
    test_rvalue_ref_support<cu_map_type>( "concurrent unordered map" );
    test_rvalue_ref_support<cu_multimap_type>( "concurrent unordered multimap" );
#endif /* __TBB_CPP11_RVALUE_REF_PRESENT */

#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT
    TestDeductionGuides<tbb::concurrent_unordered_map>();
    TestDeductionGuides<tbb::concurrent_unordered_multimap>();
#endif

    TestTypes();

#if __TBB_UNORDERED_NODE_HANDLE_PRESENT
    TestNodeHandling ();
    TestMerge();
#endif /*__TBB_UNORDERED_NODE_HANDLE_PRESENT*/

    return Harness::Done;
}
