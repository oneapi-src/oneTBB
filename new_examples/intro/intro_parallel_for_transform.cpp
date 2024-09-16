/*
    Copyright (c) 2024 Intel Corporation

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

#include <oneapi/dpl/algorithm>
#include <oneapi/dpl/execution>
#include <iostream>
#include <vector>
#include <tbb/tbb.h>
#include "intro_examples.h"

using ImagePtr = std::shared_ptr<ch01::Image>;
void writeImage(ImagePtr image_ptr);

ImagePtr applyGamma(ImagePtr image_ptr, double gamma) {
  auto output_image_ptr = 
    std::make_shared<ch01::Image>(image_ptr->name() + "_gamma", 
      ch01::IMAGE_WIDTH, ch01::IMAGE_HEIGHT);
  auto in_rows = image_ptr->rows();
  auto out_rows = output_image_ptr->rows();
  const int height = in_rows.size();
  const int width = in_rows[1] - in_rows[0];

  tbb::parallel_for( 0, height, 
    [&in_rows, &out_rows, width, gamma](int i) {
      auto in_row = in_rows[i];
      auto out_row = out_rows[i];
      std::transform(dpl::execution::unseq, in_row, in_row+width, 
        out_row, [gamma](const ch01::Image::Pixel& p) {
          double v = 0.3*p.bgra[2] + 0.59*p.bgra[1] + 0.11*p.bgra[0];
          double res = pow(v, gamma);
          if(res > ch01::MAX_BGR_VALUE) res = ch01::MAX_BGR_VALUE;
          return ch01::Image::Pixel(res, res, res);
      });
    }
  );
  return output_image_ptr;
}

ImagePtr applyTint(ImagePtr image_ptr, const double *tints) {
  auto output_image_ptr = 
    std::make_shared<ch01::Image>(image_ptr->name() + "_tinted", 
      ch01::IMAGE_WIDTH, ch01::IMAGE_HEIGHT);
  auto in_rows = image_ptr->rows();
  auto out_rows = output_image_ptr->rows();
  const int height = in_rows.size();
  const int width = in_rows[1] - in_rows[0];

  tbb::parallel_for( 0, height, 
    [&in_rows, &out_rows, width, tints](int i) {
      auto in_row = in_rows[i];
      auto out_row = out_rows[i];
      std::transform(dpl::execution::unseq, in_row, in_row+width, 
        out_row, [tints](const ch01::Image::Pixel& p) {
          std::uint8_t b = (double)p.bgra[0] + 
                           (ch01::MAX_BGR_VALUE-p.bgra[0])*tints[0];
          std::uint8_t g = (double)p.bgra[1] + 
                           (ch01::MAX_BGR_VALUE-p.bgra[1])*tints[1];
          std::uint8_t r = (double)p.bgra[2] + 
                           (ch01::MAX_BGR_VALUE-p.bgra[2])*tints[2];
          return ch01::Image::Pixel(
            (b > ch01::MAX_BGR_VALUE) ? ch01::MAX_BGR_VALUE : b,
            (g > ch01::MAX_BGR_VALUE) ? ch01::MAX_BGR_VALUE : g,
            (r > ch01::MAX_BGR_VALUE) ? ch01::MAX_BGR_VALUE : r
          );
      });
    }
  );
  return output_image_ptr;
}

void fig_1_12(std::vector<ImagePtr>& image_vector) {
  const double tint_array[] = {0.75, 0, 0};

  tbb::flow::graph g;

  int i = 0;
  tbb::flow::input_node<ImagePtr> src( g, [&]( tbb::flow_control &fc ) -> ImagePtr 
  {   
      if ( i < image_vector.size() ) 
      {
          return image_vector[i++];
      } 
      else
      {
          fc.stop();
          return nullptr;
      }        
  });

  tbb::flow::function_node<ImagePtr, ImagePtr> gamma(g, 
    tbb::flow::unlimited,
    [] (ImagePtr img) -> ImagePtr {
      return applyGamma(img, 1.4);  
    }
  );

  tbb::flow::function_node<ImagePtr, ImagePtr> tint(g, 
    tbb::flow::unlimited,
    [tint_array] (ImagePtr img) -> ImagePtr {
      return applyTint(img, tint_array);
    }
  );

  tbb::flow::function_node<ImagePtr> write(g, 
    tbb::flow::unlimited,
    [] (ImagePtr img) {
      writeImage(img);
    }
  );

  tbb::flow::make_edge(src, gamma);
  tbb::flow::make_edge(gamma, tint);
  tbb::flow::make_edge(tint, write);
  src.activate();
  g.wait_for_all();
} 

void writeImage(ImagePtr image_ptr) {
  image_ptr->write( (image_ptr->name() + ".bmp").c_str());
}

int main(int argc, char* argv[]) {
  std::vector<ImagePtr> image_vector;

  for ( int i = 2000; i < 20000000; i *= 10 ) 
    image_vector.push_back(ch01::makeFractalImage(i));

  // warmup the scheduler
  tbb::parallel_for(0, tbb::this_task_arena::max_concurrency(), 
    [](int) {
      tbb::tick_count t0 = tbb::tick_count::now();
      while ((tbb::tick_count::now() - t0).seconds() < 0.01);
    }
  );

  tbb::tick_count t0 = tbb::tick_count::now();
  fig_1_12(image_vector);
  std::cout << "Time : " << (tbb::tick_count::now()-t0).seconds() 
            << " seconds" << std::endl;
  return 0;
}

