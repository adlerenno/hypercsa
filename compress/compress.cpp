//
// Created by Enno Adler on 01.04.25.
//

#include <iostream>
#ifdef USE_PARALLEL_EXECUTION
#include <execution>
#endif
#include <sdsl/enc_vector.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/suffix_arrays.hpp>
#include <sdsl/rank_support.hpp>
#include "compress.h"
#include "util/type_definitions.h"

#define BITS_64

using namespace std;
using namespace sdsl;

typedef struct {
    uint64_t edge_count;
    vector<Edge> edges;
} HyperGraph;

void print_int_vector(int_vector<64> *vec) {
    std::cout << "int_vector: ";
    for (size_t i = 0; i < vec->size(); ++i) {
        std::cout << (*vec)[i] << " ";
    }
    std::cout << std::endl;
}

void print_enc_vector(enc_vector<> *vec) {
    std::cout << "enc_vector: ";
    for (size_t i = 0; i < vec->size(); ++i) {
        std::cout << (*vec)[i] << " ";
    }
    std::cout << std::endl;
}

void print_psi(int_vector<> *vec) {
    std::cout << "PSI: ";
    for (size_t i = 0; i < vec->size(); ++i) {
        std::cout << (*vec)[i] << " ";
    }
    std::cout << std::endl;
}

void print_d(bit_vector *d)
{
    cout << *d << endl;
}

void print_psi_cycles(int_vector<> *vec) {
    for (size_t i = 0; i < vec->size(); ++i) {
        if ((*vec)[i] <= i) // Happens only once each edge.
        {
            cout << "(";
            size_t j = (*vec)[i];
            while (j != i)
            {
                cout << j << ", ";
                j = (*vec)[j];
            }
            cout << i << ")";
        }
    }
    cout << endl;
}

void print_edges(int_vector<> *vec, bit_vector *d) {
    rank_support_v<1> rank_d(d);
    for (size_t i = 0; i < vec->size(); ++i) {
        if ((*vec)[i] <= i) // Happens only once each edge.
        {
            cout << "(";
            size_t j = (*vec)[i];
            while (j != i)
            {
                cout << rank_d(j+1)-1 << ", ";
                j = (*vec)[j];
            }
            cout << rank_d(i+1)-1 << ")";
        }
    }
    cout << endl;
}

int parse_graph(const char *input_file, HyperGraph *graph)
{
    // graph->edges; // TODO: initialise.
    Edge edge1(2, 1, 64);
    edge1[1] = 2;
    Edge edge2(3, 1, 64);
    edge2[0] = 0;
    edge2[2] = 4;
    Edge edge3(5, 0, 64);
    util::set_to_id(edge3);
    vector<Edge> edges = {edge2, edge1, edge3};
    graph->edges = edges;
    graph->edge_count = 3;
    print_int_vector(&edge1);
    print_int_vector(&edge2);
    print_int_vector(&edge3);
    return 0;
}

uint64_t size_of_hypergraph(HyperGraph *graph)
{
    uint64_t sum = 0;
    for (uint64_t i = 0; i < graph->edge_count; i++)
    {
        sum += graph->edges[i].size();
    }
    return sum;
}

bool compare_desc(const Edge &a, const Edge &b) {
    return lexicographical_compare(b.begin(), b.end(), a.begin(), a.end());
}

int compute_linear_representation(HyperGraph *graph, int_vector<64> *linear)
{
    // Sort each edge ascending.
    for (uint32_t i = 0; i < graph->edge_count; i++)
    {
        sort(graph->edges[i].begin(), graph->edges[i].end());
    }
    // Sort the edges descending.
    sort(graph->edges.begin(), graph->edges.end(), compare_desc);
    // Copy nodes to linear representation.
    size_t pos = 0;
    for (const auto &edge : graph->edges)
    {
        copy(edge.begin(), edge.end(), linear->begin() + pos);
        pos += edge.size();
    }
    for (size_t i = 0; i < linear->size(); i++) {
        (*linear)[i]++;
    }
    return 0;
}

