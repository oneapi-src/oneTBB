# Waiting for single messages in the Flow Graph

Extending the oneTBB Flow Graph interface with the new ``try_put_and_wait(msg)`` API which allows to wait
for completion of the chain of tasks corresponding to the ``msg``.

The feature should improve the Flow Graph performance in scenarios where multiple threads submitts the work into the
Flow Graph simultaneously and each of them should

## Introduction

Current API of oneTBB Flow Graph allows doing two basic actions after building the graph:

- Submitting messages in some nodes using the ``node.try_put(msg)`` API.
- Waiting for completion of **all** messagges in ghe graph using the ``graph.wait_for_all()`` API.

Since the only API currently available for waiting until the work would be completed waits for all tasks in the graph
to complete, there can be negative performance impact in use cases when the thread submitting the work should be notified as soon as possible
when the work is done. Having only ``graph.wait_for_all()`` forces the thread to wait until **all** of the tasks in the Flow Graph, no metter
corresponds to the waited message or not.

Consider the following example:

```cpp

struct ComputeInput;
struct ComputeOutput;

// High-level computation instance that performs some work
// using the oneTBB Flow Graph under the hood
// but the Flow Graph details are not expressed as part of public API
class ComputeTool
{
private:
    oneapi::tbb::flow::graph m_graph;
    oneapi::tbb::flow::broadcast_node<ComputeInput> m_start_node;
    // Other Flow Graph nodes
public:
    ComputeTool()
    {
        // Builds the Flow Graph
    }

    // Peforms the computation using the user-provided input
    ComputeOutput compute(ComputeInput input)
    {
        m_start_node.try_put(input); // Submit work in the graph
        m_graph.wait_for_all(); // Waiting for input to be processed
    }
};

int main()
{
    ComputeTool shared_tool;

    oneapi::tbb::parallel_for(0, 10,
        [&](int i)
        {
            // Preparing the input for index i

            ComputeOutput output = shared_tool.compute(input);

            // Post process output for index i
        });
}

```

The ``ComputeTool`` is a user interface that performs some computations on top of oneTBB Flow Graph. The function ``compute(input)`` should
perform the computations for the provided input. Since the Flow Graph is used, it should submit the message into the graph and wait for its
completion. This function also can be called concurrently from several threads as it shown in the ``main``.

Since the only available API in the Flow Graph is ``wait_for_all()``, each thread submitting the work to the graph would be required to wait
until **all** of the tasks would be done, no metter if these tasks corresponds to the input processed by this thread or not. If some
post-processing is required on each thread after receiving the computation result, it would be only possible to start it when the Flow Graph would be completed what can be inneficient if the post-processing of lightweight graph tasks would be blocked by processing the more mature input.

To get rid of this negative performance effect, it would be useful to add some kind of new API to the Flow Graph that would wait for
completion of only one message (instead of the full completion of the graph):

```cpp
ComputeOutput compute(ComputeInput input)
{
    m_start_node.try_put(input); // Submit work in the graph
    g.wait_for_one(input); // Pseudo-code. Wait for completion of only messages for computations of input
}
```

## Proposal

The idea of this proposal is to extend the existing Flow Graph API with the new member function of each receiver nodes -
``node.try_put_and_wait(msg)``. This function should submit the msg into the Flow Graph (similarly to ``try_put()``) and wait for its completion. The function should be exited only if all of the tasks corresponds to the ``msg`` and skip waiting for any other tasks to
complete.

Consider the following graph:

