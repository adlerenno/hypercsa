//
// Created by Enno Adler on 23.04.25.
//

#ifndef HYPERCSA_HYPERCSA_H
#define HYPERCSA_HYPERCSA_H

#ifdef __cplusplus
extern "C" {
#endif
    int construct_hypercsa(const char *input_file, const char *output_file);
    int query_hypercsa(const char *input_file, int type, const char *node_query); //node_query is expected
    // to be a comma-separated list of numbers.
    void query_hypercsa_from_file(const char* input_file, int type, const char* test_file);

    int test_hypercsa(const char *output_file);
    int test_query(const char *input_file);
#ifdef __cplusplus
}
#endif

#endif //HYPERCSA_HYPERCSA_H
