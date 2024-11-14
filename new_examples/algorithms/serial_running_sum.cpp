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

//
// The part that appears as an example in the book
//

#include <iostream>
#include <vector>

int serialImpl(const std::vector<int> &v, std::vector<int> &rsum) {
  int N = v.size();
  rsum[0] = v[0];
  for (int i = 1; i < N; ++i) {
    rsum[i] = rsum[i-1] + v[i];
  }
  int final_sum = rsum[N-1];
  return final_sum;
}

int main(int argc, char *argv[]) {
  const int N = 100;
  std::vector<int> numbers;
  std::vector<int> sums;

  for (int i = 1; i <= N; ++i) {
        numbers.push_back(i);
        sums.push_back(i);
  }

  std::cout << "Size " << numbers.size() << std::endl;

  int sum = serialImpl(numbers,sums);

  std::cout << "The sum was reported as " << sum << std::endl;

  //  std::cout << "numbers: ";
  //  for (int num : numbers) {
  //    std::cout << num << " ";
  //  }
  //  std::cout << std::endl;
  //  std::cout << "sums: ";
  //  for (int asum : sums) {
  //    std::cout << asum << " ";
  //  }
  //  std::cout << std::endl;

}
