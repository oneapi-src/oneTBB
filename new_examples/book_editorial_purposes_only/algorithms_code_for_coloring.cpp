/*
    Copyright (c) 2024 Intel Corporation

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

// this pseudo-code was used in the book "Today's TBB" (2015)
// it serves no other purpose other than to be here to verify compilation,
// and provide consist code coloring for the book

// Defined in header <oneapi/tbb/parallel_invoke.h>

namespace oneapi {
    namespace tbb {

        template<typename... Functions>
        void parallel_invoke(Functions&&... fs);

    } // namespace tbb
} // namespace oneapi


// Defined in header <oneapi/tbb/parallel_for.h>

namespace oneapi {
    namespace tbb {

        //! Parallel iteration over a range of integers, with no step
        template<typename Index, typename Func>
        void parallel_for(Index first, Index last, const Func& f, 
        /* see-below */ partitioner, task_group_context& context);
        template<typename Index, typename Func>
        void parallel_for(Index first, Index last, const Func& f, 
        task_group_context& context);
        template<typename Index, typename Func>
        void parallel_for(Index first, Index last, const Func& f, 
        /* see-below */ partitioner);
        template<typename Index, typename Func>
        void parallel_for(Index first, Index last, const Func& f);

        //! Parallel iteration over a range of integers, with step
        template<typename Index, typename Func>
        void parallel_for(Index first, Index last, Index step, const Func& f, 
        /* see-below */ partitioner, task_group_context& context);
        template<typename Index, typename Func>
        void parallel_for(Index first, Index last, Index step, const Func& f, 
        task_group_context& context);
        template<typename Index, typename Func>
        void parallel_for(Index first, Index last, Index step, const Func& f, 
        /* see-below */ partitioner);
        template<typename Index, typename Func>
        void parallel_for(Index first, Index last, Index step, const Func& f);

        //! Parallel iteration over TBB range
        template<typename Range, typename Body>
        void parallel_for(const Range& range, const Body& body, 
        /* see-below */ partitioner, task_group_context& context);
        template<typename Range, typename Body>
        void parallel_for(const Range& range, const Body& body, 
        task_group_context& context);
        template<typename Range, typename Body>
        void parallel_for(const Range& range, const Body& body, 
        /* see-below */ partitioner);
        template<typename Range, typename Body>
        void parallel_for(const Range& range, const Body& body);

    } // namespace tbb
} // namespace oneapi


for (int i = 0; i < N; ++i) {
    a[i] = f(a[i]);
}

tbb::parallel_for(0, N, [&](int i) {
    a[i] = f(a[i]));
});


for (int i = 0; i < N; ++i) {
    a[i] = a[i-1] + 1;
}


while (auto i = get_image()) {
    f(i);
}

std::list<image_type> my_images = get_image_list();
for (auto &i : my_list) {
    f(i);
}



// Defined in header <oneapi/tbb/parallel_for_each.h>

namespace oneapi {
    namespace tbb {

        //! Parallel iteration over a range, with optional addition of more work.
        template<typename InputIterator, typename Body>
        void parallel_for_each( InputIterator first, InputIterator last, Body body );
        template<typename InputIterator, typename Body>
        void parallel_for_each( InputIterator first, InputIterator last, Body body, task_group_context& context );

        template<typename Container, typename Body>
        void parallel_for_each( Container& c, Body body );
        template<typename Container, typename Body>
        void parallel_for_each( Container& c, Body body, task_group_context& context );

        template<typename Container, typename Body>
        void parallel_for_each( const Container& c, Body body );
        template<typename Container, typename Body>
        void parallel_for_each( const Container& c, Body body, task_group_context& context );

    } // namespace tbb
} // namespace oneapi



float r = 0.0;
for (uint64_t i = 0; i < N; ++i) {
  r += 1.0;
}
std: :cout << "in-order sum == " << r << std :: endl;




float tmp1 = 0.0, tmp2 = 0.0;
for (uint64_t i = 0; i < N/2; ++i)
  tmp1 += 1.0;
for (uint64_t i = N/2; i < N; ++i)
  tmp2 += 1.0;
float r = tmp1 + tmp2;
std :: cout << "associative sum == " << r << std :: endl;


// Defined in header <oneapi/tbb/parallel_reduce.h>

namespace oneapi {
    namespace tbb {

        //! Lambda-friendly signatures
        template<typename Range, typename Value, 
                 typename Func, typename Reduction>
        Value parallel_reduce(const Range& range, const Value& identity,
                    const Func& func, const Reduction& reduction,
                    /* see-below */ partitioner, task_group_context& context);
        template<typename Range, typename Value,
                 typename Func, typename Reduction>
        Value parallel_reduce(const Range& range, const Value& identity,
                    const Func& func, const Reduction& reduction,
                    /* see-below */ partitioner);
        template<typename Range, typename Value,
                 typename Func, typename Reduction>
        Value parallel_reduce(const Range& range, const Value& identity,
                    const Func& func, const Reduction& reduction,
                    task_group_context& context);
        template<typename Range, typename Value,
                 typename Func, typename Reduction>
        Value parallel_reduce(const Range& range, const Value& identity,
                    const Func& func, const Reduction& reduction);

        //! Class-friendly signatures
        template<typename Range, typename Body>
        void parallel_reduce(const Range& range, Body& body,
                    /* see-below */ partitioner, task_group_context& context);
        template<typename Range, typename Body>
        void parallel_reduce(const Range& range, Body& body,
                    /* see-below */ partitioner);
        template<typename Range, typename Body>
        void parallel_reduce(const Range& range, Body& body,
                             task_group_context& context);
        template<typename Range, typename Body>
        void parallel_reduce(const Range& range, Body& body);

    } // namespace tbb
} // namespace oneapi







namespace oneapi {
  namespace tbb {

    enum class filter_mode {
      parallel = /* implementation defined */,
      serial_in_order = /* implementation defined */,
      serial_out_of_order = /* implementation defined */
    };

    //! Class representing a chain of type-safe pipeline filters
    template<typename Input Type, typename OutputType>
    class filter;

    //! Create a filter to participate in parallel_pipeline
    template<typename Body> filter<filter_input<Body>, filter_output<Body>>
    make_filter( filter_mode mode, const Body& body );

    //! Composition of filters left and right.
    template<typename T, typename V, typename U> filter<T, U>
    operator&( const filter<T,V>& left, const filter<V,U>& right );

    //! Parallel pipeline over chain of filters with user-supplied context.
    inline void
    parallel_pipeline(size_t max_number_of_live_tokens,
                      const filter<void, void>& filter_chain,
                      task_group_context& context);

    //! Parallel pipeline over chain of filters.
    inline void
    parallel_pipeline(size_t max_number_of_live_tokens,
                      const filter<void, void>& filter_chain);

    //! Parallel pipeline over sequence of filters.
    template<typename F1, typename F2, typename ... FiltersContext> void
    parallel_pipeline(size_t max_number_of_live_tokens,
                      const F1& filter1,
                      const F2& filter2,
                      FiltersContext&& ... filters);

  } // namespace tbb
} // namespace oneapi












