#include <oneapi/tbb.h>
#include <iostream>
#include <vector>
#include <numeric>
#include <atomic>
#include <chrono>

int main() {
    // Initialize a vector with some values
    size_t data_size = 1 << 20; // Example: 1 million elements
    std::vector<float> data(data_size);
    std::iota(data.begin(), data.end(), 1.0f); // Fill the vector with values 1, 2, ..., data_size

    // Atomic variable to hold the total sum
    std::atomic<float> total_sum(0.0f);

    // Lambda function to add local sums to the total sum atomically
    auto atomic_add = [](std::atomic<float>& target, float value) {
        float current = target.load();
        while (!target.compare_exchange_weak(current, current + value));
    };

    // Create a static partitioner
    tbb::static_partitioner partitioner;

     // Start the timer
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Parallel sum using tbb::parallel_for with static_partitioner
    tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size()),
        [&data, &total_sum, &atomic_add](const tbb::blocked_range<size_t>& range) {
            float local_sum = 0;
            for (size_t i = range.begin(); i != range.end(); ++i) {
                local_sum += data[i];
            }
            atomic_add(total_sum, local_sum);
        }, tbb::numa_partitioner<tbb::static_partitioner>());
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Print the total sum
    std::cout << "Total sum: " << total_sum.load() << std::endl;
    std::cout << "Execution time: " << duration << " milliseconds" << std::endl;
    return 0;
}
