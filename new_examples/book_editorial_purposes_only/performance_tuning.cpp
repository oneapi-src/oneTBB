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


namespace oneapi {
namespace tbb {
    class global_control {
    public:
        enum parameter {
            max_allowed_parallelism,
            thread_stack_size,
            terminate_on_exception
        };

        global_control(parameter p, size_t value);
        ~global_control();

        static size_t active_value(parameter param);
    };
} // namespace tbb
} // namespace oneapi



namespace oneapi {
namespace tbb {
    class task_arena {
    public:
        static const int automatic = /* unspecified */;
        static const int not_initialized = /* unspecified */;

        enum class priority : { /* See Figure 11-12 */ };
        struct constraints { /* See Figure 11-15 */ };

        task_arena(int max_concurrency = automatic, 
                   unsigned reserved_for_masters = 1,
                   priority a_priority = priority::normal);
        task_arena(constraints a_constraints, 
                   unsigned reserved_for_masters = 1,
                   priority a_priority = priority::normal);
        task_arena(const task_arena &s);
        explicit task_arena(oneapi::tbb::attach);
        ~task_arena();

        void initialize();
        void initialize(int max_concurrency,
                        unsigned reserved_for_masters = 1,
                        priority a_priority = priority::normal);
        void initialize(constraints a_constraints,
                        unsigned reserved_for_masters = 1,
                        priority a_priority = priority::normal);
        void initialize(oneapi::tbb::attach);

        void terminate();
        bool is_active() const;
        int max_concurrency() const;

        template<typename F> auto execute(F&& f) -> decltype(f());
        template<typename F> void enqueue(F&& f);

        void enqueue(task_handle&& h);
    };
} // namespace tbb
} // namespace oneapi


;;

tbb::task_areana a(4);

;;

tbb::task_areana a(4, 0);

;;;

tbb::task_arena a(tbb::attach);
tbb::task_arena b(a);


;;;;

/// duplicate????

tbb::task_arena a(tbb::attach);
tbb::task_arena b(a);


;;;;







enum class priority : /* unspecified type */ {
    low =    /* unspecified */,
    normal = /* unspecified */,
    high =   /* unspecified */
};



struct constraints {
    constraints(numa_node_id numa_node_       = task_arena::automatic,
                int          max_concurrency_ = task_arena::automatic);
    constraints& set_numa_id(numa_node_id id);
    constraints& set_max_concurrency(int maximal_concurrency);
    constraints& set_core_type(core_type_id id);
    constraints& set_max_threads_per_core(int threads_number);

    numa_node_id numa_id = task_arena::automatic;
    int max_concurrency = task_arena::automatic;
    core_type_id core_type = task_arena::automatic;
    int max_threads_per_core = task_arena::automatic;
};



;;;






namespace oneapi {
namespace tbb {
    using numa_node_id = /*implementation-defined*/;
    using core_type_id = /*implementation-defined*/;

    namespace info {
        std::vector<numa_node_id> numa_nodes();
        std::vector<core_type_id> core_types();

        int default_concurrency(task_arena::constraints c);
        int default_concurrency(numa_node_id id = 
                oneapi::tbb::task_arena::automatic);
    }
} // namespace tbb
} // namespace oneapi




 ;;;

#include "tbb/task_group.h"
#include "tbb/task_arena.h"
#include <vector>

int main() {
    std::vector<tbb::numa_node_id>numa_nodes = tbb::info::numa_nodes();
    std::vector< tbb::task_arena> arenas(numa_nodes.size());
    std::vector< tbb::task_group> task_groups(numa_nodes.size());

    for (int i = 0; i < numa_nodes.size(); i++) {
        arenas[i].initialize(tbb::task_arena::constraints(numa_nodes[i]));
    }
    for (int i = 0; i < numa_nodes.size(); i++) {
        arenas[i].execute([&task_groups, i] {
            task_groups[i].run([] {
                /* executed by the thread pinned to specified NUMA node */
            });
        });
    }
    for (int i = 0; i < numa_nodes.size(); i++) {
        arenas[i].execute([&task_groups, i] {
            task_groups[i].wait();
        });
    }
    return 0;
}




;;;;;





;;;;




// Defined in header <oneapi/tbb/task_scheduler_observer.h>

namespace oneapi {
namespace tbb {

   class task_scheduler_observer {
   public:
       task_scheduler_observer();
       explicit task_scheduler_observer( task_arena& a );
       virtual ~task_scheduler_observer();

       void observe( bool state=true );
       bool is_observing() const;

       virtual void on_scheduler_entry( bool is_worker ) {}
       virtual void on_scheduler_exit( bool is_worker } {}
   };

} // namespace tbb
} // namespace oneapi




class pinning_observer : public oneapi::tbb::task_scheduler_observer {
public:
    // HW affinity mask to be used for threads in an arena
    affinity_mask_t m_mask; 
    pinning_observer( oneapi::tbb::task_arena &a, affinity_mask_t mask )
        : oneapi::tbb::task_scheduler_observer(a), m_mask(mask) {
        observe(true); // activate the observer
    }
    void on_scheduler_entry( bool worker ) override {
        set_thread_affinity(
            oneapi::tbb::this_task_arena::current_thread_index(), m_mask);
    }
    void on_scheduler_exit( bool worker ) override {
        restore_thread_affinity();
    }
};


;;;



if (end - begin < cutoff) {
    serialQuicksort(begin, end);
}


;;;



for (int i = i_begin; i < i_end, ++i )
    for (int j = j_begin; j < j_end; ++j )
        /* loop body */



;;;;;




template< typename P >
static inline double executePfor(int num_trials, int N,
                                 int gs, P &p, double tpi) {
    tbb::tick_count t0;
    for (int t = -1; t < num_trials; ++t) {
    if (!t) t0 = tbb::tick_count::now();
        tbb::parallel_for (
        tbb::blocked_range<int>{0, N, gs},
            [tpi](const tbb::blocked_range<int> &r) {
                int e = r.end();
                for (int i = r.begin(); i < e; ++i) {
                    spinWaitForAtLeast(tpi);
                }
            },
            p
        );
    }
    tbb::tick_count t1 = tbb::tick_count::now();
    return (t1 - t0).seconds()/num_trials;
}



;;;




















// 11-27

void matrixtranspose(int N, double *a, double *b) {
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      b[j*N+i] = a[i*N+j];
    }
  }
}



;;;;

