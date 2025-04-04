//
// Created by Enno Adler on 02.04.25.
//

#include "read.h"

#include <sdsl/int_vector.hpp>
#include <sdsl/enc_vector.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support.hpp>
#include <sdsl/select_support.hpp>
#include "util/type_definitions.h"

using namespace sdsl;
using namespace std;

int load(const string& filename, enc_vector<> *psi, bit_vector *d) {
    ifstream in(filename); //TODO: Catch exceptions here.
    d->load(in);
    psi->load(in);
    in.close();
    return 0;
}

int find_exact_next_interval(enc_vector<> psi, uint64_t *from, uint64_t *to, uint64_t interval_start, uint64_t interval_end) {
    // First
    uint64_t middle;
    while (*from < *to) {
        middle = (*from) + ((*to) - (*from)) / 2;
        uint64_t middle_next = psi[middle];
        if (middle_next > interval_end)
            *to = middle - 1;
        else if (middle_next < interval_start)
            *from = middle + 1;
        else { // middle == query[1]
            break; // Because we want an interval and we found two
        }
    }
    if (*to < *from)
        return -1;
    // TODO: what if not found? I think, this case is not solved yet.
    uint64_t lower_middle = middle;
    uint64_t higher_middle = middle;

    while (*from < lower_middle) {
        middle = (*from) + (lower_middle - (*from)) / 2;
        uint64_t middle_next = psi[middle];
        if (middle_next > interval_start)
            lower_middle = middle; // middle might be the lowest position pointing (psi) into the interval.
        else if (middle_next < interval_start)
            *from = middle + 1;
        else { // middle == query[1]
            *from = middle; // There is no smaller value pointing towards the interval by construction.
            break;
        }
    }

    while (higher_middle < *to) {
        middle = higher_middle + ((*to) - higher_middle) / 2;
        uint64_t middle_next = psi[middle];
        if (middle_next > interval_end)
            *to = middle - 1;
        else if (middle_next < interval_end)
            *from = middle; // Middle could be the highest position pointing (psi) into the interval.
        else { // middle == query[1]
            *to = middle;
            break; // we found the jump to the border of the interval -> there is no better value.
        }
    }

    return 0;
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
int query_perform(enc_vector<> psi, bit_vector d, Edge query, int type)
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
    from = select_d(query[0]), to = select_d(query[0]+1)-1;

    // Goal: Find the interval that matches query[1] and the set of intervals that point to nodes lower than query[1].
    for (uint64_t i = 1; i < query.size(); i++)
    {
        uint64_t next_from = select_d(query[i]), next_to = select_d(query[i]+1)-1;
        int res = find_exact_next_interval(psi, &from, &to, next_from, next_to);
        if (res == -1)
            return -1; // No results found.
        from = psi[from];
        to = psi[to];
    }

    if (type == 0)
    {
        for (uint64_t i = from; i <= to; i++) // to is inclusive here in the current implementation
        {
            if (psi[i] <= i) // Check if edge has no further nodes.
            {  //TODO: Collect result.
                decompress_edge(&psi, &rank_d, psi[i]); // psi[i] is the smallest index of the edge.
            }
        }
    }

    return 0;
}

int query(string filename, Edge query)
{
    enc_vector<> psi;
    bit_vector d;
    load(filename, &psi, &d);
    query_perform(psi, d, query, 0);
}

int test_query(string filename)
{

}