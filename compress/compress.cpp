//
// Created by Enno Adler on 01.04.25.
//

#include <iostream>
#ifdef USE_PARALLEL_EXECUTION
#include <execution>
#endif
#include <sdsl/enc_vector.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/suffix_arrays.hpp>
#include <sdsl/csa_alphabet_strategy.hpp>
#include "compress.hpp"
#include "util/type_definitions.hpp"
#include "util/prints.hpp"

using namespace std;
using namespace sdsl;

uint64_t size_of_hypergraph(HyperGraph& graph)
{
    uint64_t sum = 0;
    for (uint64_t i = 0; i < graph.edge_count; i++)
    {
        sum += graph.edges[i].size();
    }
    return sum;
}

bool compare_desc(const Edge &a, const Edge &b) {
    return lexicographical_compare(b.begin(), b.end(), a.begin(), a.end());
}

int compute_linear_representation(HyperGraph& graph, int_vector<64> *linear)
{
    // Sort each edge ascending.
    for (uint32_t i = 0; i < graph.edge_count; i++)
    {
        sort(graph.edges[i].begin(), graph.edges[i].end());
    }
    // Sort the edges descending.
    sort(graph.edges.begin(), graph.edges.end(), compare_desc);
    // Copy nodes to linear representation.
    size_t pos = 0;
    for (const auto &edge : graph.edges)
    {
        copy(edge.begin(), edge.end(), linear->begin() + pos);
        pos += edge.size();
    }

    // Increase all values by 1, because 0 will be added by SDSL to construct the suffix array.
    for (size_t i = 0; i < linear->size(); i++) {
        (*linear)[i]++;
    }
    return 0;
}

int adjust_psi(int_vector<> *psi) //, bit_vector *d)
{
    // Restriction: Self-loops are minimal in this interpretation:
    // Self loops with multiple connections are treated as self-loops with only one connection per self-loop.

    // Idea: Psi forms one big cycle. I cut the loop each time we do a jump to an earlier position.
    uint64_t current_position = 0;
    uint64_t next = (*psi)[0]; // Definitely greater 0, because there is a next suffix (otherwise the whole graph is one rank-1 edge).
    uint64_t last_first_node_position = 0;
    do  { // do while loop to avoid stopping at the first iteration.
        if (current_position > next)
        {
            (*psi)[current_position] = last_first_node_position;
            last_first_node_position = next;
        }
        current_position = next;
        next = (*psi)[next];
    } while (current_position != 0);

    // Remove first and last and subtract 1 from each element (due to deletion of the first element).
    // This is only necessary due to the addition of elements to the
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
    uint64_t last_val = 0; // Only used to check proper working. The lowest value starts at 1.
    uint64_t pos = 0;
    for (const auto& [value, count] : freq_table) {
        assert(value == last_val + 1); // ERROR: Encoding of values is not continuous, or the map is not sorted.
        (*d)[pos] = true;
        pos += count;
        last_val++;
    }
    (*d)[pos++] = true; // Additional 1 at the end to enable select(i+1)-1 for the end of intervals.
    assert(pos == d->size());
    return 0;
}

CompressedHyperGraph construct(HyperGraph& graph) {
#ifdef TRACK_MEMORY
    memory_monitor::start();
#endif

    LinearRepresentation linear_representation(size_of_hypergraph(graph), 0, 64);
    compute_linear_representation(graph, &linear_representation);
#ifdef VERBOSE_DEBUG
        print_linear_representation(&linear_representation);
#endif

    //
    csa_sada<enc_vector<>, 32, 32, sa_order_sa_sampling<>, isa_sampling<>, int_alphabet<>> csa; // TODO: Use http://vios.dc.fi.udc.es/indexing/wsi/
    construct_im(csa, linear_representation, 8); // Note that csa.size = linear_representation.size + 1 (auto adds the 0 at the end)
#ifdef VERBOSE
        cout << "Size of CSA: " << size_in_bytes(csa) << " bytes, " << size_in_mega_bytes(csa) << "MB." << endl;
#endif

    // Adjust PSI
    int_vector<> psi_copy(csa.size());
#ifdef USE_OPENMP
    #pragma omp parallel for
    for (size_t i = 0; i < csa.size(); i++) {
        psi_copy[i] = csa.psi[i];
    }
#elifdef USE_PARALLEL_EXECUTION
    copy(execution::par, csa.psi.begin(), csa.psi.end(), psi_copy.begin());
#else
    copy(csa.psi.begin(), csa.psi.end(), psi_copy.begin());
#endif
#ifdef VERBOSE
        print_psi_vector(&psi_copy);
        print_psi_vector_cycles(&psi_copy);
#endif

    adjust_psi(&psi_copy);

    enc_vector<> comp_psi(psi_copy);

    // Create D.
    bit_vector d(linear_representation.size()+1, 0); // +1 for an additional 1 at the end to enable interval search via select commands.
    calc_d(&linear_representation, &d);

#ifdef TRACK_MEMORY
    memory_monitor::stop();
    memory_monitor::write_memory_log<JSON_FORMAT>(cout);
#endif
    return {d, comp_psi};
}