#include "common/test.h"
#include "common/utils.h"
#include <iostream>
#include "oneapi/tbb/flow_graph.h"
#include "oneapi/tbb/global_control.h"
#include <mutex>
#include <tuple>

class InputNodeBody
{
public:
    int operator() (oneapi::tbb::flow_control &)
    {
        return 1;
    }
};

using IndexerNodeType = oneapi::tbb::flow::indexer_node<int,int>;
using MultifunctionNodeType = oneapi::tbb::flow::multifunction_node<IndexerNodeType::output_type, std::tuple<int>, oneapi::tbb::flow::lightweight>;

class MultifunctionNodeBody
{
public:
    void operator()(MultifunctionNodeType::input_type, MultifunctionNodeType::output_ports_type)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        throw std::exception();
    }
};

TEST_CASE("420") {
    oneapi::tbb::flow::graph g;
    oneapi::tbb::flow::input_node<int> input1(g, InputNodeBody());
    oneapi::tbb::flow::input_node<int> input2(g, InputNodeBody());
    IndexerNodeType indexer(g);
    MultifunctionNodeType multi(g, oneapi::tbb::flow::serial, MultifunctionNodeBody());
    oneapi::tbb::flow::make_edge(indexer, multi);
    oneapi::tbb::flow::make_edge(input1, indexer);
    oneapi::tbb::flow::make_edge(input2, oneapi::tbb::flow::input_port<1>(indexer));

    input1.activate();
    input2.activate();
    try
    {
        g.wait_for_all();
    }
    catch (const std::exception&)
    {
        std::cout << "Exception from wait_for_all" << std::endl;
    }
}