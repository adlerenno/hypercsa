//
// Created by Enno Adler on 02.04.25.
//

#ifndef HYPERCSA_READ_HPP
#define HYPERCSA_READ_HPP

#include "type_definitions.hpp"
using namespace std;

bool read_edges_equal(CompressedHyperGraph &graph, Index edge1, Index edge2);
EdgeIterator query_iterator(CompressedHyperGraph& graph, Edge query, int type);
Index next(EdgeIterator);
EdgeList query(CompressedHyperGraph& graph, Edge query, int type);

#endif //HYPERCSA_READ_HPP
