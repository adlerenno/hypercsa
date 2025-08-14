//
// Created by Enno Adler on 22.04.25.
//

#include <sdsl/enc_vector.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/rank_support.hpp>
#include "type_definitions.hpp"

using namespace std;
using namespace sdsl;

void print_edge(Edge *vec) {
    std::cout << "int_vector: ";
    for (size_t i = 0; i < vec->size(); ++i) {
        std::cout << (*vec)[i] << " ";
    }
    std::cout << std::endl;
}

void print_linear_representation(int_vector<64> *vec) { // cout << "Linear Representation" << endl;
    std::cout << "Size of T: " << vec->size() << endl;
    std::cout << "T: ";
    for (size_t i = 0; i < vec->size(); ++i) {
        std::cout << (*vec)[i] << " ";
    }
    std::cout << std::endl;
}

void print_psi_vector(int_vector<> *PSI)
{
    std::cout << "PSI: ";
    for (size_t i = 0; i < PSI->size(); ++i) {
        std::cout << (*PSI)[i] << " ";
    }
    std::cout << std::endl;
}

void print_psi(CompressedHyperGraph *g)
{
    std::cout << "PSI: ";
    for (size_t i = 0; i < g->PSI.size(); ++i) {
        std::cout << (g->PSI)[i] << " ";
    }
    std::cout << std::endl;
}

void print_d(CompressedHyperGraph *g)
{
    std::cout << "D  : ";
    for (size_t i = 0; i < g->D.size(); ++i) {
        std::cout << (g->D)[i] << " ";
    }
    std::cout << std::endl;
    // cout << *d << endl;
}

void print_psi_vector_cycles(int_vector<> *PSI) {
    // Contains only a single cycle.
    cout << "(";
    size_t j = (*PSI)[0];
    while (j != 0)
    {
        cout << j << ", ";
        j = (*PSI)[j];
    }
    cout << "0)" << endl;
}

void print_psi_cycles(CompressedHyperGraph *g) {
    for (size_t i = 0; i < g->PSI.size(); ++i) {
        if ((g->PSI)[i] <= i) // Happens only once each edge.
        {
            cout << "(";
            size_t j = (g->PSI)[i];
            while (j != i)
            {
                cout << j << ", ";
                j = (g->PSI)[j];
            }
            cout << i << ")";
        }
    }
    cout << endl;
}

bool check_and_print_sanity(CompressedHyperGraph &g)
{
    cout << "Sanity check: ";
    for (Index i = 0; i < g.PSI.size(); i++)
    {
        if (g.D[i] == 0 && g.PSI[i-1] >= g.PSI[i])
        {
            cout << "false" << "\n";
            return false;
        }
    }
    cout << "true" << "\n";
    return true;
}

void print_edges(CompressedHyperGraph *g) {
    rank_support_v<1> rank_d(&(g->D));
    for (size_t i = 0; i < g->PSI.size(); ++i) {
        if ((g->PSI)[i] <= i) // Happens only once each edge.
        {
            cout << "(";
            size_t j = (g->PSI)[i];
            while (j != i)
            {
                cout << rank_d(j+1)-1 << ", ";
                j = (g->PSI)[j];
            }
            cout << rank_d(i+1)-1 << ")";
        }
    }
    cout << endl;
}

void print_edge(CompressedHyperGraph& graph, rank_support_v<1, 1> *rank_d, int64_t index) {
    cout << "(";
    size_t j = (graph.PSI)[index];
    while (j != index)
    {
        cout << (*rank_d)(j+1)-1 << ", ";
        j = (graph.PSI)[j];
    }
    cout << (*rank_d)(index+1)-1 << ")" << endl;
}

int64_t find_i(CompressedHyperGraph& graph, int64_t psi_i)
{
    int64_t i = psi_i;
    do
    {
        i = graph.PSI[i];
    }
    while (graph.PSI[i] != psi_i);
    return i;
}
