//
// Created by Enno Adler on 23.04.25.
//

#include "hypercsa.h"

#include "type_definitions.hpp"
#include "compress.hpp"
#include "parse.hpp"
#include "read.h"

#include "test.h"
#ifdef VERBOSE_DEBUG
#include "prints.hpp"
#endif

using namespace std;

Edge parse_edge_from_string(const std::string& input) {
    std::vector<uint64_t> values;
    std::stringstream ss(input);
    std::string token;

    // Split by comma
    while (std::getline(ss, token, ',')) {
        if (!token.empty()) {
            values.push_back(std::stoull(token));
        }
    }

    // Fill int_vector
    Edge edge(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        edge[i] = values[i];
    }

    return edge;
}

int construct_hypercsa(const char *input_file, const char *output_file)
{
    HyperGraph graph;
    parse_graph(input_file, graph);
    CompressedHyperGraph compressed_graph = construct(graph);
    write_hyper_csa(output_file, compressed_graph);
    return 0;
}

int query_hypercsa(const char *input_file, int type, const char *node_query)
{
    CompressedHyperGraph compressed_graph = load_hyper_csa(input_file);
    query(compressed_graph, parse_edge_from_string(node_query), type);
    return 0;
}

void query_hypercsa_from_file(const char* input_file, int type, const char* test_file)
{
    HyperGraph graph;
    parse_graph(test_file, graph);
    CompressedHyperGraph compressed_graph = load_hyper_csa(input_file);
    for (int i=0; i < graph.edge_count; i++)
    {
        EdgeList el = query(compressed_graph, graph.edges[i], type);
        cout << "Query " << i << " has " << el.size() << " results." << endl;
    }
}

int test_hypercsa(const char *output_file)
{
    HyperGraph graph;
    test_graph(graph);
    CompressedHyperGraph compressed_graph = construct(graph);

#ifdef VERBOSE_DEBUG
        cout << "Data to review" << endl;
        print_d(&compressed_graph);
        print_psi(&compressed_graph);
        print_psi_cycles(&compressed_graph);
        cout << "Decompress" << endl;
        print_edges(&compressed_graph);

        cout << "Size of HyperCSA: " << endl;
        cout << "- D  : " << size_in_bytes(compressed_graph.D) << "bytes, " << size_in_mega_bytes(compressed_graph.D) << "MB." << endl;
        cout << "- PSI: " << size_in_bytes(compressed_graph.PSI) << "bytes, " << size_in_mega_bytes(compressed_graph.PSI) << "MB." << endl;
        cout << "- sum: " << size_in_bytes(compressed_graph.D) + size_in_bytes(compressed_graph.PSI) << "bytes, "
             << size_in_mega_bytes(compressed_graph.D) + size_in_mega_bytes(compressed_graph.PSI) << "MB." << endl;
#endif

    write_hyper_csa(output_file, compressed_graph);
    return 0;
}

int test_query(const char *filename)
{
    CompressedHyperGraph compressed = load_hyper_csa(filename);
    cout << "Test Query {0, 1}" << endl;
    Edge e = {0, 1};
    query(compressed, e, CONTAIN);
    cout << "Test Query {1, 3}" << endl;
    e = {1, 3};
    query(compressed, e, CONTAIN);
    cout << "Test Query {1, 3, 4}" << endl;
    e = {1, 3, 4};
    query(compressed, e, CONTAIN);
    return 0;
}
