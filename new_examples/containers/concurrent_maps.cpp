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
#include <vector>
#include <tbb/concurrent_map.h>
#include <tbb/concurrent_unordered_map.h>

using namespace std;

int main()
{
  vector<string> names {"alpha", "beta", "gamma", "delta", "epsilon",
                        "zeta", "eta", "theta", "iota", "kappa",
                        "lambda", "mu", "nu", "xi", "omicron",
                        "pi", "rho", "sigma", "tau", "upsilon",
                        "phi", "chi", "psi", "omega"};

  tbb::concurrent_map<string,int> greekOrdered;
  tbb::concurrent_unordered_map<string,int> greekToMe;

  for(int i=0; i<names.size(); i++) {
    greekOrdered[names[i]] = i;
    greekToMe[names[i]] = i;
  }

  for(auto i=greekOrdered.begin(); i!=greekOrdered.end(); i++) {
    cout << i->first << "(" << i->second << ")  ";
  }
  cout << "\n\n";
  for(auto i=greekToMe.begin(); i!=greekToMe.end(); i++) {
    cout << i->first << "(" << i->second << ")  ";
  }
  cout << "\n";

  return 0;
}
