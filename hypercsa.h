//
// Created by Enno Adler on 23.04.25.
//

#ifndef HYPERCSA_HYPERCSA_H
#define HYPERCSA_HYPERCSA_H

#ifdef __cplusplus
extern "C" {
#endif
    ///////////// Test and CLI related operations ////////////////////////
    int construct_hypercsa(const char *input_file, const char *output_file);
    int query_hypercsa(const char *input_file, int type, const char *node_query); //node_query is expected
    // to be a comma-separated list of numbers.
    void query_hypercsa_from_file(const char* input_file, int type, const char* test_file);

    int test_hypercsa(const char *output_file);
    int test_hypercsa_delete_edge();
    int test_hypercsa_delete_node_from_edge();
    int test_hypercsa_insert_node_to_edge();
    int test_query(const char *input_file);
#ifdef __cplusplus
}
#endif

#include "type_definitions.hpp"

namespace hypercsa {

// Construction
    CompressedHyperGraph construct(const char *input_file);

    CompressedHyperGraph from_file(const char *input_file);

    int to_file(CompressedHyperGraph hgraph, const char *output_file);

// Updates
    bool edges_equal(CompressedHyperGraph &graph, Index edge1, Index edge2);

    int delete_edge(CompressedHyperGraph &graph, Index pos);

    int delete_node_from_edge(CompressedHyperGraph &graph, Index pos, Node node);

    int insert_edge(CompressedHyperGraph &graph, Edge edge);

    int insert_node_to_edge(CompressedHyperGraph &graph, Index pos, Node node);

// Queries
    //EdgeIterator query(CompressedHyperGraph hgraph, int type, Edge edge);

    //Index edge_iterator_next(EdgeIterator ei);

    //void edge_iterator_finish(EdgeIterator ei);
}

#endif //HYPERCSA_HYPERCSA_H
