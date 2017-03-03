/*
    Copyright (c) 2005-2017 Intel Corporation

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

#ifndef __TBB__flow_graph_async_msg_impl_H
#define __TBB__flow_graph_async_msg_impl_H

#ifndef __TBB_flow_graph_H
#error Do not #include this internal file directly; use public TBB headers instead.
#endif

// included in namespace tbb::flow::interfaceX (in flow_graph.h)

template< typename T > class async_msg;

namespace internal {

template< typename T, typename = void >
struct async_helpers {
    typedef async_msg<T> async_type;
    typedef T filtered_type;

    static const bool is_async_type = false;

    static const void* to_void_ptr(const T& t) {
        return static_cast<const void*>(&t);
    }

    static void* to_void_ptr(T& t) {
        return static_cast<void*>(&t);
    }

    static const T& from_void_ptr(const void* p) {
        return *static_cast<const T*>(p);
    }

    static T& from_void_ptr(void* p) {
        return *static_cast<T*>(p);
    }

    static task* try_put_task_wrapper_impl( receiver<T>* const this_recv, const void *p, bool is_async ) {
        if ( is_async ) {
            // This (T) is NOT async and incoming 'A<X> t' IS async
            // Get data from async_msg
            const async_msg<filtered_type>& msg = async_helpers< async_msg<filtered_type> >::from_void_ptr(p);
            task* const new_task = msg.my_storage->subscribe(*this_recv);
            // finalize() must be called after subscribe() because set() can be called in finalize()
            // and 'this_recv' client must be subscribed by this moment
            msg.finalize();
            return new_task;
        } else {
            // Incoming 't' is NOT async
            return this_recv->try_put_task( from_void_ptr(p) );
        }
    }
};

template< typename T >
struct async_helpers< T, typename std::enable_if< std::is_base_of<async_msg<typename T::async_msg_data_type>, T>::value >::type > {
    typedef T async_type;
    typedef typename T::async_msg_data_type filtered_type;

    static const bool is_async_type = true;

    // Receiver-classes use const interfaces
    static const void* to_void_ptr(const T& t) {
        return static_cast<const void*>( &static_cast<const async_msg<filtered_type>&>(t) );
    }

    static void* to_void_ptr(T& t) {
        return static_cast<void*>( &static_cast<async_msg<filtered_type>&>(t) );
    }

    // Sender-classes use non-const interfaces
    static const T& from_void_ptr(const void* p) {
        return *static_cast<const T*>( static_cast<const async_msg<filtered_type>*>(p) );
    }

    static T& from_void_ptr(void* p) {
        return *static_cast<T*>( static_cast<async_msg<filtered_type>*>(p) );
    }

    // Used in receiver<T> class
    static task* try_put_task_wrapper_impl(receiver<T>* const this_recv, const void *p, bool is_async) {
        if ( is_async ) {
            // Both are async
            return this_recv->try_put_task( from_void_ptr(p) );
        } else {
            // This (T) is async and incoming 'X t' is NOT async
            // Create async_msg for X
            const filtered_type& t = async_helpers<filtered_type>::from_void_ptr(p);
            const T msg(t);
            return this_recv->try_put_task(msg);
        }
    }
};

template <typename T>
class async_storage {
public:
    typedef receiver<T> async_storage_client;

    async_storage() { my_data_ready.store<tbb::relaxed>(false); }

    template<typename C>
    async_storage(C&& data) : my_data( std::forward<C>(data) ) {
        using namespace tbb::internal;
        __TBB_STATIC_ASSERT( (is_same_type<typename strip<C>::type, typename strip<T>::type>::value), "incoming type must be T" );

        my_data_ready.store<tbb::relaxed>(true);
    }

    template<typename C>
    bool set(C&& data) {
        using namespace tbb::internal;
        __TBB_STATIC_ASSERT( (is_same_type<typename strip<C>::type, typename strip<T>::type>::value), "incoming type must be T" );

        {
            tbb::spin_mutex::scoped_lock locker(my_mutex);

            if (my_data_ready.load<tbb::relaxed>()) {
                __TBB_ASSERT(false, "double set() call");
                return false;
            }

            my_data = std::forward<C>(data);
            my_data_ready.store<tbb::release>(true);
        }

        // Thread sync is on my_data_ready flag
        for (typename subscriber_list_type::iterator it = my_clients.begin(); it != my_clients.end(); ++it) {
            (*it)->try_put(my_data);
        }

        return true;
    }

    task* subscribe(async_storage_client& client) {
        if (! my_data_ready.load<tbb::acquire>())
        {
            tbb::spin_mutex::scoped_lock locker(my_mutex);

            if (! my_data_ready.load<tbb::relaxed>()) {
#if TBB_USE_ASSERT
                for (typename subscriber_list_type::iterator it = my_clients.begin(); it != my_clients.end(); ++it) {
                    __TBB_ASSERT(*it != &client, "unexpected double subscription");
                }
#endif // TBB_USE_ASSERT

                // Subscribe
                my_clients.push_back(&client);
                return SUCCESSFULLY_ENQUEUED;
            }
        }

        __TBB_ASSERT(my_data_ready.load<tbb::relaxed>(), "data is NOT ready");
        return client.try_put_task(my_data);
    }

private:
    tbb::spin_mutex my_mutex;

    tbb::atomic<bool> my_data_ready;
    T my_data;

    typedef std::vector<async_storage_client*> subscriber_list_type;
    subscriber_list_type my_clients;
};

} // namespace internal

template <typename T>
class async_msg {
    template< typename > friend class receiver;
    template< typename, typename > friend struct internal::async_helpers;
public:
    typedef T async_msg_data_type;

    async_msg() : my_storage(std::make_shared< internal::async_storage<T> >()) {}

    async_msg(const T& t) : my_storage(std::make_shared< internal::async_storage<T> >(t)) {}

    async_msg(T&& t) : my_storage(std::make_shared< internal::async_storage<T> >( std::move(t) )) {}

    virtual ~async_msg() {}

    void set(const T& t) {
        my_storage->set(t);
    }

    void set(T&& t) {
        my_storage->set( std::move(t) );
    }

protected:
    // Can be overridden in derived class to inform that 
    // async calculation chain is over
    virtual void finalize() const {}

private:
    typedef std::shared_ptr< internal::async_storage<T> > async_storage_ptr;
    async_storage_ptr my_storage;
};

#endif  // __TBB__flow_graph_async_msg_impl_H
