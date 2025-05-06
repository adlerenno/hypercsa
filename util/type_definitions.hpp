//
// Created by Enno Adler on 03.04.25.
//

#ifndef HYPERCSA_TYPE_DEFINITIONS_HPP
#define HYPERCSA_TYPE_DEFINITIONS_HPP

#include <sdsl/int_vector.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/enc_vector.hpp>
#include <utility>

#define EXACT 0
#define CONTAIN 1

typedef sdsl::int_vector<64> Edge;
typedef std::vector<Edge> EdgeList;

typedef sdsl::int_vector<64> LinearRepresentation;

class HyperGraph {
public:
    uint64_t edge_count;
    EdgeList edges;

    HyperGraph() : edge_count(0) {}
};

class CompressedHyperGraph {
public:
    sdsl::bit_vector D;
    sdsl::enc_vector<> PSI;

    CompressedHyperGraph(sdsl::bit_vector d, const sdsl::enc_vector<>& psi)
            : D(std::move(d)), PSI(psi) {}
};

#endif //HYPERCSA_TYPE_DEFINITIONS_HPP
