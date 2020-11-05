//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <CL/sycl.hpp>
// For random number generation in device code:
#include <oneapi/dpl/random>
// Utility header for output Image
#include "utils.hpp"

using namespace sycl;

const int iter = 10000;
const int img_dimensions = 512;

const int size_wg = 32;
const int num_wg = (int)ceil((double)iter / (double)size_wg);

// Return index of pixel corresponding to a set of simulation coordinates:
int GetPixelIndex(double x, double y) {
    const int radius = img_dimensions / 2;
    int img_x = x * radius + radius;
    int img_y = y * radius + radius;
    return img_y * img_dimensions + img_x;
}

int main() {
    Img<ImgFormat::BMP> image{img_dimensions, img_dimensions};
    int sum = 0;
    
    queue q(default_selector{});
    
    std::uint32_t seed = 667;
    {
        buffer sum_buf(&sum, range(1));
        buffer<ImgPixel,1> imgplot_buf(image.data(), image.width() * image.height());

        queue q;
        // Compute a dot-product by reducing all computed values using standard plus functor
        q.submit([&](handler& cgh) {
            accessor img_plot(imgplot_buf, cgh, read_write);
            accessor sum(sum_buf, cgh, read_write);
            cgh.parallel_for<class dot_product>(nd_range<1>{num_wg * size_wg, size_wg}, sycl::ONEAPI::reduction(sum, 0, std::plus<int>()), [=](nd_item<1> it, auto& sum) {
                int i = it.get_global_id(0);
                
                if (i < iter) {
                    // Get random coords
                    oneapi::std::minstd_rand engine(seed, i * 2);
                    oneapi::std::uniform_real_distribution<double> distr(-1.0,1.0);
                    double x = distr(engine);
                    double y = distr(engine);
                    
                    auto hypotenuse_sqr = x * x + y * y;
                    if (hypotenuse_sqr <= 1.0) {
                        sum += 1;
                        img_plot[GetPixelIndex(x,y)].set(255,255,255,255);
                    }
                }
            });
        }).wait();   
    }
    std::cout << "DONE. \nPi: " << 4.0 * (double)sum / iter << "\n";
    image.write("img/pi.png");
    return 0;
}
