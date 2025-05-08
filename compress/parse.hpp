//
// Created by Enno Adler on 23.04.25.
//

#ifndef HYPERCSA_PARSE_HPP
#define HYPERCSA_PARSE_HPP

#include "type_definitions.hpp"

int parse_graph(const char *input_file, HyperGraph& graph, bool base_zero);
int write_hyper_csa(const char *output_file, CompressedHyperGraph& g);
CompressedHyperGraph load_hyper_csa(const char *input_file);

#endif //HYPERCSA_PARSE_HPP
