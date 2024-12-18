#define TBB_PREVIEW_PARALLEL_PHASE 1

#include "common/test.h"
#include "common/utils.h"
#include "common/spin_barrier.h"

#include "tbb/task_arena.h"
#include "tbb/parallel_for.h"

struct dummy_func {
    void operator()() const {
    }
};

template <typename F1 = dummy_func, typename F2 = dummy_func>
std::size_t measure_median_start_time(tbb::task_arena* ta, const F1& start = F1{}, const F2& end = F2{}) {
    std::size_t num_threads = ta ? ta->max_concurrency() : tbb::this_task_arena::max_concurrency();
    std::size_t num_runs = 1000;
    std::vector<std::size_t> longest_start_times;
    longest_start_times.reserve(num_runs);

    std::vector<std::chrono::steady_clock::time_point> start_times(num_threads);
    utils::SpinBarrier barrier(num_threads);
    auto measure_start_time = [&] (std::size_t) {
        start_times[tbb::this_task_arena::current_thread_index()] = std::chrono::steady_clock::now();
        barrier.wait();
    };

    auto get_longest_start = [&] (std::chrono::steady_clock::time_point start_time) {
        std::size_t longest_time = 0;
        for (auto& time : start_times) {
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(time - start_time);
            longest_time = std::max(longest_time, std::size_t(diff.count()));
        }
        return longest_time;
    };

    auto work = [&] {
        auto start_time = std::chrono::steady_clock::now();
        start();
        tbb::parallel_for(std::size_t(0), num_threads, measure_start_time, tbb::static_partitioner{});
        end();
        longest_start_times.push_back(get_longest_start(start_time));
    };

    for (std::size_t i = 0; i < num_runs; ++i) {
        if (ta) {
            ta->execute(work);
        } else {
            work();
        }
        utils::doDummyWork(i*500);
    }
    return utils::median(longest_start_times.begin(), longest_start_times.end());
}

template <typename Impl>
class start_time_collection_base {
    friend Impl;
public:
    start_time_collection_base(tbb::task_arena& ta, std::size_t ntrials) :
        arena(&ta), num_trials(ntrials), start_times(ntrials) {}

    explicit start_time_collection_base(std::size_t ntrials) :
        arena(nullptr), num_trials(ntrials), start_times(ntrials) {}

    std::vector<std::size_t> measure() {
        for (std::size_t i = 0; i < num_trials; ++i) {
            std::size_t median_start_time = static_cast<Impl*>(this)->measure_impl();
            start_times[i] = median_start_time;
        }
        return start_times;
    }
protected:
    tbb::task_arena* arena;
    std::size_t num_trials;
    std::vector<std::size_t> start_times;
};

class start_time_collection : public start_time_collection_base<start_time_collection> {
    using base = start_time_collection_base<start_time_collection>;
    using base::base;
    friend base;

    std::size_t measure_impl() {
        return measure_median_start_time(arena);
    };
};

class start_time_collection_phase_wrapped
    : public start_time_collection_base<start_time_collection_phase_wrapped>
{
    using base = start_time_collection_base<start_time_collection_phase_wrapped>;
    using base::base;
    friend base;

    std::size_t measure_impl() {
        arena->start_parallel_phase();
        auto median_start_time = measure_median_start_time(arena);
        arena->end_parallel_phase(/*with_fast_leave*/true);
        return median_start_time;
    };
};

class start_time_collection_scoped_phase_wrapped
    : public start_time_collection_base<start_time_collection_scoped_phase_wrapped>
{
    using base = start_time_collection_base<start_time_collection_scoped_phase_wrapped>;
    using base::base;
    friend base;

    std::size_t measure_impl() {
        tbb::task_arena::scoped_parallel_phase phase{*arena};
        auto median_start_time = measure_median_start_time(arena);
        return median_start_time;
    };
};

class start_time_collection_sequenced_phases
    : public start_time_collection_base<start_time_collection_sequenced_phases>
{
    using base = start_time_collection_base<start_time_collection_sequenced_phases>;
    friend base;

    bool with_fast_leave;

    std::size_t measure_impl() {
        std::size_t median_start_time;
        if (arena) {
            median_start_time = measure_median_start_time(arena,
                [this] { arena->start_parallel_phase(); },
                [this] { arena->end_parallel_phase(with_fast_leave); });
        } else {
            median_start_time = measure_median_start_time(arena,
                [] { tbb::this_task_arena::start_parallel_phase(); },
                [this] { tbb::this_task_arena::end_parallel_phase(with_fast_leave); });
        }
        return median_start_time;
    };

public:
    start_time_collection_sequenced_phases(tbb::task_arena& ta, std::size_t ntrials, bool fast_leave = false) :
        base(ta, ntrials), with_fast_leave(fast_leave)
    {}

    explicit start_time_collection_sequenced_phases(std::size_t ntrials, bool fast_leave = false) :
        base(ntrials), with_fast_leave(fast_leave)
    {}
};

