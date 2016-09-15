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

#ifndef tbb_tests_harness_checktype_H
#define tbb_tests_harness_checktype_H

// type that checks construction and destruction.

#ifndef __HARNESS_CHECKTYPE_DEFAULT_CTOR
    #define __HARNESS_CHECKTYPE_DEFAULT_CTOR 1
#endif

template<class Counter>
class check_type : Harness::NoAfterlife {
    Counter id;
    bool am_ready;
public:
    static tbb::atomic<int> check_type_counter;
    // if only non-default constructors are desired, set __HARNESS_CHECKTYPE_NODEFAULT_CTOR
    check_type(Counter _n
#if __HARNESS_CHECKTYPE_DEFAULT_CTOR
            = 0
#endif
            ) : id(_n), am_ready(false) {
        ++check_type_counter;
    }

    check_type(const check_type& other) : Harness::NoAfterlife(other) {
        other.AssertLive();
        AssertLive();
        id = other.id;
        am_ready = other.am_ready;
        ++check_type_counter;
    }

    operator int() const { return (int)my_id(); }
    check_type& operator++() { ++id; return *this;; }

    ~check_type() { 
        AssertLive(); 
        --check_type_counter;
        ASSERT(check_type_counter >= 0, "too many destructions");
    }

    check_type &operator=(const check_type &other) {
        other.AssertLive();
        AssertLive();
        id = other.id;
        am_ready = other.am_ready;
        return *this;
    }

    Counter my_id() const { AssertLive(); return id; }
    bool is_ready() { AssertLive(); return am_ready; }
    void function() {
        AssertLive();
        if( id == (Counter)0 ) {
            id = (Counter)1;
            am_ready = true;
        }
    }

};

template<class Counter>
tbb::atomic<int> check_type<Counter>::check_type_counter;

// provide a class that for a check_type will initialize the counter on creation, and on
// destruction will check that the constructions and destructions of check_type match.
template<class MyClass>
struct Check {
    Check() {}   // creation does nothing
    ~Check() {}  // destruction checks nothing
};

template<class Counttype>
struct Check<check_type< Counttype > > {
    Check() { check_type<Counttype>::check_type_counter = 0; }
    ~Check() { ASSERT(check_type<Counttype>::check_type_counter == 0, "check_type constructions and destructions don't match"); }
};

#endif  // tbb_tests_harness_checktype_H