```cpp

using namespace oneapi::tbb;

flow::graph g;

flow::broadcast_node<int> start(g);

flow::function_node<int, int> f1(g, unlimited, f1_body);
flow::function_node<int, int> f2(g, unlimited, f2_body);
flow::function_node<int, int> f3(g, unlimited, f3_body);

flow::join_node<std::tuple<int, int, int>> join(g);

flow::function_node<int, int> pf(g, serial, pf_body);

flow::make_edge(start, f1);
flow::make_edge(start, f2);
flow::make_edge(start, f3);

flow::make_edge(f1, flow::input_port<0>(join));
flow::make_edge(f2, flow::input_port<1>(join));
flow::make_edge(f3, flow::input_port<2>(join));

flow::make_edge(join, pf);

// Parallel submittion
oneapi::tbb::parallel_for (0, 100, [](int input) {
    start.try_put(input);
});

start.try_put_and_wait(444);
// Post-processing 444

g.wait_for_all();
```

<img src="try_put_and_wait_queues.png" width=400>

Each message is broadcasted from ``start`` to three concurrent computational functions ``f1``, ``f2`` and ``f3``. The result is when joined into single tuple in ``join`` node and
post-processed in a serial ``pf`` function node. The task queue corresponding to each node in the graph is exposed under the node in the picture. The tasks that corresponds
to the parallel loop 0-100 are shown as blue tasks in the queue. Red tasks corresponds to the message submitted as an argument in ``try_put_and_wait``.
The ``try_put_and_wait`` is expected to exit when all of the red tasks and the necessary amount of blue tasks would be completed. Completion of all blue tasks as in ``wait_for_all``
is not guaranteed.

From the implementation perspective, the feature is implemented currently by creating an instance of special class ``message_metainfo`` with the input message in ``try_put_and_wait``
and then broadcast it through the graph together with the message. The actual value of message can be changed during the computation but the stored metainformation should be preserved.

When the message is buffered in one of the buffering nodes or one of the internal buffers (such as ``queueing`` ``function_node`` or ``join_node``), the corresponding metainformation
instance should be buffered as well.

For reference counting on single messages, the dedicated ``wait_context`` is assigned to each message passed to ``try_put_and_wait``. It is possible to use ``wait_context`` itself
instead of ``message_metainfo``, but it can be useful to pass something with each message through the graph, not only for single message waiting. The initial implementation of
``message_metainfo`` just wrapping the ``wait_context``, but it can be extended to cover additional use-cases. Each task corresponding to the completion of the message
associated with the awaited message holds the reference counting on the corresponding ``wait_context``. In case of buffering the message somewhere in the graph,
the additional reference counter would be held and released when the item is removed from the buffer.

From the implementation perspective, working with metainformation is exposed by adding the new internal virtual functions in the Flow Graph:

| Base Template Class | Existing Function Signature   | New Function Signature                              | Information                                      |
|---------------------|-------------------------------|-----------------------------------------------------|--------------------------------------------------|
| receiver            | bool try_put_task(const T& t) | bool try_put_task(const T& t)                       | Performs an action required by the node logic.   |
|                     |                               | bool try_put_task(const T& t,                       | May buffer both ``t`` and ``metainfo``.          |
|                     |                               |                   const message_metainfo& metainfo) | May broadcast the result and ``metainfo`` to     |
|                     |                               |                                                     | successors of the node.                          |
|                     |                               |                                                     | The first function can reuse the second with     |
|                     |                               |                                                     | the empty metainfo.                              |
|---------------------|-------------------------------|-----------------------------------------------------|--------------------------------------------------|
| sender              | bool try_get(T& t)            | bool try_get(T& t)                                  | For buffers, gets the element from the buffer.   |
|                     |                               | bool try_get(T& t, message_metainfo&)               | The second function provides both placeholders   |
|                     |                               |                                                     | for metainformation and the element.             |
|---------------------|-------------------------------|-----------------------------------------------------|--------------------------------------------------|
| sender              | bool try_reserve(T& t)        | bool try_reserve(T& t)                              | For buffers, reserves the element in the buffer. |
|                     |                               | bool try_reserve(T& t, message_metainfo& metainfo)  | The second function provides both placeholders   |
|                     |                               |                                                     | for metainformantion and the element.            |
|---------------------|-------------------------------|-----------------------------------------------------|--------------------------------------------------|

