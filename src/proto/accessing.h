/*
    Copyright (C) 2005-2011, Intel Corporation

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

#ifndef __TBB_accessing_H
#define __TBB_accessing_H

namespace tbb {

//! @cond INTERNAL
namespace internal {
    class empty_deleter {
        void operator()(void*) {}
    };
} // namespace internal
//! @endcond

template<typename T, typename Synchronizer, typename Deleter=internal::empty_deleter >
class accessing : public Synchronizer {
    T my_value;
public:
    //! Rebind for different types
    template<typename U, typename S = Synchronizer, typename M = Deleter>
    struct rebind {
        typedef accessing<U, S, M> other;
    };
    
    typedef Synchronizer synchronizer_type;
    //typedef synchronizer_type::access access;
    typedef Deleter deleter_type;
    // Standard types
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;

    //! Default constructor
    accessing() {}
    //! Construct by value without access
    accessing( const_reference v ) : my_value( v ) {}
    //! Construct by value with accessor acquired
    template<typename Accessor>
    accessing( const_reference v, Accessor &a )
        : my_value( v ), Synchronizer(a) { }
    //! Return reference to underlying type.
    reference unprotected_ref() const { return const_cast<reference>(my_value); }

    //! Combines data access and locking
    template<typename Access>
    class accessor : protected access_holder<Access> {
        void operator=( const accessor & ) const; // Deny access
    public:
        // Standard types
        typedef T value_type;
        typedef value_type *pointer;
        typedef const value_type *const_pointer;
        typedef value_type &reference;
        typedef const value_type &const_reference;
        typedef Deleter deleter_type;

        //! Create empty accessor
        accessor( const deleter_type &d = deleter_type() ) : my_node(NULL), deleter(d) {}
        //! Move and downgrade
        accessor( const accessor & );
        //! Move
        accessor( const const_accessor & );
        //! Destroy result after releasing the underlying reference.
        ~accessor() {
            // do not call deleter(my_node); here due to semantics of simple??? synchronizer
            my_node = NULL; // my_lock.release() is called in scoped_lock destructor
        }
        //! Attempt to acquire an access right. Returns true if successful; false otherwise
        bool try_acquire( simple_synchronizer &item ) {
            if( !scoped_t::try_acquire( item, false ) )
                return false;
            my_node = &item;
            return true;
        }
        //! True if result is empty.
        bool empty() const { return !my_node; }
        //! True if holder is empty.
        bool owns_lock() const;
        //! Synonym of empty()
        explicit operator bool() const { return !my_node; }
        //! Release access, return synchronizer
        simple_synchronizer *release() {
            if( simple_synchronizer *n = my_node ) {
                scoped_t::release();
                my_node = 0;
                return n;
            }
            return 0;
        }
        //! Return reference to associated value in hash table.
        reference operator*() const { return const_accessor::internal_get_value(); }
        //! Return pointer to associated value in hash table.
        pointer operator->() const { return &const_accessor::internal_get_value(); }
        //! Return pointer to associated value in hash table.
        pointer get() const { return &const_accessor::internal_get_value(); }
        //! Return reference to deleter
        deleter_type &get_deleter() { return deleter; }
        
    private:
        friend simple_synchronizer;
        reference internal_get_value() const {
            __TBB_ASSERT( my_node, "attempt to dereference empty accessor" );
            return my_node->my_value;
        }
        simple_synchronizer *my_node;
        deleter_type deleter;
    };
};

}//namespace tbb

#endif//__TBB_accessing_H
