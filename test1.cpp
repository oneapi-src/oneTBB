#include <iostream>
#include <vector>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <iostream>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>

#include <iostream>
#include <vector>
#include <tbb/tbb.h>


using namespace tbb;
using namespace tbb::detail::d1;

// Example 1D range processing with NUMA partitioner
struct SumBody {
    const std::vector<int>& array;
    int& total_sum;

    SumBody(const std::vector<int>& arr, int& sum) : array(arr), total_sum(sum) {}

    void operator()(const blocked_range<size_t>& range) const {
        int local_sum = 0;
        for (size_t i = range.begin(); i != range.end(); ++i) {
            local_sum += array[i];
        }
        // Use a mutex or atomic operation here in a real-world scenario to update total_sum
        // For simplicity, we assume single-threaded behavior in this example
        total_sum += local_sum;
    }
};

int main() {
    // Define the size of the array and initialize it with some values
    const size_t n = 100;
    std::vector<int> array(n);
    for (size_t i = 0; i < n; ++i) {
        array[i] = i + 1; // Array contains values from 1 to 100
    }

    // Variable to store the result
    int total_sum = 0;

    // Instantiate the NUMA partitioner with a simple partitioner as the base
    numa_partitioner<simple_partitioner> partitioner;

    // Parallel computation using parallel_for with NUMA partitioner
    parallel_for(blocked_range<size_t>(0, n), SumBody(array, total_sum), partitioner);

    // Print the result
    std::cout << "Total sum: " << total_sum << std::endl;

    return 0;
}
