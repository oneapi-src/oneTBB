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

#include <iostream>

class Functor {
  int my_i;
  int& my_jRef;

public:
  Functor(int i, int& j) : my_i{i}, my_jRef{j} { }

  int operator()(int k0, int& l0) {
    my_jRef = 2 * my_jRef;
    k0 = 2 * k0;
    l0 = 2 * l0;
    return my_i + my_jRef + k0 + l0;
  }
};

void PrintValues(int i, int j, int k, int l) {
  std::cout << "i == " << i << std::endl
            << "j == " << j << std::endl
            << "k == " << k << std::endl
            << "l == " << l << std::endl;
}

int main(int argc, char *argv[]) {
  int i = 1, j = 10, k = 100, l = 1000;
  Functor f{i,j};

  PrintValues(i, j, k, l);
  std::cout << "First call returned " << f(k, l) << std::endl;
  PrintValues(i, j, k, l);
  std::cout << "Second call returned " << f(k, l) << std::endl;
  PrintValues(i, j, k, l);
  return 0;
}

