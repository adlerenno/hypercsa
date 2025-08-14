//
// Created by Enno Adler on 23.04.25.
//

#ifndef HYPERCSA_PRINTS_HPP
#define HYPERCSA_PRINTS_HPP

#include "type_definitions.hpp"
#include <sdsl/int_vector.hpp>

void print_edge(Edge *vec);
void print_linear_representation(sdsl::int_vector<64> *vec);
void print_psi_vector(sdsl::int_vector<> *psi);
void print_psi(CompressedHyperGraph *g);
void print_d(CompressedHyperGraph *g);
void print_psi_vector_cycles(sdsl::int_vector<> *psi);
void print_psi_cycles(CompressedHyperGraph *g);
bool check_and_print_sanity(CompressedHyperGraph &g);
void print_edges(CompressedHyperGraph *g);
void print_edge(CompressedHyperGraph& graph, int64_t sa_position);
int64_t find_i(CompressedHyperGraph& graph, int64_t psi_i);

#endif //HYPERCSA_PRINTS_HPP
