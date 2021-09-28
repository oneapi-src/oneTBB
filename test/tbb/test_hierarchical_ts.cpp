#include "common/test.h"

#include "tbb/parallel_for.h"

#include "common/config.h"
#include "common/utils.h"
#include "common/utils_concurrency_limit.h"
#include "common/utils_report.h"
#include "common/vector_types.h"
#include "common/cpu_usertime.h"
#include "common/spin_barrier.h"
#include "common/exception_handling.h"
#include "common/concepts_common.h"
#include "test_partitioner.h"
#include "common/test.h"
#include "common/spin_barrier.h"
#include "common/utils.h"
#include "common/memory_usage.h"
#include "common/utils_concurrency_limit.h"
#include "oneapi/tbb/mutex.h"

#include "oneapi/tbb/task_arena.h"
#include "oneapi/tbb/task_group.h"
#include "oneapi/tbb/spin_mutex.h"
// #include "../../src/tbb/governor.cpp"

#include <cstddef>
#include <vector>
#include <thread>
#include <x86intrin.h>

std::atomic<std::size_t> global_accumullator{0};

struct my_task {
    tbb::task_group& tg;

    static constexpr std::size_t array_size {10000};
    mutable std::array<std::size_t, array_size> data;
    mutable std::list<std::size_t> data_list;
    int level{0};
 
    my_task(tbb::task_group& _tg) : tg(_tg), data_list(array_size){};

    void operator()() const {
        double accumulation = 1.;
        auto iter = data_list.begin();
        for(std::size_t i = 0; i < array_size; ++i){
            data[i] = i;
            *iter = data[i];
        }

        iter = data_list.begin();
        for (auto& el: data) {
            accumulation += sin(el) + sin(*iter);
            ++iter;
        }

        if (level < 5) {
            my_task new_task(tg);
            new_task.level = this->level + 1;
            for(int i = 0; i < 10; ++i){
                tg.run(new_task);
            }
        }
        global_accumullator += std::size_t(sin(accumulation));
    };
};


void TestSimplePartitionerStability(){
    tbb::task_group tg;
    auto t1 = tbb::tick_count::now();
    for(int i = 0; i < 10; ++i){
        tg.run(my_task(tg));
    }

    tg.wait();
    std::cout << "Elapsed time: " << (tbb::tick_count::now() - t1).seconds() << std::endl;
    static constexpr std::size_t array_size {500000};
    std::array<std::size_t, array_size> data;
    for(std::size_t i = 0; i < array_size; ++i){
            data[i] = i;
    }
}

// void TestNumaAccess(){
//     static constexpr std::size_t array_size {500000};
//     std::array<std::size_t, array_size> data;
//     for(std::size_t i = 0; i < array_size; ++i){
//             data[i] = i;
//     }

//     int this_numa_id = tbb::detail::r1::governor::get_my_current_numa_node();

//     std::vector<std::thread> threads;
//     for(int i = 0; i < 20; ++i){
//         threads.emplace_back([&, i](){
//             if(tbb::detail::r1::governor::get_my_current_numa_node() != this_numa_id){
//                 u_int64_t tick;
//                 tick = __rdtsc();
//                 for(int j = i * 1000; j < (i+1) * 1000; ++j){
//                     data[i] += j / 2;
//                 }
//                 std::cout << "Other NUMA %I64d ticks "<<__rdtsc() - tick << std::endl;
//             }else{
//                 u_int64_t tick;
//                 tick = __rdtsc();
//                 for(int j = i * 1000; j < (i+1) * 1000; ++j){
//                     data[i] += j / 2;
//                 }
//                 std::cout << "My NUMA %I64d ticks "<<__rdtsc() - tick << std::endl;
//             }
//         });
//     }

//     for(int i = 0; i < 20; ++i){
//         threads[i].join();
//     }
// }

//! Testing simple partitioner stability
//! \brief \ref error_guessing
TEST_CASE("Simple partitioner stability hier") {
    TestSimplePartitionerStability();
}
