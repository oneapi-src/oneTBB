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

#ifndef coarse_grained_raii_lru_cache_H
#define coarse_grained_raii_lru_cache_H

#include <map>
#include <list>

#include "tbb/spin_mutex.h"
#include "tbb/tbb_stddef.h"
template <typename key_type, typename value_type, typename value_functor_type = value_type (*)(key_type) >
class coarse_grained_raii_lru_cache : tbb::internal::no_assign{
    typedef value_functor_type value_function_type;

    typedef std::size_t ref_counter_type;
    struct map_value_type;
    typedef std::map<key_type, map_value_type> map_storage_type;
    typedef std::list<typename map_storage_type::iterator> lru_list_type;
    struct map_value_type {
        value_type my_value;
        ref_counter_type my_ref_counter;
        typename lru_list_type::iterator my_lru_list_iterator;
        bool my_is_ready;

        map_value_type (value_type const& a_value,  ref_counter_type a_ref_counter,    typename lru_list_type::iterator a_lru_list_iterator, bool a_is_ready)
            : my_value(a_value), my_ref_counter(a_ref_counter), my_lru_list_iterator (a_lru_list_iterator)
            ,my_is_ready(a_is_ready)
        {}
    };

    class handle_object;
public:
    typedef handle_object handle;

    coarse_grained_raii_lru_cache(value_function_type f, std::size_t number_of_lru_history_items): my_value_function(f),my_number_of_lru_history_items(number_of_lru_history_items){}
    handle_object operator[](key_type k){
        tbb::spin_mutex::scoped_lock lock(my_mutex);
        bool is_new_value_needed = false;
        typename map_storage_type::iterator it = my_map_storage.find(k);
        if (it == my_map_storage.end()){
            it = my_map_storage.insert(it,std::make_pair(k,map_value_type(value_type(),0,my_lru_list.end(),false)));
            is_new_value_needed = true;
        }else {
            typename lru_list_type::iterator list_it = it->second.my_lru_list_iterator;
            if (list_it!=my_lru_list.end()) {
                my_lru_list.erase(list_it);
                it->second.my_lru_list_iterator= my_lru_list.end();
            }
        }
        typename map_storage_type::reference value_ref = *it;
        //increase ref count
        ++(value_ref.second.my_ref_counter);
        if (is_new_value_needed){
            lock.release();
            value_ref.second.my_value = my_value_function(k);
            __TBB_store_with_release(value_ref.second.my_is_ready, true);

        }else{
            if (!value_ref.second.my_is_ready){
                lock.release();
                tbb::internal::spin_wait_while_eq(value_ref.second.my_is_ready,false);
            }
        }
        return handle_object(*this,(value_ref));
    }
private:
    void signal_end_of_usage(typename map_storage_type::reference value_ref){
        tbb::spin_mutex::scoped_lock lock(my_mutex);
        typename map_storage_type::iterator it = my_map_storage.find(value_ref.first);
        __TBB_ASSERT(it!=my_map_storage.end(),"cache should not return past-end iterators to outer world");
        __TBB_ASSERT(&(*it) == &value_ref,"dangling reference has been returned to outside world? data race ?");
        __TBB_ASSERT( my_lru_list.end()== std::find(my_lru_list.begin(),my_lru_list.end(),it),
                "object in use should not be in list of unused objects ");
        if (! --(it->second.my_ref_counter)){ //decrease ref count, and check if it was the last reference
            if (my_lru_list.size()>=my_number_of_lru_history_items){
                size_t number_of_elements_to_evict = 1 + my_lru_list.size() - my_number_of_lru_history_items;
                for (size_t i=0; i<number_of_elements_to_evict; ++i){
                    typename map_storage_type::iterator it_to_evict = my_lru_list.back();
                    my_lru_list.pop_back();
                    my_map_storage.erase(it_to_evict);
                }
            }
            my_lru_list.push_front(it);
            it->second.my_lru_list_iterator = my_lru_list.begin();
        }
    }
private:
    value_function_type my_value_function;
    std::size_t const my_number_of_lru_history_items;
    map_storage_type my_map_storage;
    lru_list_type my_lru_list;
    tbb::spin_mutex my_mutex;
private:
    struct handle_move_t:tbb::internal::no_assign{
        coarse_grained_raii_lru_cache & my_cache_ref;
        typename map_storage_type::reference my_value_ref;
        handle_move_t(coarse_grained_raii_lru_cache & cache_ref, typename map_storage_type::reference value_ref):my_cache_ref(cache_ref),my_value_ref(value_ref) {};
    };
    class handle_object {
        coarse_grained_raii_lru_cache * my_cache_pointer;
        typename map_storage_type::reference my_value_ref;
    public:
        handle_object(coarse_grained_raii_lru_cache & cache_ref, typename map_storage_type::reference value_ref):my_cache_pointer(&cache_ref), my_value_ref(value_ref) {}
        handle_object(handle_move_t m):my_cache_pointer(&m.my_cache_ref), my_value_ref(m.my_value_ref){}
        operator handle_move_t(){ return move(*this);}
        value_type& value(){return my_value_ref.second.my_value;}
        ~handle_object(){
            if (my_cache_pointer){
                my_cache_pointer->signal_end_of_usage(my_value_ref);
            }
        }
    private:
        friend handle_move_t move(handle_object& h){
            return handle_object::move(h);
        }
        static handle_move_t move(handle_object& h){
            __TBB_ASSERT(h.my_cache_pointer,"move from the same object twice ?");
            coarse_grained_raii_lru_cache * cache_pointer = NULL;
            std::swap(cache_pointer,h.my_cache_pointer);
            return handle_move_t(*cache_pointer,h.my_value_ref);
        }
    private:
        void operator=(handle_object&);
        handle_object(handle_object &);
    };
};
#endif //coarse_grained_raii_lru_cache_H
