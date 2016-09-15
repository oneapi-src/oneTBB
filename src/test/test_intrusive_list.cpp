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

#define HARNESS_NO_PARSE_COMMAND_LINE 1
#include "harness.h"

#include "../tbb/intrusive_list.h"

using tbb::internal::intrusive_list_node;

// Machine word filled with repeated pattern of FC bits
const uintptr_t NoliMeTangere = ~uintptr_t(0)/0xFF*0xFC;

struct VerificationBase : Harness::NoAfterlife {
    uintptr_t m_Canary;
    VerificationBase () : m_Canary(NoliMeTangere) {}
};

struct DataItemWithInheritedNodeBase : intrusive_list_node {
    int m_Data;
public:
    DataItemWithInheritedNodeBase ( int value ) : m_Data(value) {}

    int Data() const { return m_Data; }
};

class DataItemWithInheritedNode : public VerificationBase, public DataItemWithInheritedNodeBase {
    friend class tbb::internal::intrusive_list<DataItemWithInheritedNode>;
public:
    DataItemWithInheritedNode ( int value ) : DataItemWithInheritedNodeBase(value) {}
};

struct DataItemWithMemberNodeBase {
    int m_Data;
public:
    // Cannot be used by member_intrusive_list to form lists of objects derived from DataItemBase
    intrusive_list_node m_BaseNode;

    DataItemWithMemberNodeBase ( int value ) : m_Data(value) {}

    int Data() const { return m_Data; }
};

class DataItemWithMemberNodes : public VerificationBase, public DataItemWithMemberNodeBase {
public:
    intrusive_list_node m_Node;

    DataItemWithMemberNodes ( int value ) : DataItemWithMemberNodeBase(value) {}
};

typedef tbb::internal::intrusive_list<DataItemWithInheritedNode> IntrusiveList1;
typedef tbb::internal::memptr_intrusive_list<DataItemWithMemberNodes, 
        DataItemWithMemberNodeBase, &DataItemWithMemberNodeBase::m_BaseNode> IntrusiveList2;
typedef tbb::internal::memptr_intrusive_list<DataItemWithMemberNodes, 
        DataItemWithMemberNodes, &DataItemWithMemberNodes::m_Node> IntrusiveList3;

const int NumElements = 256 * 1024;

//! Iterates through the list forward and backward checking the validity of values stored by the list nodes
template<class List, class Iterator>
void CheckListNodes ( List& il, int valueStep ) {
    int i;
    Iterator it = il.begin();
    for ( i = valueStep - 1; it != il.end(); ++it, i += valueStep ) {
        ASSERT( it->Data() == i, "Unexpected node value while iterating forward" );
        ASSERT( (*it).m_Canary == NoliMeTangere, "Memory corruption" );
    }
    ASSERT( i == NumElements + valueStep - 1, "Wrong number of list elements while iterating forward" );
    it = il.end();
    for ( i = NumElements - 1, it--; it != il.end(); --it, i -= valueStep ) {
        ASSERT( (*it).Data() == i, "Unexpected node value while iterating backward" );
        ASSERT( it->m_Canary == NoliMeTangere, "Memory corruption" );
    }
    ASSERT( i == -1, "Wrong number of list elements while iterating backward" );
}

template<class List, class Item>
void TestListOperations () {
    typedef typename List::iterator iterator;
    List il;
    for ( int i = NumElements - 1; i >= 0; --i )
        il.push_front( *new Item(i) );
    CheckListNodes<const List, typename List::const_iterator>( il, 1 );
    iterator it = il.begin();
    for ( ; it != il.end(); ++it ) {
        Item &item = *it;
        it = il.erase( it );
        delete &item;
    }
    CheckListNodes<List, iterator>( il, 2 );
    for ( it = il.begin(); it != il.end(); ++it ) {
        Item &item = *it;
        il.remove( *it++ );
        delete &item;
    }
    CheckListNodes<List, iterator>( il, 4 );
}

#include "harness_bad_expr.h"

template<class List, class Item>
void TestListAssertions () {
#if TRY_BAD_EXPR_ENABLED
    tbb::set_assertion_handler( AssertionFailureHandler );
    List il1, il2;
    Item n1(1), n2(2), n3(3);
    il1.push_front(n1);
    TRY_BAD_EXPR( il2.push_front(n1), "only one intrusive list" );
    TRY_BAD_EXPR( il1.push_front(n1), "only one intrusive list" );
    il2.push_front(n2);
    TRY_BAD_EXPR( il1.remove(n3), "not in the list" );
    tbb::set_assertion_handler( ReportError );
#endif /* TRY_BAD_EXPR_ENABLED */
}

int TestMain () {
    TestListOperations<IntrusiveList1, DataItemWithInheritedNode>();
    TestListOperations<IntrusiveList2, DataItemWithMemberNodes>();
    TestListOperations<IntrusiveList3, DataItemWithMemberNodes>();
    TestListAssertions<IntrusiveList1, DataItemWithInheritedNode>();
    TestListAssertions<IntrusiveList2, DataItemWithMemberNodes>();
    TestListAssertions<IntrusiveList3, DataItemWithMemberNodes>();
    return Harness::Done;
}
