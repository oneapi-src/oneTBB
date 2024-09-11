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



## Combined or separated

## Process Specific Information
