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

#include <list>
#include <utility>
#include <tbb/tbb.h>

using PrimesValue = std::pair<int, bool>;
using PrimesList = std::list<PrimesValue>;
bool isPrime(int n);

struct PrimesTreeElement {
  using Ptr = std::shared_ptr<PrimesTreeElement>; 

  PrimesValue v;
  Ptr left;
  Ptr right;
  PrimesTreeElement(const PrimesValue& _v) : left{}, right{} {
    v.first = _v.first;
    v.second = _v.second;
  }
};


//
// Simple serial implementation that checks each element in the list
// and assigns true if it is a prime number
//
void serialPrimesList(PrimesList& values) {
  for (PrimesList::reference v : values) {
    if (isPrime(v.first)) 
      v.second = true;
  }
}


//
// TBB implementation that checks each element in the list
// using a parallel_for_each and assigns true if it is a prime number
//
void parallelPrimesList(PrimesList& values) {
  tbb::parallel_for_each(values,
    [](PrimesList::reference v) {
      if (isPrime(v.first)) 
        v.second = true;
    }
  );
}

//
// Simple serial implementation that traverses each element in a tree
// and assigns true if it is a prime number
//
void serialPrimesTree(PrimesTreeElement::Ptr e) {
  if (e) {
    if ( isPrime(e->v.first) )
      e->v.second = true;
    if (e->left) serialPrimesTree(e->left);
    if (e->right) serialPrimesTree(e->right);
  }
}

//
// TBB implementation that uses a parallel_for_each to traverse the
// the tree. Assigns true if it is a prime number and feeds left
// and right children back to parallel_for_each.
//
void parallelPrimesTree(PrimesTreeElement::Ptr root) {
  PrimesTreeElement::Ptr tree_array[] = {root};
  tbb::parallel_for_each(tree_array,
    [](PrimesTreeElement::Ptr e, 
       tbb::feeder<PrimesTreeElement::Ptr>& f) {
        if (e) {
          if (e->left) f.add(e->left);
          if (e->right) f.add(e->right);
          if (isPrime(e->v.first))
            e->v.second = true;
        }
      } 
  );
}

#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <vector>

using PrimesMap = std::map<int, bool>;
using IntVector = std::vector<int>;

static IntVector makePrimesValues(int n, PrimesMap& m);
static PrimesList makePrimesList(const IntVector& vec);
static PrimesTreeElement::Ptr makePrimesTree(int level, IntVector& vec);
static bool validatePrimesTree(PrimesTreeElement::Ptr e, PrimesMap& m);
static void warmupTBB();

int main() {
  const int levels = 14;
  const int N = std::pow(2, levels) - 1;
  PrimesMap m;
  auto vec = makePrimesValues(N, m);
  PrimesList slist = makePrimesList(vec);
  PrimesList plist = makePrimesList(vec);
  auto sroot = makePrimesTree(levels, vec);
  auto proot = makePrimesTree(levels, vec);
  
  double serial_list_time = 0.0, tbb_list_time = 0.0;
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    serialPrimesList(slist);
    serial_list_time = (tbb::tick_count::now() - t0).seconds();
  }
  for (auto p : slist) {
    if (p.second != m[p.first])
      std::cerr << "Error: serial list results are incorrect!" << std::endl;
  }
  
  warmupTBB();
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    parallelPrimesList(plist);
    tbb_list_time = (tbb::tick_count::now() - t0).seconds();
  }
  for (auto p : plist) {
    if (p.second != m[p.first])
      std::cerr << "Error: tbb list results are incorrect!" << std::endl;
  }
  
  if (slist != plist) {
    std::cerr << "Error: serial and tbb implementations do not agree!" << std::endl;
  }

  double serial_tree_time = 0.0, tbb_tree_time = 0.0;
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    serialPrimesTree(sroot);
    serial_tree_time = (tbb::tick_count::now() - t0).seconds();
  }

  if (!validatePrimesTree(sroot, m)) {
      std::cerr << "Error: serial tree results are incorrect!" << std::endl;
  }
  
  warmupTBB();
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    parallelPrimesTree(proot);
    tbb_tree_time = (tbb::tick_count::now() - t0).seconds();
  }

  if (!validatePrimesTree(proot, m)) {
      std::cerr << "Error: tbb tree results are incorrect!" << std::endl;
  }
  
  std::cout << "serial_list_time == " << serial_list_time << " seconds" << std::endl
            << "tbb_list_time == " << tbb_list_time << " seconds" << std::endl
            << "speedup for list == " << serial_list_time/tbb_list_time << std::endl
            << "serial_tree_time == " << serial_tree_time << " seconds" << std::endl
            << "tbb_tree_time == " << tbb_tree_time << " seconds" << std::endl
            << "speedup for tree == " << serial_tree_time/tbb_tree_time << std::endl;

  return 0;
}


bool isPrime(int n) {
  int e =  std::sqrt(n);
  std::vector<bool> p(e+1, true);

  for (int i = 2; i <= e; ++i) {
    if (p[i]) {
      if (n % i) {
        for (int j = 2*i; j <= e; j += i) {
          p[j] = false;
        }
      } else {
        return false;
      }
    }
  }
  return true;
}

static IntVector makePrimesValues(int n, PrimesMap& m) {
  std::default_random_engine gen;
  std::uniform_int_distribution<int> dist;
  IntVector vec;

  for (int i = 0; i < n; ++i) {
    int v = dist(gen);
    vec.push_back( v );
    m[v] = isPrime(v);
  }
  return vec;
}

static PrimesList makePrimesList(const IntVector& vec) {
  PrimesList l;
  for (auto& v : vec) {
    l.push_back(PrimesValue(v, false));
  }
  return l;
}

static PrimesTreeElement::Ptr makePrimesTreeElem(int level, const IntVector& vec, 
                                                 IntVector::const_iterator i) {
  if (level && i != vec.cend()) {
    PrimesTreeElement::Ptr e = std::make_shared<PrimesTreeElement>(PrimesValue(*i, false));
    if (level - 1) {
      e->left = makePrimesTreeElem(level-1, vec, ++i);
      e->right = makePrimesTreeElem(level-1, vec, ++i);
    }
    return e;
  } else {
    return nullptr; 
  }
}

static PrimesTreeElement::Ptr makePrimesTree(int level, 
                                             IntVector& vec) {
  return makePrimesTreeElem(level, vec, vec.cbegin());
}

static bool validatePrimesTree(PrimesTreeElement::Ptr e, PrimesMap& m) {
  if (e) {
    if ( m[e->v.first] != e->v.second ) {
      return false;
    }
    if (!validatePrimesTree(e->left, m) || !validatePrimesTree(e->right, m)) {
      return false;
    }
  }
  return true;
}

static void warmupTBB() {
  // This is a simple loop that should get workers started.
  // oneTBB creates workers lazily on first use of the library
  // so this hides the startup time when looking at trivial
  // examples that do little real work. 
  tbb::parallel_for(0, tbb::info::default_concurrency(), 
   [=](int) {
      tbb::tick_count t0 = tbb::tick_count::now();
      while ((tbb::tick_count::now() - t0).seconds() < 0.01);
    }
  );
}

