/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

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

#ifndef harness_iterator_H
#define harness_iterator_H

#include <iterator>
#include <memory>

namespace Harness {

template <class T>
class InputIterator {
    T * my_ptr;
public:
#if HARNESS_EXTENDED_STD_COMPLIANCE
    typedef std::input_iterator_tag iterator_category;
    typedef T value_type;
    typedef typename std::allocator<T>::difference_type difference_type;
    typedef typename std::allocator<T>::pointer pointer;
    typedef typename std::allocator<T>::reference reference;
#endif /* HARNESS_EXTENDED_STD_COMPLIANCE */

    explicit InputIterator( T * ptr): my_ptr(ptr){}

    T& operator* () { return *my_ptr; }

    InputIterator& operator++ () { ++my_ptr; return *this; }

    bool operator== ( const InputIterator& r ) { return my_ptr == r.my_ptr; }
};

template <class T>
class ForwardIterator {
    T * my_ptr;
public:
#if HARNESS_EXTENDED_STD_COMPLIANCE
    typedef std::forward_iterator_tag iterator_category;
    typedef T value_type;
    typedef typename std::allocator<T>::difference_type difference_type;
    typedef typename std::allocator<T>::pointer pointer;
    typedef typename std::allocator<T>::reference reference;
#endif /* HARNESS_EXTENDED_STD_COMPLIANCE */

    explicit ForwardIterator ( T * ptr ) : my_ptr(ptr){}

    ForwardIterator ( const ForwardIterator& r ) : my_ptr(r.my_ptr){}

    T& operator* () { return *my_ptr; }

    ForwardIterator& operator++ () { ++my_ptr; return *this; }

    bool operator== ( const ForwardIterator& r ) { return my_ptr == r.my_ptr; }
};

template <class T>
class RandomIterator {
    T * my_ptr;
#if !HARNESS_EXTENDED_STD_COMPLIANCE
    typedef typename std::allocator<T>::difference_type difference_type;
#endif

public:
#if HARNESS_EXTENDED_STD_COMPLIANCE
    typedef std::random_access_iterator_tag iterator_category;
    typedef T value_type;
    typedef typename std::allocator<T>::pointer pointer;
    typedef typename std::allocator<T>::reference reference;
    typedef typename std::allocator<T>::difference_type difference_type;
#endif /* HARNESS_EXTENDED_STD_COMPLIANCE */

    explicit RandomIterator ( T * ptr ) : my_ptr(ptr){}
    RandomIterator ( const RandomIterator& r ) : my_ptr(r.my_ptr){}
    T& operator* () { return *my_ptr; }
    RandomIterator& operator++ () { ++my_ptr; return *this; }
    bool operator== ( const RandomIterator& r ) { return my_ptr == r.my_ptr; }
    bool operator!= ( const RandomIterator& r ) { return my_ptr != r.my_ptr; }
    difference_type operator- (const RandomIterator &r) const {return my_ptr - r.my_ptr;}
    RandomIterator operator+ (difference_type n) {return RandomIterator(my_ptr + n);}
    bool operator< (const RandomIterator &r) const {return my_ptr < r.my_ptr;}
};

template <class T>
class ConstRandomIterator {
    const T * my_ptr;
#if !HARNESS_EXTENDED_STD_COMPLIANCE
    typedef typename std::allocator<T>::difference_type difference_type;
#endif

public:
#if HARNESS_EXTENDED_STD_COMPLIANCE
    typedef std::random_access_iterator_tag iterator_category;
    typedef T value_type;
    typedef typename std::allocator<T>::const_pointer pointer;
    typedef typename std::allocator<T>::const_reference reference;
    typedef typename std::allocator<T>::difference_type difference_type;
#endif /* HARNESS_EXTENDED_STD_COMPLIANCE */

    explicit ConstRandomIterator ( const T * ptr ) : my_ptr(ptr){}
    ConstRandomIterator ( const ConstRandomIterator& r ) : my_ptr(r.my_ptr){}
    const T& operator* () { return *my_ptr; }
    ConstRandomIterator& operator++ () { ++my_ptr; return *this; }
    bool operator== ( const ConstRandomIterator& r ) { return my_ptr == r.my_ptr; }
    bool operator!= ( const ConstRandomIterator& r ) { return my_ptr != r.my_ptr; }
    difference_type operator- (const ConstRandomIterator &r) const {return my_ptr - r.my_ptr;}
    ConstRandomIterator operator+ (difference_type n) {return ConstRandomIterator(my_ptr + n);}
    bool operator< (const ConstRandomIterator &r) const {return my_ptr < r.my_ptr;}
};

} // namespace Harness

#if !HARNESS_EXTENDED_STD_COMPLIANCE
namespace std {
    template<typename T>
    struct iterator_traits< Harness::InputIterator<T> > {
        typedef std::input_iterator_tag iterator_category;
        typedef T value_type;
        typedef value_type& reference;
    };

    template<typename T>
    struct iterator_traits< Harness::ForwardIterator<T> > {
        typedef std::forward_iterator_tag iterator_category;
        typedef T value_type;
        typedef value_type& reference;
    };

    template<typename T>
    struct iterator_traits< Harness::RandomIterator<T> > {
        typedef std::random_access_iterator_tag iterator_category;
        typedef T value_type;
        typedef value_type& reference;
    };

    template<typename T>
    struct iterator_traits< Harness::ConstRandomIterator<T> > {
        typedef std::random_access_iterator_tag iterator_category;
        typedef T value_type;
        typedef const value_type& reference;
    };
} // namespace std
#endif /* !HARNESS_EXTENDED_STD_COMPLIANCE */

#endif //harness_iterator_H
