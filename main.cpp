/**
 * @file main.cpp
 * @author EA
 */

#include <iostream>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <getopt.h>

#include <hypercsa.h>

using namespace std::filesystem;

// used to convert the default values of the compression to a string
#define STRINGIFY( x) #x
#define STR(x) STRINGIFY(x)

static void print_usage(bool error) {
	static const char* usage_str = \
	"Usage: hypercsa-cli\n"
    "-h                                        show this help\n"
    "-i [input] -o [output]                    to compress a hypergraph. hypergraph format is one edge per line, nodes are integers and comma separated\n"
    "-i [input] -t [type] -q [list of nodes]   evaluate query on compressed hypergraph. query is a comma-separated-list of integers\n"
    "-i [input] -t [type] -f [queryfile]       evaluates all queries in the file, one query per line.\n"
    "                                          Type 0 is exists query, Type 1 is contains query.\n"

	;
	FILE* os = error ? stderr : stdout;
	fprintf(os, "%s", usage_str);
}


#define check_mode(mode_compress, mode_read, compress_expected, option_name) \
do { \
    if(compress_expected) { \
        if(mode_read) { \
            fprintf(stderr, "option '-%s' not allowed when compressing\n", option_name); \
            return -1; \
        } \
        mode_compress = true; \
    } \
    else { \
        if(mode_compress) { \
            fprintf(stderr, "option '-%s' not allowed when reading the compressed graph\n", option_name); \
            return -1; \
        } \
        mode_read = true; \
    } \
} while(0)

int main(int argc, char** argv) {
	if(argc <= 1) {
		print_usage(true);
		return EXIT_FAILURE;
	}

    int opt;
    std::string input_file;
    std::string output_file;
    std::string node_query;
    std::string test_file;
    int type = 0;
    bool mode_compress = false;
    bool mode_read = false;
    while ((opt = getopt(argc, argv, "hi:o:t:q:f:")) != -1) {
        switch (opt) {
            case 'i':
                input_file = optarg;
                if (!exists(input_file)) {
                    printf("Invalid input file.");
                    return EXIT_FAILURE;
                }
                break;
            case 'o':
                check_mode(mode_compress, mode_read, true, "o");
                output_file = optarg;
                if (output_file.empty()) {
                    printf("Output file not specified correctly.");
                    return -1;
                }
                break;
            case 't':
                check_mode(mode_compress, mode_read, false, "t");
                type = std::stoi(optarg);
                if (type < 0 || type > 3) {
                    printf("Invalid query Type.");
                    return EXIT_FAILURE;
                }
                break;
            case 'q':
                check_mode(mode_compress, mode_read, false, "q");
                node_query = optarg;
                break;
            case 'f':
                check_mode(mode_compress, mode_read, false, "f");
                test_file = optarg;
                if (!exists(test_file)) {
                    printf("Invalid test file.");
                    return EXIT_FAILURE;
                }
                break;
            case 'h':
            default:
                print_usage(true);
                return 0;
        }
    }

    if (mode_compress) {
        construct_hypercsa(input_file.c_str(), output_file.c_str());
    }
    if (mode_read) {
        if (!test_file.empty())
            query_hypercsa_from_file(input_file.c_str(), type, test_file.c_str());
        if (!node_query.empty())
            query_hypercsa(input_file.c_str(), type, node_query.c_str());
    }
    return EXIT_SUCCESS;
}
