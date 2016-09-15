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

#include "tbb/tbb_stddef.h"

void TestInsideFunction(){
    __TBB_STATIC_ASSERT(sizeof(char)>=1,"");
}
void TestTwiceAtTheSameLine(){
//    for current implementation it is not possible to use
//    two __TBB_STATIC_ASSERT on a same line
//    __TBB_STATIC_ASSERT(true,""); __TBB_STATIC_ASSERT(true,"");
}

void TestInsideStructure(){
    struct helper{
        __TBB_STATIC_ASSERT(true,"");
    };
}

void TestTwiceInsideStructure(){
    struct helper{
        //for current implementation it is not possible to use
        //two __TBB_STATIC_ASSERT on a same line inside a class definition
        //__TBB_STATIC_ASSERT(true,"");__TBB_STATIC_ASSERT(true,"");

        __TBB_STATIC_ASSERT(true,"");
        __TBB_STATIC_ASSERT(true,"");
    };
}

namespace TestTwiceInsideNamespaceHelper{
    __TBB_STATIC_ASSERT(true,"");
    __TBB_STATIC_ASSERT(true,"");
}

namespace TestTwiceInsideClassTemplateHelper{
    template <typename T>
    struct template_struct{
        __TBB_STATIC_ASSERT(true,"");
        __TBB_STATIC_ASSERT(true,"");
    };
}

void TestTwiceInsideTemplateClass(){
    using namespace TestTwiceInsideClassTemplateHelper;
    typedef template_struct<int> template_struct_int_typedef;
    typedef template_struct<char> template_struct_char_typedef;
    tbb::internal::suppress_unused_warning(template_struct_int_typedef(), template_struct_char_typedef());
}

template<typename T>
void TestTwiceInsideTemplateFunction(){
    __TBB_STATIC_ASSERT(sizeof(T)>=1,"");
    __TBB_STATIC_ASSERT(true,"");
}

#include "harness.h"
int TestMain() {
    #if __TBB_STATIC_ASSERT_PRESENT
        REPORT("Known issue: %s\n", "no need to test ad-hoc implementation as native feature of C++11 is used");
        return Harness::Skipped;
    #else
        TestInsideFunction();
        TestInsideStructure();
        TestTwiceAtTheSameLine();
        TestTwiceInsideStructure();
        TestTwiceInsideTemplateClass();
        TestTwiceInsideTemplateFunction<char>();
        return Harness::Done;
    #endif
}
