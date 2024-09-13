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

#include <atomic>
#include <iostream>
#include <memory>
#include <tbb/tbb.h>

static void spinWaitForAtLeast(double sec);

class BigObject {
public:
   BigObject();
   BigObject(int i);
   BigObject(const BigObject& b);
   BigObject(BigObject&&) = default;
   BigObject& operator=(const BigObject& b);
   BigObject& operator=(BigObject&&) = default;
   virtual ~BigObject();

   int getId() const;
   int mergeIds(int id0, int id1);
private:
   int id;
};
using BigObjectPtr = std::shared_ptr<BigObject>;

using CompositeType = 
  tbb::flow::composite_node<std::tuple<BigObjectPtr, BigObjectPtr>,
                            std::tuple<BigObjectPtr>>;

class MergeNode : public CompositeType {
  using token_t = int;
  tbb::flow::buffer_node<token_t> tokenBuffer;
  tbb::flow::join_node<std::tuple<BigObjectPtr, BigObjectPtr, token_t>, 
                       tbb::flow::reserving> join;
  using MFNode = 
    tbb::flow::multifunction_node<std::tuple<BigObjectPtr, BigObjectPtr, token_t>,
                                  std::tuple<BigObjectPtr, token_t>>;
  MFNode combine;

public:
  MergeNode(tbb::flow::graph& g) :
    CompositeType{g},
    tokenBuffer{g},
    join{g},
    combine{g, tbb::flow::unlimited,
      [] (const MFNode::input_type& in, MFNode::output_ports_type& p) {
        BigObjectPtr b0 = std::get<0>(in); 
        BigObjectPtr b1 = std::get<1>(in); 
        token_t t = std::get<2>(in);
        spinWaitForAtLeast(0.0001);      
        b0->mergeIds(b0->getId(), b1->getId());      
        std::get<0>(p).try_put(b0);
        std::get<1>(p).try_put(t);
      }}
  {
    // make the edges
    tbb::flow::make_edge(tokenBuffer, tbb::flow::input_port<2>(join));
    tbb::flow::make_edge(join, combine);
    tbb::flow::make_edge(tbb::flow::output_port<1>(combine), tokenBuffer);

    // set the input and output ports
    CompositeType::set_external_ports(
      CompositeType::input_ports_type(
        tbb::flow::input_port<0>(join),
        tbb::flow::input_port<1>(join)
      ),
      CompositeType::output_ports_type(
        tbb::flow::output_port<0>(combine)
      )
    );

    for (token_t i = 0; i < 3; ++i)
      tokenBuffer.try_put(i);
  }
};

const int A_LARGE_NUMBER = 100;
static void reset_counters();

void usingMergeNode() {
  tbb::flow::graph g;

  tbb::flow::input_node<BigObjectPtr> source1{g, 
  [&] (tbb::flow_control& fc) {
    static int in1_count = 0;
    BigObjectPtr p;
    if (in1_count < A_LARGE_NUMBER)
      p = std::make_shared<BigObject>(in1_count++);
    else 
      fc.stop();
    return p;
  }};


 tbb::flow::input_node<BigObjectPtr> source2{g, 
  [&] (tbb::flow_control& fc) {
    static int in2_count = 0;
    BigObjectPtr p;
    if (in2_count < A_LARGE_NUMBER)
      p = std::make_shared<BigObject>(in2_count++);
    else 
      fc.stop();
    return p;
  }};

  MergeNode merge{g};

  tbb::flow::function_node<BigObjectPtr> output{g, 
    tbb::flow::serial,
    [] (BigObjectPtr b) {
      std::cout << "Received id == " << b->getId() 
                << " in final node" << std::endl;
    }
  };

  tbb::flow::make_edge(source1, tbb::flow::input_port<0>(merge));
  tbb::flow::make_edge(source2, tbb::flow::input_port<1>(merge));
  tbb::flow::make_edge(merge, output);

  reset_counters();
  source1.activate();
  source2.activate();

  g.wait_for_all();

}

static void warmupTBB();
int maxCount = 0;

int main(int argc, char *argv[]) {
  warmupTBB();
  tbb::tick_count t0 = tbb::tick_count::now();
  usingMergeNode();
  auto t1 = (tbb::tick_count::now() - t0).seconds();


  std::cout << "maxCount == " << maxCount << "\n";
  std::cout << "merge node time == " << t1 << "\n";
  return 0;
}

std::atomic<int> bigObjectCount;

BigObject::BigObject() : id(-1) { } 
BigObject::BigObject(int i) : id(i) { 
  int cnt = ++bigObjectCount;
  if (cnt > maxCount) 
    maxCount = cnt;
}
BigObject::~BigObject() {
  --bigObjectCount;
}

BigObject::BigObject(const BigObject& b) : id(b.id) { }

BigObject& BigObject::operator=(const BigObject& b) { 
  id = b.id; 
  return *this; 
}

int BigObject::getId() const {return id;}
int BigObject::mergeIds(int id0, int id1) {return id = id1*100 + id0;}

static void spinWaitForAtLeast(double sec) {
  tbb::tick_count t0 = tbb::tick_count::now();
  while ((tbb::tick_count::now() - t0).seconds() < sec);
}

static void reset_counters() {
  bigObjectCount = 0;
  maxCount = 0;
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


