//
// Created by Enno Adler on 02.04.25.
//

#include "read.h"

#include <algorithm>

#include <sdsl/int_vector.hpp>
#include <sdsl/enc_vector.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support.hpp>
#include <sdsl/select_support.hpp>
#include <utility>

#include "util/type_definitions.hpp"
#include "prints.hpp"

using namespace sdsl;
using namespace std;

struct SearchInterval {
    uint64_t lower_bound;
    uint64_t higher_bound;
    uint64_t index; // Contains the position in the query we are currently looking for.

    SearchInterval(uint64_t low, uint64_t high, uint64_t idx)
            : lower_bound(low), higher_bound(high), index(idx) {}

    void print() const {
        std::cout << "Index: " << index
                  << ", Lower Bound: " << lower_bound
                  << ", Higher Bound: " << higher_bound << std::endl;
    }
};


int binary_search_left_jumps(enc_vector<> *psi, uint64_t *from, const uint64_t *to)
{
    uint64_t low = *from, middle, high = *to;
    while (low < high) {
        middle = (low) + ((high) - (low)) / 2;
        if ((*psi)[middle] <= middle) // Backwards jump criteria.
            low = middle + 1;
        else
            high = middle;
    }
    *from = low;
    return 0;
}

int find_exact_next_interval(enc_vector<> *psi, uint64_t *from, uint64_t *to, uint64_t interval_start, uint64_t interval_end) {
    // Solution by ChatGPT, but distance may be very inefficient.
    // Finding `low` (smallest index where psi[low] >= next_from)
    auto low_it = std::lower_bound(psi->begin() + (*from), psi->begin() + (*to), interval_start);
    int low = std::distance(psi->begin(), low_it);

    // Finding `high` (smallest index where psi[high] > next_to)
    auto high_it = std::upper_bound(psi->begin() + (*from), psi->begin() + (*to), interval_end);
    int high = std::distance(psi->begin(), high_it);
    *from = low;
    *to = high;
    return low < high;
}

Edge decompress_edge(enc_vector<> *psi, rank_support_v<1>& rank_d, uint64_t index) {
    vector<int64_t> nodes;
    size_t j = (*psi)[index];
    while (j != index)
    {
        nodes.push_back((int64_t) rank_d(j+1)-1);
        j = (*psi)[j];
    }
    nodes.push_back((int64_t) rank_d(index+1)-1);
    Edge e = Edge(nodes.size(), 0, 64);
    copy(nodes.begin(), nodes.end(), e.begin());
    return e;
}

#define EXACT 0
#define CONTAINS 1
EdgeList query_perform(enc_vector<> psi, const bit_vector& d, Edge query)
{
    // initialize structures for search.
    rank_support_v<1> rank_d(&d);
    select_support_mcl<1> select_d(&d);
    EdgeList edge_list;

    // Sort the query for processing.
    sort(query.begin(), query.end());
    for (int i=0; i < query.size(); i++)
    {
        query[i]++; // Add 1, as also the compression is 1-based on nodes.
    }

    // Find interval for node query[0].
    uint64_t from, to;
    from = select_d(query[0]), to = select_d(query[0]+1);

    // Goal: Find the interval that matches query[1] and the set of intervals that point to nodes lower than query[1].
    for (uint64_t i = 1; i < query.size(); i++)
    {
        uint64_t next_from = select_d(query[i]), next_to = select_d(query[i]+1);
        int res = find_exact_next_interval(&psi, &from, &to, next_from, next_to);
        if (res == 0)
            return edge_list; // No results found. Return an empty list.
        from = psi[from];
        to = psi[to-1]+1; //-1 +1 for staying in intervall and then extend it afterward.
    }

    from = select_d(query[0]), to = select_d(query[0]+1);
    for (uint64_t i = from; i < to; i++) // to is inclusive here in the current implementation
    {
        if (from <= psi[i] && psi[i] < to) // <= i) // Check if edge has no further nodes (Psi[i] < i).
            // Need to check psi[i] \in [from, to) to secure that there is no lower node.
        {  //TODO: Collect result.
            Edge e = decompress_edge(&psi, rank_d, i); // psi[i] is the smallest index of the edge.
            edge_list.push_back(e);
            print_edge(&e);
        }
    }
    return edge_list;
}

