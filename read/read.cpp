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

#include "util/type_definitions.h"

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

int load(const string& filename, enc_vector<> *psi, bit_vector *d) {
    ifstream in(filename); //TODO: Catch exceptions here.
    d->load(in);
    psi->load(in);
    in.close();
    return 0;
}


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

void decompress_edge(enc_vector<> *psi, rank_support_v<1> *rank_d, uint64_t index) {
    cout << "(";
    size_t j = (*psi)[index];
    while (j != index)
    {
        cout << (*rank_d)(j+1)-1 << ", ";
        j = (*psi)[j];
    }
    cout << (*rank_d)(index+1)-1 << ")" << endl;
}

#define EXACT 0
#define CONTAINS 1
int query_perform(enc_vector<> psi, const bit_vector& d, Edge query)
{
    // initialize structures for search.
    rank_support_v<1> rank_d(&d);
    select_support_mcl<1> select_d(&d);

    // Sort the query for processing.
    sort(query.begin(), query.end());
    for (unsigned long long & i : query)
    {
        i++; // Add 1, as also the compression is 1-based on nodes.
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
            return -1; // No results found.
        from = psi[from];
        to = psi[to-1]+1; //-1 +1 for staying in intervall and then extend it afterwards.
    }

    from = select_d(query[0]), to = select_d(query[0]+1);
    for (uint64_t i = from; i < to; i++) // to is inclusive here in the current implementation
    {
        if (from <= psi[i] && psi[i] < to) // <= i) // Check if edge has no further nodes (Psi[i] < i).
            // Need to check psi[i] \in [from, to) to secure that there is no lower node.
        {  //TODO: Collect result.
            decompress_edge(&psi, &rank_d, i); // psi[i] is the smallest index of the edge.
        }
    }
    return 0;
}

#define EXACT 0
#define CONTAINS 1
// TODO: Use search intervals and different the 4 areas correctly.
int query_perform_contains(enc_vector<> psi, const bit_vector& d, Edge query)
{
    // initialize structures for search.
    rank_support_v<1> rank_d(&d);
    select_support_mcl<1> select_d(&d);

    // Sort the query for processing.
    sort(query.begin(), query.end());
    for (unsigned long long & i : query)
    {
        i++; // Add 1, as also the compression is 1-based on nodes.
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
        uint64_t start = lower_from;
        uint64_t node_of_start = rank_d(psi[start]); // Only the change of a node is important.
        uint64_t node_current;
        for (uint64_t i = start+1; i < lower_to; i++)
        {
            node_current = rank_d(psi[i]);
            if (node_current > node_of_start)
            //if (d[i + 1]) // Is next a 1 → i is the end of interval.
            {
                search_to_do.emplace(psi[start], psi[i-1]+1, si.index);
                // start = i + 1;
                node_of_start = node_current;
            }
        }
        if (start < lower_to) // add last interval
        {
            search_to_do.emplace(psi[start], psi[lower_to-1]+1, si.index);
        }

        // Exact next result. Added here, because then the queue stays more sorted regarding si.index (but not fully).
        if (res > 0)
        {
            if (si.index+1 == query.size()) {
                for (uint64_t i = from; i < to; i++) // to is exclusive in the current implementation
                {
                    decompress_edge(&psi, &rank_d, i);
                }
            } else {
                search_to_do.emplace(psi[from], psi[to - 1] + 1, si.index + 1);
            } //-1 +1 for staying in intervall and then extend it afterward.
        }
    }

    return 0;
}

int query(const string& filename, Edge query)
{
    enc_vector<> psi;
    bit_vector d;
    load(filename, &psi, &d);
    query_perform_contains(psi, d, std::move(query));
    return 0;
}

int test_query(const char *filename)
{
    cout << "Test Query {0, 1, 4}" << endl;
    Edge e = {0, 1, 4};
    query(string(filename), e);
    cout << "Test Query {1, 2, 4}" << endl;
    e = {1, 2, 4};
    query(string(filename), e);
    cout << "Test Query {3, 1, 4}" << endl;
    e = {3, 1, 4};
    query(string(filename), e);
    return 0;
}