class start_time_collection_sequenced_scoped_phases
    : public start_time_collection_base<start_time_collection_sequenced_scoped_phases>
{
  using base = start_time_collection_base<start_time_collection_sequenced_scoped_phases>;
  friend base;

  bool with_fast_leave;

  std::size_t measure_impl() {
      tbb::task_arena::scoped_parallel_phase* phase = nullptr;
      auto median_start_time = measure_median_start_time(arena,
          [this, &phase] {
              phase = new tbb::task_arena::scoped_parallel_phase{*arena, with_fast_leave};
          },
          [&phase] {
              delete phase;
      });
    return median_start_time;
  };

public:
  start_time_collection_sequenced_scoped_phases(tbb::task_arena& ta, std::size_t ntrials, bool fast_leave = false) :
      base(ta, ntrials), with_fast_leave(fast_leave)
  {}

  explicit start_time_collection_sequenced_scoped_phases(std::size_t ntrials, bool fast_leave = false) :
      base(ntrials), with_fast_leave(fast_leave)
  {}
};

//! \brief \ref interface \ref requirement
TEST_CASE("Check that workers leave faster with leave_policy::fast") {
    tbb::task_arena ta_automatic_leave {
        tbb::task_arena::automatic, 1,
        tbb::task_arena::priority::normal,
        tbb::task_arena::leave_policy::automatic
    };
    tbb::task_arena ta_fast_leave { 
        tbb::task_arena::automatic, 1,
        tbb::task_arena::priority::normal,
        tbb::task_arena::leave_policy::fast
    };
    start_time_collection st_collector1{ta_automatic_leave, /*num_trials=*/10};
    start_time_collection st_collector2{ta_fast_leave, /*num_trials=*/10};

    auto times_automatic = st_collector1.measure();
    auto times_fast = st_collector2.measure();

    auto median_automatic = utils::median(times_automatic.begin(), times_automatic.end());
    auto median_fast = utils::median(times_fast.begin(), times_fast.end());

    WARN_MESSAGE(median_automatic < median_fast,
        "Expected workers to start new work faster with delayed leave");
}

//! \brief \ref interface \ref requirement
TEST_CASE("Parallel Phase retains workers in task_arena") {
    tbb::task_arena ta_fast1 {
        tbb::task_arena::automatic, 1,
        tbb::task_arena::priority::normal,
        tbb::task_arena::leave_policy::fast
    };
    tbb::task_arena ta_fast2 { 
        tbb::task_arena::automatic, 1,
        tbb::task_arena::priority::normal,
        tbb::task_arena::leave_policy::fast
    };
    start_time_collection_phase_wrapped st_collector1{ta_fast1, /*num_trials=*/10};
    start_time_collection_scoped_phase_wrapped st_collector_scoped{ta_fast1, /*num_trials=*/10};
    start_time_collection st_collector2{ta_fast2, /*num_trials=*/10};

    auto times1 = st_collector1.measure();
    auto times2 = st_collector2.measure();
    auto times_scoped = st_collector_scoped.measure();

    auto median1 = utils::median(times1.begin(), times1.end());
    auto median2 = utils::median(times2.begin(), times2.end());
    auto median_scoped = utils::median(times_scoped.begin(), times_scoped.end());

    WARN_MESSAGE(median1 < median2,
        "Expected workers start new work faster when using parallel_phase");

    WARN_MESSAGE(median_scoped < median2,
        "Expected workers start new work faster when using scoped parallel_phase");
}

//! \brief \ref interface \ref requirement
TEST_CASE("Test one-time fast leave") {
    tbb::task_arena ta1{};
    tbb::task_arena ta2{};
    start_time_collection_sequenced_phases st_collector1{ta1, /*num_trials=*/10};
    start_time_collection_sequenced_phases st_collector2{ta2, /*num_trials=*/10, /*fast_leave*/true};
    start_time_collection_sequenced_scoped_phases st_collector_scoped{ta2, /*num_trials=*/10, /*fast_leave*/true};

    auto times1 = st_collector1.measure();
    auto times2 = st_collector2.measure();
    auto times_scoped = st_collector_scoped.measure();

    auto median1 = utils::median(times1.begin(), times1.end());
    auto median2 = utils::median(times2.begin(), times2.end());
    auto median_scoped = utils::median(times_scoped.begin(), times_scoped.end());

    WARN_MESSAGE(median1 < median2,
        "Expected one-time fast leave setting to slow workers to start new work");

    WARN_MESSAGE(median1 < median_scoped,
        "Expected one-time fast leave setting to slow workers to start new work");
}

//! \brief \ref interface \ref requirement
TEST_CASE("Test parallel phase with this_task_arena") {
    start_time_collection_sequenced_phases st_collector1{/*num_trials=*/10};
    start_time_collection_sequenced_phases st_collector2{/*num_trials=*/10, /*fast_leave*/true};

    auto times1 = st_collector1.measure();
    auto times2 = st_collector2.measure();

    auto median1 = utils::median(times1.begin(), times1.end());
    auto median2 = utils::median(times2.begin(), times2.end());

    WARN_MESSAGE(median1 < median2,
        "Expected one-time fast leave setting to slow workers to start new work");
}
