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
