//
// Created by Enno Adler on 22.04.25.
//

#include <iostream>
#include "type_definitions.hpp"

using namespace std;

#define MAX_LINE_LENGTH (1024).
/*
 * Argument base_zero: Reduce all node numbers such that the lowest node has id 0.
 */
int parse_graph(const char *input_file, HyperGraph& graph, bool base_zero)
{
    std::ifstream infile(input_file);
    std::string line;
    EdgeList edges;
    uint64_t lowest_node = -1;

    while (std::getline(infile, line)) {
        std::vector<uint64_t> nums;
        std::stringstream ss;

        for (char& ch : line) {
            if (ch == ',' || ch == '\t') {
                ch = ' ';
            }
        }

        ss.str(line);
        uint64_t num;
        while (ss >> num) {
            nums.push_back(num);
            if ((lowest_node == -1) || (num < lowest_node))
                lowest_node = num;
        }

        // Create sdsl::int_vector<>
        Edge iv(nums.size());
        for (size_t i = 0; i < nums.size(); ++i) {
            iv[i] = nums[i];
        }

        edges.push_back(iv);
    }

    cout << "Lowest node: " << lowest_node << endl;
    if (lowest_node != 0 && base_zero) // If not 0-based, transform it 0-based.
    {
        for (auto & edge : edges)
        {
            for (int i=0; i < edge.size(); i++)
            {
                edge[i] -= lowest_node;
            }
        }
    }
    graph.edge_count = edges.size();
    graph.edges = edges;

//    int res = -1;
//    bool err = false;
//    char line[MAX_LINE_LENGTH];
//    FILE* edge_file = fopen((const char*) input_file, "r");
//
//    if(!edge_file)
//        return res;
//
//    uint64_t cn = 0;
//    long long n[LIMIT_MAX_RANK];
//    while (fgets(line, sizeof(line), edge_file) && !err) {
//        cn = 0;
//        // Process each line of the hyperedge file
//        // Split the line into individual items using strtok
//        char* token = strtok(line, " ,\t\n"); //Use empty space, tab, and newline as delimiter.
//        while (token != NULL) {
//            n[cn++] = strtoll(token, NULL, 0);
//            if (cn == LIMIT_MAX_RANK)
//                return -1; //Allowed number of parameters are exceeded.
//            token = strtok(NULL, " ,\t\n");
//        }
//        if (cgraphw_add_edge_id(g, cn, max_rank_one_edge_label_count + cn, n) < 0) { // TODO: Label is always cn for this parser, because we need labels depending on the rank.
//            err = true;
//        }
//    }
//    fclose(edge_file);
    return 0;
}

int write_hyper_csa(const char *output_file, CompressedHyperGraph& g)
{
    std::ofstream out(output_file);
    g.D.serialize(out);
    g.PSI.serialize(out);
    out.flush();
    out.close();
    return 0;
}

CompressedHyperGraph load_hyper_csa(const char *input_file) {
    ifstream in(input_file); //TODO: Catch exceptions here.
    sdsl::enc_vector<> psi;
    sdsl::bit_vector d;
    d.load(in);
    psi.load(in);
    in.close();
    return {d, psi}; // CompressedHyperGraph.
}