The ``message_metainfo`` class is described in details in the [separate section](#details-about-metainformation-class).
The [Nodes behavior](#nodes-behavior) section describes the behavior of each particular node when the metainformation is received.

## Nodes behavior

This chapter describes detailed behavior of each Flow Graph node when the item and the metainformation is received. Similarly to the message itself, the metainformation
can be received from the predecessor node (explicit ``try_put_task`` call) or initially from ``try_put_and_wait``.

### ``function_node<input, output, queueing>``

If the concurrency of the ``function_node`` is ``unlimited``, the node creates a task for executting the body of the node. The created task should hold the metainfo
received by the function node and broadcast it to the node successors when the task is completed.

Otherwise, similarly to the original ``function_node`` behavior, the node tries to occupy its concurrency. If the limit is not yet reached, creates a body task
similarly to the ``unlimited`` case. If the concurrency limit is reached, both input message and the associated metainformation would be stored in the internal queue, associated
with the node. When one of the other tasks, associated with the node would be completed, it will retrieve the postponed message together with the metainformation and spawn it as
a task.

Since the ``function_node`` guarantees that all of the elements would be retrieved from the internal queue at some time, [buffering issues](#buffering-the-metainfo) cannot take place.

### ``function_node<input, output, rejecting>``

For the ``unlimited`` use-case, behaves the same as ``queueing`` node.

If the concurrency limit of the node is reached, both message and the associated metainfo would be rejected and it is a predecessor responsibility to buffer them.
If the predecessor is not the buffering node, both message and the metainfo would be lost.
When another task would be completed, it will try to get a buffered message together with the metainfo (by calling the ``try_get(msg, metainfo)`` method) from the predecessing node.

Since the ``function_node`` guarantees that all of the elements would be retrieved from the internal queue at some time, [buffering issues](#buffering-the-metainfo) cannot take place
for buffering nodes, preceding the ``function_node``.

### ``function_node<input, output, lightweight>``

In regard to the concurrency limit, the lightweight function node behaves as it described in the corresponding buffering policy section (``queueing`` or ``rejecting``).
The only difference is that for such nodes the tasks would not be spawned and the associated function will be executted by the calling thread. And since we don't have tasks,
the calling thread should broadcast the metainformation to the successors after completing the function.

### ``continue_node``

The ``continue_node`` has one of the most specific semantics in regard to the metainformation. Since the node only executes the associated body (and broadcasts the signal
to the successors) if it receives ``N`` signals from it's predecessors (where ``N`` is the number of predecessors). It means that prior to executing the body,
the node can receive several metainformation instances from different predecessors.

To handle this, the ``continue_node`` initially stores an empty metainfo instance and on each ``try_put_task(continue_msg, metainfo)`` call, it [merges](#details-about-metainformation-class)
the received metainformation with the stored instance. Under the hood the merged instance will contain the ``wait_context`` pointers from its previous state and all of the pointers from
the received ``metainfo``.

When the ``continue_node`` receives ``N`` signals from the predecessors, it wraps stored metainformation into the task for completion of the associated body. Once the task is ready, the stored
metainformation instance switchs back to the empty state for further work. Once the function would be completed, the task is expected to broadcast the metainfo to the successors.

The lightweight ``continue_node`` behaves the same as described above, but without spawning any tasks. Everything would be performed by the calling thread.

### Multi-output functional nodes

``multifunction_node`` and ``async_node`` classes are not currently supported by this feature because of issues described in [the separate section](#multi-output-nodes-support).

Passing the metainformation to such a node by the predecessor would have no effect and no metainfo would be broadcasted to further successors.

### Single-push buffering nodes

This section describes the behavior for ``buffer_node``, ``queue_node``, ``priority_queue_node`` and ``sequencer_node`` classes. The only difference between them would be in
ordering of retrieving the elements from the buffer.

Once the buffering node receives a message and the metainformation, both of them should be stored into the buffer.

Since bufferring nodes are commonly used as part of the Flow Graph push-pull protocol, e.g. before the rejecting ``function_node`` or reserving ``join_node``, it means that
the waiting for the message should be prolonged once it stored in the buffer. In particular, once the metainformation is in the buffer, the buffer should call ``reserve(1)`` on each
associated ``wait_context`` to prolongue the wait and call ``release(1)`` once the element is retrieved from the buffer (while calling ``try_get`` or ``try_consume``).

Once the element and the metainfo are stored in the buffer, the node will try to push them to the successor. If one of the successors accepts the message and the metainfo,
both of them are removed from the buffer. Otherwise, the push-pull protocol works and the successor should return for the item once it becomes available by calling
``try_get(msg, metainfo)`` or ``try_reserve(msg, metainfo)``.

Since placing the buffers before rejecting nodes is not the only use-case, there is a risk of issues relates to buffering. It is described in details in the [separate section](#buffering-the-metainfo).

### Broadcast-push buffering nodes

The issue with broadcast-push ``overwrite_node`` and ``write_once_node`` is these nodes stores the received item and even if this item is accepted by one successor, it would be broadcasted to others and
kept in the buffer. Since the metainformation is kept in the buffer together with the message itself, even if the computation is completed, the ``try_put_and_wait`` would stuck because of the reference
held by the buffer.

Even the ``wait_for_all()`` call would be able to finish in this case since it counting only the tasks in progress and ``try_put_and_wait`` would still stuck.

``try_put_and_wait`` feature for the graph containing these nodes should be used carefully because of this issue:

* The ``overwrite_node`` should be explicitly resetted by calling ``node.reset()`` or the element with the stored metainfo should be overwritten with another element without metainfo.
* The ``write_once_node`` should be explicitly resetted by calling ``node.reset()`` since the item cannot be overwritten.

### ``broadcast_node``

The behavior of ``broadcast_node` is preety obvious - the metainformation would just be broadcasted to each successor of the node.

### ``limiter_node``

### ``join_node<output_tuple, queueing>``

### ``join_node<output_tuple, reserving>``

### ``join_node<output_tuple, key_matching>``

### ``split_node``

### ``indexer_node``

### ``composite_node``

## Additional implementation details

### Details about metainformation class

``message_metainfo`` class synopsis:

```cpp
class message_metainfo
{
public:
    using waiters_type = std::forward_list<d1::wait_context_vertex*>;

    message_metainfo();

    message_metainfo(const message_metainfo&);
    message_metainfo(message_metainfo&&);

    message_metainfo& operator=(const message_metainfo&);
    message_metainfo& operator=(message_metainfo&&);

    const waiters_type& waiters() const &;
    waiters_type&& waiters() &&;

    bool empty() const;

    void merge(const message_metainfo&);
};
```

The initial implementation of ``message_metainfo`` class wraps only the list of single message waiters. The class may be extended if necessary to cover additional use-cases.

The metainfo is required to hold a list of message waiters instead of single waiter to cover the ``continue_node`` and ``join_node`` joining use-cases. Consider the example:

```cpp
using namespace oneapi::tbb;

flow::function_node<int, int> start1(g, ...);
flow::function_node<int, int> start2(g, ...);

flow::join_node<std::tuple<int, int>> join(g);

flow::function_node<std::tuple<int, int>, int> post_process(g, ...);

flow::make_edge(start, flow::input_port<0>(join));
flow::make_edge(start, flow::input_port<1>(join));
flow::make_edge(join, post_process);

std::thread t1([&]() {
    start1.try_put_and_wait(1);
});

std::thread t2([&]() {
    start2.try_put_and_wait(2);
})
```



### Combined or separated wait

### Buffering the metainfo

### Multi-output nodes support

### Optimization while retrieving from buffer to task

## Process Specific Information

Open questions:
