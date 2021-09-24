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

#include "oneapi/tbb/task_arena.h"
#include "oneapi/tbb/task_group.h"
#include "oneapi/tbb/spin_mutex.h"

#include <cstddef>
#include <vector>
#include <thread>

std::atomic<std::size_t> global_accumullator{0};

struct my_task {
    static constexpr std::size_t array_size {500000};
    mutable std::array<std::size_t, array_size> data;
    int level{0};
    tbb::task_group& tg;
    my_task(tbb::task_group& _tg) : tg(_tg){};

    void operator()() const {
        double accumulation = 1.;

        for(std::size_t i = 0; i < array_size; ++i){
            data[i] = i;
        }
        for (auto& el: data) {
            accumulation += sin(el);
        }

        if (level < 3) {
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
}

//! Testing simple partitioner stability
//! \brief \ref error_guessing
TEST_CASE("Simple partitioner stability hier") {
    TestSimplePartitionerStability();
}

