#include <iostream>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>

#include <iostream>
#include <vector>
#include <tbb/tbb.h>

using namespace tbb;
using namespace tbb::detail::d1;

// Define a body that performs work on the range
struct MyBody2D {
    void operator()(const blocked_range2d<int>& range) const {
        for (int i = range.rows().begin(); i != range.rows().end(); ++i) {
            for (int j = range.cols().begin(); j != range.cols().end(); ++j) {
                // Example computation: print the range
                std::cout << "Processing element (" << i << ", " << j << ")\n";
            }
        }
    }
};

int main() {
    // Define the 2D range of work (100x100 grid)
    blocked_range2d<int> range(0, 100, 10, 0, 100, 10); // 10 is the grainsize for rows and columns
    MyBody2D body;                   // Define the body of work

    // Instantiate the NUMA partitioner with a simple partitioner as the base
    numa_partitioner<simple_partitioner> partitioner;

    // Use parallel_for with the NUMA partitioner
    parallel_for(range, body, partitioner);

    return 0;
}
