#include <iostream>
#include <vector>
#include <cmath>

#include <tbb/parallel_for.h>

int main(int argc, char **argv)
{
    auto values = std::vector<double>(10000);
    
    tbb::parallel_for( tbb::blocked_range<size_t>(0,values.size()),
                       [&](tbb::blocked_range<size_t> r)
    {
        for (size_t i=r.begin(); i<r.end(); ++i)
        {
            values[i] = std::sin(i * 0.001);
        }
    },tbb::numa_partitioner<tbb::static_partitioner>());
    double total = 0;

    for (double value : values)
    {
        total += value;
    }

    std::cout << total << std::endl;

    total = 0;
    tbb::parallel_for( tbb::blocked_range<size_t>(0,values.size()),
                       [&](tbb::blocked_range<size_t> r)
    {
        for (size_t i=r.begin(); i<r.end(); ++i)
        {
            values[i] = std::sin(i * 0.001);
        }
    }, tbb::static_partitioner());
    for (double value : values)
    {
        total += value;
    }

    std::cout << total << std::endl;
    return 0;
}
