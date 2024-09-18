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

void printValues(int i, int j, int k, int l) {
  std::cout << "i == " << i << std::endl
            << "j == " << j << std::endl
            << "k == " << k << std::endl
            << "l == " << l << std::endl;
}

int main(int argc, char *argv[]) {
  int i = 1, j = 10, k = 100, l = 1000;
  auto lambdaExpression = [i, &j] (int k0, int& l0) -> int {
    j = 2 * j;
    k0 = 2 * k0;
    l0 = 2 * l0;
    return i + j + k0 + l0;
  };

  printValues(i, j, k, l);
  std::cout << "First call returned " << lambdaExpression(k, l) << std::endl;
  printValues(i, j, k, l);
  std::cout << "Second call returned " << lambdaExpression(k, l) << std::endl;
  printValues(i, j, k, l);
  return 0;
}

