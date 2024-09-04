/*
Copyright (C) 2024 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.

SPDX-License-Identifier: MIT
*/

#include <tbb/tbb.h>
#include <vector>
#include <iostream>

class div_ex
{
  public:
    int it;
    explicit div_ex(int it_): it{it_}{}
    const char* what() const throw(){
      return "Division by 0!";
    }
    const char* name() const throw(){
      return typeid(*this).name();
    }
};

int main(int argc, char** argv) {
  int n = 1000;

  std::vector<float> data(n, 1.0);
  data[n/2]=0.0;
  try {
    tbb::parallel_for(0, n, [&](int i) {
      if (data[i]) data[i]=1/data[i];
      else{
        throw div_ex{i};
      }
    });
  }
  catch(div_ex& ex) {
    std::cout << "Exception name: " << ex.name() << std::endl;
    std::cout << "Exception: "      << ex.what();
    std::cout << " at position: "   << ex.it << std::endl;
  }
  return 0;
}
