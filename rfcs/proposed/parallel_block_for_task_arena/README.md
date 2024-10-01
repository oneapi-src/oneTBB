# Adding API for parallel block to task_arena to warm-up/retain/release worker threads

## Introduction

In oneTBB, there has never been an API that allows users to block worker threads within the arena.
This design choice was made to preserve the composability of the application.<br>
Since oneTBB is a dynamic runtime based on task stealing, threads will migrate from one arena to
another while they have tasks to execute.<br>
Before PR#1352, workers moved to the thread pool to sleep once there were no arenas with active
demand. However, PR#1352 introduced a busy-wait block time that blocks a thread for an
`implementation-defined` duration if there is no active demand in arenas. This change significantly
improved performance in cases where the application is run on high thread count systems.<br>
The main idea is that usually, after one parallel computation ends,
another will start after some time. The default block time is a heuristic to utilize this,
covering most cases within its duration.

The default behavior of oneTBB with these changes does not affect performance when oneTBB is used
as the single parallel runtime.<br>
However, some cases where several runtimes are used together might be affected. For example, if an
application builds a pipeline where oneTBB is used for one stage and OpenMP is used for a
subsequent stage, there is a chance that oneTBB workers will interfere with OpenMP threads.
This interference might result in slight oversubscription,
which in turn might lead to underperformance.

This problem can be resolved with an API that indicates when parallel computation is done,
allowing worker threads to be released from the arena,
essentially overriding the default block-time.<br>

This problem can be considered from another angle. Essentially, if the user can indicate where
parallel computation ends, they can also indicate where they start.

<img src="parallel_block_introduction.png" width=800>

With this approach, the user not only releases threads when necessary
but also specifies a programmable block where worker threads should stick to the
executing arena.

## Proposal

Let's consider the guarantees that an API for explicit parallel blocks can provides:
* Start of parallel block:
  * Indicates the point from which the scheduler can use a hint and stick threads to the arena.
  * Serve as a warm-up hint to the scheduler, making some worker threads immediately available
    at the start of the real computatin.
* "Parallel block" itself:
  * Scheduler can implement different busy-wait policies to retain threads in the arena.
* End of parallel block:
  * Indicates the point from which the scheduler can drop a hint
    and unstick threads from the arena.
  * Indicates that worker threads should ignore
    the default block time (introduced by PR#1352) and leave.

Start of parallel block:<br>
The warm-up hint should have similar guarantees as `task_arena::enqueue` from a signal standpoint.
Users should expect the scheduler will do its best to make some threads available in the arena.

"Parallel block" itself:<br>
The guarantee for retaining threads is a hint to the scheduler;
thus, no real guarantee is provided. The scheduler can ignore the hint and
move threads to another arena or to sleep if conditions are met.

End of parallel block:<br>
It can indicate that worker threads should ignore the default block time but
if work was submitted immediately after the end of the parallel block,
the default block time will be restored.

But what if user would like to disable default block time entirely?<br>
Because the heuristic of extended block time is unsuitable for the task submitted
in unpredictable pattern and duration. In this case, there should be an API to disable
the default block time in the arena entirely.

```cpp
class task_arena {
    void indicate_start_of_parallel_block(bool do_warmup = false);
    void indicate_end_of_parallel_block(bool disable_default_block_time = false);
    void disable_default_block_time();
    void enable_default_block_time();
};

namespace this_task_arena {
    void indicate_start_of_parallel_block(bool do_warmup = false);
    void indicate_end_of_parallel_block(bool disable_default_block_time = false);
    void disable_default_block_time();
    void enable_default_block_time();
}
```

If the end of the parallel block is not indicated by the user, it will be done automatically when
the last public reference is removed from the arena (i.e., task_arena is destroyed or a thread
is joined for an implicit arena). This ensures correctness is
preserved (threads will not stick forever).

## Considerations

The retaining of worker threads should be implemented with care because
it might introduce performance problems if:
* Threads cannot migrate to another arena because they
  stick to the current arena.
* Compute resources are not homogeneous, e.g., the CPU is hybrid.
  Heavier involvement of less performant core types might result in artificial work
  imbalance in the arena.


## Open Questions in Design

Some open questions that remain:
* Are the suggested APIs sufficient?
* Are there additional use cases that should be considered that we missed in our analysis?
