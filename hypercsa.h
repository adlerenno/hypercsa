//
// Created by Enno Adler on 23.04.25.
//

#ifndef HYPERCSA_HYPERCSA_H
#define HYPERCSA_HYPERCSA_H

#ifdef __cplusplus
extern "C" {
#endif
    /**
    * Type used as the handler for the API calls for compressing and writing graphs.
    * The exact type of the handler is not defined in the header,
    * because it should only be known internally by the cgraph functions.
    */


    ///////////// Test and CLI related operations ////////////////////////
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
