#include <iostream>
#include <vector>
#include <numeric> // For std::accumulate
#include <tbb/tbb.h>

class SumReducer {
public:
    SumReducer(const std::vector<int>& data) : data(data), sum(0) {}

    // Splitting constructor
    SumReducer(SumReducer& s, tbb::split) : data(s.data), sum(0) {}

    // The function that processes a range
    void operator()(const tbb::blocked_range<size_t>& range) {
        int local_sum = 0;
        for(size_t i = range.begin(); i != range.end(); ++i) {
            local_sum += data[i];
        }
        sum += local_sum; // Accumulate local sum to the overall sum
    }

    // Join operation to combine results from different threads
    void join(const SumReducer& other) {
        sum += other.sum;
    }

    // Accessor for the result
    int get_sum() const {
        return sum;
    }

private:
    const std::vector<int>& data;
    int sum;
};

int main() {
    // Create a large vector of integers
    std::vector<int> data(1000000);
    std::iota(data.begin(), data.end(), 1); // Fill with 1, 2, 3, ..., 1000000
    tbb::numa_partitioner<tbb::static_partitioner> s;
    // Perform parallel reduction
    SumReducer reducer(data);
    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, data.size()), reducer, s);

    // Output the result
    std::cout << "Sum: " << reducer.get_sum() << std::endl;

    return 0;
}