// Deprecated, produces false positives.
EdgeList query_perform_contains(enc_vector<> psi, const bit_vector& d, Edge query)
{
    // initialize structures for search.
    rank_support_v<1> rank_d(&d);
    select_support_mcl<1,1> select_d(&d);
    EdgeList edge_list;

    // Sort the query for processing.
    sort(query.begin(), query.end());
    for (int i=0; i < query.size(); i++)
    {
        query[i]++; // Add 1, as also the compression is 1-based on nodes.
    }

    // Find interval for node query[0].
    stack<SearchInterval> search_to_do{};
    search_to_do.emplace(select_d(query[0]), select_d(query[0]+1), 1);

    // Goal: Find the interval that matches query[1] and the set of intervals that point to nodes lower than query[1].
    while (!search_to_do.empty())
    {
        SearchInterval si = search_to_do.top();
        search_to_do.pop();
        uint64_t next_from = select_d(query[si.index]),
            next_to = select_d(query[si.index]+1);
        uint64_t lower_from = si.lower_bound,
            lower_to,
            from = si.lower_bound,
            to = si.higher_bound;
        int res = find_exact_next_interval(&psi, &from, &to, next_from, next_to);


        // Matches with steps that are not in the query.
        lower_to = from;
        binary_search_left_jumps(&psi, &lower_from, &lower_to); // Only increases lower_from.
        uint64_t lower_interval_begin = lower_from;
        uint64_t node_of_start = rank_d(psi[lower_interval_begin]+1); // Only the change of a node is important.
        // +1 because we need the current position included in the rank.
        uint64_t node_current;
        for (uint64_t i = lower_interval_begin + 1; i < lower_to; i++)
        {
            node_current = rank_d(psi[i]+1);
            if (node_current > node_of_start)
            //if (d[i + 1]) // Is next a 1 → i is the end of interval.
            {
                search_to_do.emplace(psi[lower_interval_begin], psi[i - 1] + 1, si.index);
                // lower_interval_begin = i + 1;
                lower_interval_begin = i;
                node_of_start = node_current;
            }
        }
        if (lower_interval_begin < lower_to) // add last interval
        {
            search_to_do.emplace(psi[lower_interval_begin], psi[lower_to - 1] + 1, si.index);
        }

        // Exact next result. Added here, because then the queue stays more sorted regarding si.index (but not fully).
        if (res > 0)
        {
            if (si.index+1 == query.size()) {
                for (uint64_t i = from; i < to; i++) // to is exclusive in the current implementation
                {
                    Edge e = decompress_edge(&psi, rank_d, i); // TODO:
                    // Check again that all nodes are connected, false positives are possible here.
                    // The false positives start with later nodes or skip the first node v_0.
                    edge_list.push_back(e);
                    print_edge(&e);
                }
            } else {
                search_to_do.emplace(psi[from], psi[to - 1] + 1, si.index + 1);
            } //-1 +1 for staying in intervall and then extend it afterward.
        }
    }
    return edge_list;
}

EdgeList query_perform_contains_correct(CompressedHyperGraph& g, Edge& query)
{
    rank_support_v<1> rank_d(&g.D);
    select_support_mcl<1,1> select_d(&g.D);
    EdgeList edge_list;

    // Sort the query for processing.
    sort(query.begin(), query.end());
    for (int i=0; i < query.size(); i++)
    {
        query[i]++; // Add 1, as also the compression is 1-based on nodes.
    }

    // Find smallest degree of nodes in query.
    uint64_t min_interval = std::numeric_limits<uint64_t>::max();
    size_t best_start_node_index = 0;
    for (uint64_t i = 0; i < query.size(); i++) {
        uint64_t next_from = select_d(query[i]);
        uint64_t next_to = select_d(query[i] + 1);
        uint64_t interval_size = next_to - next_from;

        if (interval_size < min_interval) {
            min_interval = interval_size;
            best_start_node_index = i;
        }
    }

    //Process each cycle of PSI starting from the interval of the smallest node-degree and check for the occurrence of nodes.
    uint64_t next_from = select_d(query[best_start_node_index]);
    uint64_t next_to = select_d(query[best_start_node_index] + 1);
    for (uint64_t i = next_from; i < next_to; i++)
    {
        uint64_t current_query_position = (best_start_node_index + 1) % query.size();

        uint64_t current_sa_position = g.PSI[i];
        while (current_sa_position != i && current_query_position != best_start_node_index) {
            if (g.PSI[current_sa_position] <= current_sa_position && current_query_position != 0)
                // Downward jump only allowed if the check for the next position is the lowest node (aka cur_qer_pos=0)
                // Otherwise, a higher node is never reached by this edge.
                break; // Faster omit of not correct edge.
            uint64_t node = rank_d(current_sa_position+1);
            if (node > query[current_query_position] && current_query_position != 0) // Forward jump is greater than the next query node:
                // this edge does not contain the next query node.
                break; // Faster omit of not correct edge.
            if (node == query[current_query_position])
            {
                current_query_position++;
                current_query_position %= query.size();
            }
            current_sa_position = g.PSI[current_sa_position];
        }
        if (current_query_position == best_start_node_index)
        {
            Edge e = decompress_edge(&g.PSI, rank_d, i);
            edge_list.push_back(e);
#ifdef VERBOSE_DEBUG
            print_edge(&e);
#endif
        }
    }

    return edge_list;
}

EdgeList query(CompressedHyperGraph& graph, Edge query, int type)
{
    switch (type) {
        case EXACT:
            return query_perform(graph.PSI, graph.D, std::move(query));
            break;
        case CONTAIN:
            return query_perform_contains_correct(graph, query);
            break;
        default: return {};
    }
}