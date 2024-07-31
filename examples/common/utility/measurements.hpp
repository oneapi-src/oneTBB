/*
    Copyright (c) 2005-2024 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef TBB_measurements_H
#define TBB_measurements_H

#include <cmath>
#include "oneapi/tbb/tick_count.h"

namespace utility {

// utility class to aid relative error measurement of samples
class measurements {
public:
    measurements() {
        clear();
    }
    void clear() {
        _secPerFrame.clear();
    }
    void start() {
        _startTime = oneapi::tbb::tick_count::now();
    }
    void stop() {
        _endTime = oneapi::tbb::tick_count::now();
        auto count = _endTime - _startTime;
        _secPerFrame.push_back(count.seconds());
    }
    double computeRelError() {
        auto averageTimePerFrame = 
           std::accumulate(_secPerFrame.begin(), _secPerFrame.end(), 0.0) / _secPerFrame.size();
        std::vector<double> diff(_secPerFrame.size());
        std::transform(_secPerFrame.begin(),
                       _secPerFrame.end(),
                       diff.begin(),
                       [averageTimePerFrame](double x) {
                           return (x - averageTimePerFrame);
                       });
        double sumOfSquareDiff = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
        double stdDev = std::sqrt(sumOfSquareDiff / _secPerFrame.size());
        double relError = 100 * (stdDev / averageTimePerFrame);
        return relError;
    }
private:
    std::vector<double> _secPerFrame;
    oneapi::tbb::tick_count _startTime;
    oneapi::tbb::tick_count _endTime;
};

} // namespace utility

#endif /* TBB_measurements_H */
