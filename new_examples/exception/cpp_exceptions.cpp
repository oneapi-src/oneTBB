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
using namespace std;

int main() {

  try{
    try{
      throw 5;
    }
    catch (const int& n){
      cout << "Re-throwing value: " << n << endl;
      throw;
    }
  }
  catch(int& e){
    cout << "Value caught: " << e << endl;
  }
  catch (...){
    cout << "Exception occurred\n";
  }

  return 0;
  
}
