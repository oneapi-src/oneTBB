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

#ifndef __TBB_guarded_H
#define __TBB_guarded_H

#include "tbb/spin_mutex.h"

namespace tbb {

template <typename T, class M = tbb::spin_mutex>
class guarded;

//! @cond INTERNAL
namespace internal {
    //! The class is used to access data from guarded<T> class.
    template <typename T, class M>
    class guarded_accessor_provider {
        void operator=( const guarded_accessor_provider& ) const; // Deny access
        guarded_accessor_provider( const guarded_accessor_provider& ); // Deny access
    public:
        //! True if the accessor is empty.
        bool empty() const {
            return !my_guarded;
        }

        //! Create empty result
        guarded_accessor_provider() {}

        //! Destroy result after releasing the underlying reference.
        ~guarded_accessor_provider() {
            my_guarded = NULL; // my_lock.release() is called in scoped_lock destructor
        }

        //! Constructor for guarded<T>
        guarded_accessor_provider(guarded<T, M>& guarded_item) {
            my_guarded = &guarded_item;
            my_lock.acquire (guarded_item.my_mutex);
        }

        //! Return reference to guarded value
        T& operator*() const {
            __TBB_ASSERT( my_guarded, "attempt to dereference empty accessor" );
            return my_guarded->my_value;
        }

        //! Return pointer to guarded value
        T* operator->() const {
            return &operator*();
        }

        //! Explicit locking
        void acquire() {
            my_lock.acquire(my_guarded->my_mutex);
        }

        bool try_acquire() {
            return my_lock.try_acquire(my_guarded->my_mutex);
        }

        //! Explicit releasing
        void release() {
            my_lock.release();
        }

        //! Changes the guarded<T> value the accessor holds
        void acquire(guarded<T, M>& item) {
            my_lock.acquire(item.my_mutex);
            my_guarded = &item;
        }

    protected:
        guarded<T, M>* my_guarded;
        typename M::scoped_lock my_lock;
    };

    //! Const accessor class. It is used to access read-only data
    template <typename T, class M, bool is_rw_mutex> 
    class guarded_const_accessor_provider : public guarded_accessor_provider<T, M> {
    public:
        //! Constructor for guarded<T>
        guarded_const_accessor_provider(guarded<T, M>& guarded_item) {
            this->my_guarded = &guarded_item;
            this->my_lock.acquire (guarded_item.my_mutex);
        }

        //! Return reference to guarded value
        const T& operator*() const {
            __TBB_ASSERT( this->my_guarded, "attempt to dereference empty accessor" );
            return this->my_guarded->my_value;
        }

        //! Return pointer to guarded value
        const T* operator->() const {
            return &operator*();
        }
    };

    //! Explicit specialization of const_accessor for readerwriter mutex
    template<typename T, class M>
    class guarded_const_accessor_provider<T, M, true> : public guarded_accessor_provider<T, M>{
    public:
        //! Constructor for guarded<T>
        guarded_const_accessor_provider(guarded<T, M>& guarded_item) {
            this->my_guarded = &guarded_item;
            this->my_lock.acquire (guarded_item.my_mutex, false);
        }

        //! Explicit locking
        void acquire() {
            this->my_lock.acquire(this->my_guarded->my_mutex, false);
        }

        bool try_acquire() {
            return this->my_lock.try_acquire(this->my_guarded->my_mutex, false);
        }

        //! Changes the guarded<T> value the accessor holds
        void acquire(guarded<T, M>& item) {
            this->my_lock.acquire(item.my_mutex, false);
            this->my_guarded = &item;
        }

        //! Return reference to guarded value
        const T& operator*() const {
            __TBB_ASSERT( this->my_guarded, "attempt to dereference empty accessor" );
            return this->my_guarded->my_value;
        }

        //! Return pointer to guarded value
        const T* operator->() const {
            return &operator*();
        }
    };
}// end of namespace internal

//! The class provides locked access to data
template <typename T, class M> 
class guarded {
public:
    //! Constructor
    guarded (T value): my_value(value), my_mutex(){};
    //! Copy constructor
    guarded (const guarded& guarded_item): my_mutex(){ 
        const_accessor ca_item(const_cast<guarded &> (guarded_item));
        my_value = guarded_item.my_value;
    }
    //! Default constructor
    guarded (): my_value(), my_mutex(){}; 
    ~guarded(){};
    // ! operator =
    // It is required by TBB containers
    guarded& operator =(const guarded& guarded_item) {
        T tmp_value;
        {
            const_accessor ca_item (const_cast<guarded &> (guarded_item));
            tmp_value = guarded_item.my_value;
        }
        {
            accessor a_item (*this);
            my_value = tmp_value;
        }
        return *this; 
    } 

    friend class internal::guarded_accessor_provider<T, M>;
    friend class internal::guarded_const_accessor_provider<T, M, M::is_rw_mutex>;

    typedef internal::guarded_accessor_provider<T, M> accessor; 
    typedef internal::guarded_const_accessor_provider<T, M, M::is_rw_mutex> const_accessor; 
private:
    T my_value;
    mutable M my_mutex;
};

}// namespace tbb

#endif /* __TBB_guarded_H */
