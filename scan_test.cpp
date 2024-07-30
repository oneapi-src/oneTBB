#include <iostream>
#include <vector>
#include <tbb/parallel_scan.h>
#include <tbb/blocked_range.h>

// A body class that defines the scan operation
class PrefixSumBody {
public:
    // Constructor initializes the sum
    PrefixSumBody(std::vector<int>& input, std::vector<int>& output)
        : input(input), output(output), sum(0) {}

    // Split constructor for parallel_scan
    PrefixSumBody(PrefixSumBody& body, tbb::split)
        : input(body.input), output(body.output), sum(0) {}

    // The operator() method defines the scan operation over a range
    template <typename Range>
    void operator()(const Range& range, tbb::pre_scan_tag) {
        int temp = sum;
        for (size_t i = range.begin(); i < range.end(); ++i) {
            temp += input[i];
        }
        sum = temp;
    }

    // This version of operator() updates the output array
    template <typename Range>
    void operator()(const Range& range, tbb::final_scan_tag) {
        for (size_t i = range.begin(); i < range.end(); ++i) {
            sum += input[i];
            output[i] = sum;
        }
    }

    // Reverse operator to combine partial results
    void reverse_join(PrefixSumBody& rhs) { sum += rhs.sum; }

    // Operator to assign partial sum to current body
    void assign(PrefixSumBody& rhs) { sum = rhs.sum; }

private:
    std::vector<int>& input;
    std::vector<int>& output;
    int sum;
};

int main() {
    // Input data
    std::vector<int> input = {1, 2, 3, 4, 5};
    std::vector<int> output(input.size());
    tbb::numa_partitioner<tbb::simple_partitioner> n_part;
    // Run parallel_scan
    PrefixSumBody body(input, output);
    tbb::parallel_scan(tbb::blocked_range<size_t>(0, input.size()), body, n_part);

    // Output the result
    for (const auto& val : output) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    return 0;
}