int adjust_psi(int_vector<> *psi) //, bit_vector *d)
{
    // TODO: Restriction: Self loops are maximal by this interpretation: Multiple self loops at the same node are treated as one.
    // Idea: Psi forms one big loop. I cut the loop each time we do a jump to an earlier position.
    uint64_t current_position = 0;
    uint64_t next = (*psi)[0]; // Definitely greater 0, because there is a next suffix (otherwise the whole graph is one rank-1 edge).
    uint64_t last_first_edge_position = 0;
    do  { // do while loop to avoid stopping at the first iteration.
        if (current_position > next)
        {
            (*psi)[current_position] = last_first_edge_position;
            last_first_edge_position = next;
        }
        current_position = next;
        next = (*psi)[next];
    } while (current_position != 0);

    // Remove first and last and subtract 1 from each element (due to deletion of the first element).
    for (uint64_t i = 1; i < psi->size()-1; i++)
    {
        (*psi)[i-1] = (*psi)[i]-1;
    }
    (*psi).resize(psi->size()-2);
    return 0;
}

int calc_d(int_vector<64> *linear, bit_vector *d)
{
    map<uint64_t, size_t> freq_table;
    for (auto val : *linear) {
        freq_table[val]++;
    }
    uint64_t last_val = 0; // Only used to check proper working. Lowest value starts at 1.
    uint64_t pos = 0;
    for (const auto& [value, count] : freq_table) {
        assert(value == last_val + 1); // ERROR: Encoding of values is not continuous, or map is not sorted.
        (*d)[pos] = true;
        pos += count;
        last_val++;
    }
    (*d)[pos++] = true; // Additional 1 at the end to enable select(i+1)-1 for the end of intervals.
    assert(pos == d->size());
    return 0;
}

int write_hyper_csa(const char *output_file, bit_vector *d, enc_vector<> *psi)
{
    std::ofstream out(output_file);
    d->serialize(out);
    psi->serialize(out);
    out.flush();
    out.close();
    return 0;
}

int construct_hypercsa(const char *input_file, const char *output_file) {
#ifdef TRACK_MEMORY
    memory_monitor::start();
#endif

    HyperGraph graph;
    parse_graph(input_file, &graph);

    int_vector<64> linear_representation(size_of_hypergraph(&graph), 0, 64);
    compute_linear_representation(&graph, &linear_representation);
    print_int_vector(&linear_representation);

    // int_vector<>, 32, 32, sa_order_sa_sampling<>, isa_sampling<>, integer_alphabet
    csa_sada<> csa;
    construct_im(csa, linear_representation, 8); // Note that csa.size = linear_representation.size + 1 (auto adds the 0 at the end)
    cout << "Size of CSA: " << size_in_bytes(csa) << " bytes, " << size_in_mega_bytes(csa) << "MB." << endl;

    // Adjust PSI
    int_vector<> psi_copy(csa.size());
#ifdef USE_OPENMP
    #pragma omp parallel for
    for (size_t i = 1; i < csa.size(); i++) {
        psi_copy[i] = csa.psi[i];
    }
#elifdef USE_PARALLEL_EXECUTION
    copy(execution::par, csa.psi.begin()+1, csa.psi.end(), psi_copy.begin());
#else
    copy(csa.psi.begin(), csa.psi.end(), psi_copy.begin()); // +1, because the first the is zeros element.
#endif
    print_psi(&psi_copy);
    print_psi_cycles(&psi_copy);
    adjust_psi(&psi_copy);
    print_psi(&psi_copy);
    print_psi_cycles(&psi_copy);
    enc_vector<> comp_psi(psi_copy);

    // Create D.
    bit_vector d(linear_representation.size()+1, 0); // +1 for an additional 1 at the end to enable interval search via select commands.
    calc_d(&linear_representation, &d);
    cout << "Size of HyperCSA: " << endl;
    cout << "-   D:" << size_in_bytes(d) << "bytes, " << size_in_mega_bytes(d) << "MB." << endl;
    cout << "- PSI:" << size_in_bytes(comp_psi) << "bytes, " << size_in_mega_bytes(comp_psi) << "MB." << endl;
    cout << "- sum:" << size_in_bytes(d) + size_in_bytes(comp_psi) << "bytes, "
         << size_in_mega_bytes(d) + size_in_mega_bytes(comp_psi) << "MB." << endl;

    print_d(&d);
    print_psi_cycles(&psi_copy);
    print_edges(&psi_copy, &d);

    write_hyper_csa(output_file, &d, &comp_psi);

#ifdef TRACK_MEMORY
    memory_monitor::stop();
    memory_monitor::write_memory_log<JSON_FORMAT>(cout);
#endif
}