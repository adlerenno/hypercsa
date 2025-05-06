//
// Created by Enno Adler on 22.04.25.
//

#include "test.h"

#include "type_definitions.hpp"
#include "prints.hpp"

using namespace sdsl;
using namespace std;

int test_graph(HyperGraph& graph)
{
    Edge edge1 = {0, 1, 2, 3};
    Edge edge2 = {1, 2, 3};
    Edge edge3 = {2};
    Edge edge4 = {0, 1, 2, 4};
    Edge edge5 = {2};
    vector<Edge> edges = {edge1, edge2, edge3, edge4, edge5};
    graph.edges = edges;
    graph.edge_count = edges.size();
    cout << "Graph" << endl;
    print_edge(&edge1);
    print_edge(&edge2);
    print_edge(&edge3);
    print_edge(&edge4);
    print_edge(&edge5);
    return 0;
}


