//
// Created by Enno Adler on 08.08.25.
//

#ifndef HYPERCSA_MODIFY_HPP
#define HYPERCSA_MODIFY_HPP
#include "type_definitions.hpp"
int modify_delete_edge(CompressedHyperGraph& hgraph, Index pos);

int modify_delete_node_from_edge(CompressedHyperGraph& hgraph, Index pos, Node node);

int modify_insert_edge(CompressedHyperGraph& hgraph, Edge edge);

int modify_insert_node_to_edge(CompressedHyperGraph& hgraph, Index pos, Node node);

#endif //HYPERCSA_MODIFY_HPP
