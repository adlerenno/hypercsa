/**
 * @file cgraph.c
 * @author FR
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <inttypes.h>
#include <unistd.h>

#include <serd-0/serd/serd.h>

#include <cgraph.h>
#include <constants.h>


#include "arith.h"

// used to convert the default values of the compression to a string
#define STRINGIFY( x) #x
#define STR(x) STRINGIFY(x)

static void print_usage(bool error) {
	static const char* usage_str = \
	"Usage: cgraph-cli\n"
	"    -h,--help                       show this help\n"
	"\n"
	" * to compress a RDF graph:\n"
	"   cgraph-cli [options] [input] [output]\n"
	"                       [input]          input file of the RDF graph\n"
	"                       [output]         output file of the compressed graph\n"
	"\n"
	"   optional options:\n"
	"    -f,--format        [format]         format of the RDF graph; keep empty to auto detect the format\n"
	"                                        possible values: \"turtle\", \"ntriples\", \"nquads\", \"trig\", \"hyperedge\"\n"
	"       --overwrite                      overwrite if the output file exists\n"
	"    -v,--verbose                        print advanced information\n"
	"\n"
	"   options to influence the resulting size and the runtime to browse the graph (optional):\n"
	"       --max-rank      [rank]           maximum rank of edges, set to 0 to remove limit (default: " STR(DEFAULT_MAX_RANK) ")\n"
	"       --monograms                      enable the replacement of monograms\n"
	"       --factor        [factor]         number of blocks of a bit sequence that are grouped into a superblock (default: " STR(DEFAULT_FACTOR) ")\n"
	"       --sampling      [sampling]       sampling value of the dictionary; a value of 0 disables sampling (default: " STR(DEFAULT_SAMPLING) ")\n"
	"       --no-rle                         disable run-length encoding\n"
	"       --no-table                       do not add an extra table to speed up the decompression of the edges for an specific label\n"
#ifdef RRR
    "    --rrr                               use bitsequences based on R. Raman, V. Raman, and S. S. Rao [experimental]\n"
    "                                        --factor can also be applied to this type of bit sequences\n"
#endif
#ifndef RRR
    "    --rrr                               not available. Recompile with -DWITH_RRR=on\n"
#endif
	"\n"
	" * to read a compressed RDF graph:\n"
	"   cgraph-cli [options] [input] [commands...]\n"
	"                       [input]      input file of the compressed RDF graph\n"
	"\n"
	"   optional options:\n"
	"    -f,--format        [format]         default format for the RDF graph at the command `--decompress`\n"
	"                                        possible values: \"turtle\", \"ntriples\", \"nquads\", \"trig\"\n"
	"       --overwrite                      overwrite if the output file exists, used with `--decompress`\n"
	"\n"
	"   commands to read the compressed path:\n"
	"       --decompress    [RDF graph]      decompresses the given compressed RDF graph\n"
	"       --extract-node  [node-id]        extracts the node label of the given node id\n"
	"       --extract-edge  [edge-id]        extracts the edge label of the given edge id\n"
	"       --locate-node   [text]           determines the node id of the node with the given node label\n"
	"       --locate-edge   [text]           determines the edge label id for the given text\n"
	"       --locatep-node  [text]           determines the node ids that have labels starting with the given text\n"
	"       --search-node   [text]           determines the node ids with labels containing the given text\n"
    "       --hyperedges    [rank,label]*{,node}\n"
    "                                        determines the edges with given rank. You can specify any number of nodes\n"
    "                                        that will be checked the edge is connected to. The incidence-type is given\n"
    "                                        implicitly. The label must not be set, use ? otherwise. For example:\n"
    "                                        - \"4,2,?,3,?,4\": determines all rank 4 edges with label 2 that are connected\n"
    "                                           to the node 3 with connection-type 2 and node 4 with connection-type 4.\n"
    "                                        - \"2,?,?,5\": determines all rank 2 edges any label that are connected\n"
    "                                           to the node 5 with connection-type 1. In the sense of regular edges, \n"
    "                                           this asks for all incoming edges of node 5.\n"
    "                                        Note that it is not allowed to pass no label and no nodes to this function.\n"
    "                                        Use --decompress in this case.\n"
    "         --no-order                     Use this flag together with hyperedge to indicate \n"
    "                                        that the order of nodes given to the hyperedge command is no"
    "         --sort-result                  sort the resulting edges using quicksort."
	"       --node-count                     returns the number of nodes in the graph\n"
	"       --edge-labels                    returns the number of different edge labels in the graph\n"
#ifdef WEB_SERVICE
    "       --port          [port]           starts the web-service at the given port. Webserver can be queried via easy SPARQL.\n"
#endif
#ifndef WEB_SERVICE
    "       --port          [port]           not available. Recompile with -DWEB_SERVICE=on\n"
#endif
	;

	FILE* os = error ? stderr : stdout;
	fprintf(os, "%s", usage_str);
}

enum opt {
	OPT_HELP = 'h',
	OPT_VERBOSE = 'v',

	OPT_CR_FORMAT = 'f',
	OPT_CR_OVERWRITE = 1000,

	OPT_C_MAX_RANK,
	OPT_C_MONOGRAMS,
	OPT_C_FACTOR,
	OPT_C_SAMPLING,
	OPT_C_NO_RLE,
	OPT_C_NO_TABLE,
#ifdef RRR
	OPT_C_RRR,
#endif

	OPT_R_DECOMPRESS,
	OPT_R_EXTRACT_NODE,
	OPT_R_EXTRACT_EDGE,
	OPT_R_LOCATE_NODE,
	OPT_R_LOCATE_EDGE,
	OPT_R_LOCATEP_NODE,
	OPT_R_SEARCH_NODE,
	OPT_R_EDGES,
    OPT_R_HYPEREDGES,
    OPT_R_NO_HYPEREDGE_ORDER,
    OPT_R_SORT_RESULT,
	OPT_R_NODE_COUNT,
	OPT_R_EDGE_LABELS,

#ifdef WEB_SERVICE
    OPT_R_PORT,
#endif
};

typedef enum {
	CMD_NONE = 0,
	CMD_DECOMPRESS,
	CMD_EXTRACT_NODE,
	CMD_EXTRACT_EDGE,
	CMD_LOCATE_NODE,
	CMD_LOCATE_EDGE,
	CMD_LOCATEP_NODE,
	CMD_SEARCH_NODE,
	CMD_EDGES,
    CMD_HYPEREDGES,
	CMD_NODE_COUNT,
	CMD_EDGE_LABELS,
#ifdef WEB_SERVICE
    CMD_WEB_SERVICE,
#endif
} CGraphCmd;

typedef struct {
	CGraphCmd cmd;
	union {
		char* arg_str;
		uint64_t arg_int;
	};
} CGraphCommand;

typedef struct {
	// -1 = unknown, 0 = compress, 1 = decompress
	int mode;

	bool verbose;
	char* format;
	bool overwrite;

	// options for compression
	CGraphCParams params;

	// options for reading
	int command_count;
	CGraphCommand commands[1024];
} CGraphArgs;

#define check_mode(mode_compress, mode_read, compress_expected) \
do { \
	if(compress_expected) { \
		if(mode_read) { \
			fprintf(stderr, "option '--%s' not allowed when compressing\n", options[longid].name); \
			return -1; \
		} \
		mode_compress = true; \
	} \
	else { \
		if(mode_compress) { \
			fprintf(stderr, "option '--%s' not allowed when reading the compressed graph\n", options[longid].name); \
			return -1; \
		} \
		mode_read = true; \
	} \
} while(0)

#define add_command_str(argd, c) \
do { \
	if((argd)->command_count == (sizeof((argd)->commands) / sizeof(*(argd)->commands))) { \
		fprintf(stderr, "exceeded the maximum number of commands\n"); \
		return -1; \
	} \
	(argd)->commands[(argd)->command_count].cmd = (c); \
	(argd)->commands[(argd)->command_count].arg_str = optarg; \
	(argd)->command_count++; \
} while(0)

#define add_command_int(argd, c) \
do { \
	if((argd)->command_count == (sizeof((argd)->commands) / sizeof(*(argd)->commands))) { \
		fprintf(stderr, "exceeded the maximum number of commands\n"); \
		return -1; \
	} \
	if(parse_optarg_int(&v) < 0) \
		return -1; \
	(argd)->commands[(argd)->command_count].cmd = (c); \
	(argd)->commands[(argd)->command_count].arg_int = v; \
	(argd)->command_count++; \
} while(0)

#define add_command_none(argd, c) \
do { \
	if((argd)->command_count == (sizeof((argd)->commands) / sizeof(*(argd)->commands))) { \
		fprintf(stderr, "exceeded the maximum number of commands\n"); \
		return -1; \
	} \
	(argd)->commands[(argd)->command_count].cmd = (c); \
	(argd)->command_count++; \
} while(0)

static const char* parse_int(const char* s, uint64_t* value) {
	if(!s || *s == '-') // handle negative values because `strtoull` ignores them
		return NULL;

	char* temp;
	uint64_t val = strtoull(s, &temp, 0);
	if(temp == s || ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE))
		return NULL;

	*value = val;
	return temp;
}

static inline int parse_optarg_int(uint64_t* value) {
	const char* temp = parse_int(optarg, value);
	if(!temp || *temp != '\0')
		return -1;
	return 0;
}

static int parse_args(int argc, char** argv, CGraphArgs* argd) {
	static struct option options[] = {
		// basic options
		{"help", no_argument, 0, OPT_HELP},
		{"verbose", no_argument, 0, OPT_VERBOSE},

		// options for compression or reading
		{"format",  required_argument, 0, OPT_CR_FORMAT},
		{"overwrite", no_argument, 0, OPT_CR_OVERWRITE},

		// options used for compression
		{"max-rank", required_argument, 0, OPT_C_MAX_RANK},
		{"monograms", no_argument, 0, OPT_C_MONOGRAMS},
		{"factor", required_argument, 0, OPT_C_FACTOR},
		{"sampling", required_argument, 0, OPT_C_SAMPLING},
		{"no-rle", no_argument, 0, OPT_C_NO_RLE},
		{"no-table", no_argument, 0, OPT_C_NO_TABLE},
#ifdef RRR
		{"rrr", no_argument, 0, OPT_C_RRR},
#endif

		// options used for browsing
		{"decompress", required_argument, 0, OPT_R_DECOMPRESS},
		{"extract-node", required_argument, 0, OPT_R_EXTRACT_NODE},
		{"extract-edge", required_argument, 0, OPT_R_EXTRACT_EDGE},
		{"locate-node", required_argument, 0, OPT_R_LOCATE_NODE},
		{"locate-edge", required_argument, 0, OPT_R_LOCATE_EDGE},
		{"locatep-node", required_argument, 0, OPT_R_LOCATEP_NODE},
		{"search-node", required_argument, 0, OPT_R_SEARCH_NODE},
		{"edges", required_argument, 0, OPT_R_EDGES},
        {"hyperedges", required_argument, 0, OPT_R_HYPEREDGES},
        {"no-order", no_argument, 0, OPT_R_NO_HYPEREDGE_ORDER},
        {"sort-result", no_argument, 0, OPT_R_SORT_RESULT},
		{"node-count", no_argument, 0, OPT_R_NODE_COUNT},
		{"edge-labels", no_argument, 0, OPT_R_EDGE_LABELS},

#ifdef WEB_SERVICE
        {"port", required_argument, 0, OPT_R_PORT},
#endif

		{0, 0, 0, 0}
	};

	bool mode_compress = false;
	bool mode_read = false;

	argd->mode = -1;
	argd->verbose = false;
	argd->format = NULL;
	argd->overwrite = false;
	argd->params.max_rank = DEFAULT_MAX_RANK;
	argd->params.monograms = DEFAULT_MONOGRAMS;
	argd->params.factor = DEFAULT_FACTOR;
	argd->params.sampling = DEFAULT_SAMPLING;
	argd->params.rle = DEFAULT_RLE;
	argd->params.nt_table = DEFAULT_NT_TABLE;
#ifdef RRR
	argd->params.rrr = DEFAULT_RRR;
#endif
#ifdef WEB_SERVICE
    argd->params.port = DEFAULT_PORT;
#endif
	argd->command_count = 0;

	uint64_t v;
	int opt, longid;
	while((opt = getopt_long(argc, argv, "hvf:", options, &longid)) != -1) {
		switch(opt) {
		case OPT_HELP:
			print_usage(false);
			exit(0);
			break;
		case OPT_VERBOSE:
			// TODO: only when compressing a graph, on reading there is no verbose yet.
			argd->verbose = 1;
			break;
		case OPT_CR_FORMAT:
			argd->format = optarg;
			break;
		case OPT_CR_OVERWRITE:
			argd->overwrite = true;
			break;
		case OPT_C_MAX_RANK:
			check_mode(mode_compress, mode_read, true);
			if(parse_optarg_int(&v) < 0) {
				fprintf(stderr, "max-rank: expected integer\n");
				return -1;
			}

			argd->params.max_rank = v;
			break;
		case OPT_C_MONOGRAMS:
			check_mode(mode_compress, mode_read, true);
			argd->params.monograms = true;
			break;
		case OPT_C_FACTOR:
			check_mode(mode_compress, mode_read, true);
			if(parse_optarg_int(&v) < 0) {
				fprintf(stderr, "factor: expected integer\n");
				return -1;
			}

			argd->params.factor = v;
			break;
		case OPT_C_SAMPLING:
			check_mode(mode_compress, mode_read, true);
			if(parse_optarg_int(&v) < 0) {
				fprintf(stderr, "sampling: expected integer\n");
				return -1;
			}

			argd->params.sampling = v;
			break;
		case OPT_C_NO_RLE:
			check_mode(mode_compress, mode_read, true);
			argd->params.rle = false;
			break;
		case OPT_C_NO_TABLE:
			check_mode(mode_compress, mode_read, true);
			argd->params.nt_table = false;
			break;
#ifdef RRR
		case OPT_C_RRR:
			check_mode(mode_compress, mode_read, true);
			argd->params.rrr = true;
			break;
#endif
		case OPT_R_DECOMPRESS:
			check_mode(mode_compress, mode_read, false);
			add_command_str(argd, CMD_DECOMPRESS);
			break;
		case OPT_R_EXTRACT_NODE:
			check_mode(mode_compress, mode_read, false);
			add_command_int(argd, CMD_EXTRACT_NODE);
			break;
		case OPT_R_EXTRACT_EDGE:
			check_mode(mode_compress, mode_read, false);
			add_command_int(argd, CMD_EXTRACT_EDGE);
			break;
		case OPT_R_LOCATE_NODE:
			check_mode(mode_compress, mode_read, false);
			add_command_str(argd, CMD_LOCATE_NODE);
			break;
		case OPT_R_LOCATE_EDGE:
			check_mode(mode_compress, mode_read, false);
			add_command_str(argd, CMD_LOCATE_EDGE);
			break;
		case OPT_R_LOCATEP_NODE:
			check_mode(mode_compress, mode_read, false);
			add_command_str(argd, CMD_LOCATEP_NODE);
			break;
		case OPT_R_SEARCH_NODE:
			check_mode(mode_compress, mode_read, false);
			add_command_str(argd, CMD_SEARCH_NODE);
			break;
		case OPT_R_EDGES:
			check_mode(mode_compress, mode_read, false);
			add_command_str(argd, CMD_EDGES);
			break;
        case OPT_R_HYPEREDGES:
            check_mode(mode_compress, mode_read, false);
            add_command_str(argd, CMD_HYPEREDGES);
            break;
        case OPT_R_NO_HYPEREDGE_ORDER:
            check_mode(mode_compress, mode_read, false);
            argd->params.no_hyperedge_order = true;
            break;
        case OPT_R_SORT_RESULT:
            check_mode(mode_compress, mode_read, false);
            argd->params.sort_result = true;
            break;
		case OPT_R_NODE_COUNT:
			check_mode(mode_compress, mode_read, false);
			add_command_none(argd, CMD_NODE_COUNT);
			break;
		case OPT_R_EDGE_LABELS:
			check_mode(mode_compress, mode_read, false);
			add_command_none(argd, CMD_EDGE_LABELS);
			break;
        case OPT_R_PORT:
            check_mode(mode_compress, mode_read, false);
            add_command_none(argd, CMD_WEB_SERVICE);
            if(parse_optarg_int(&v) < 0 && v >= 0) {
                fprintf(stderr, "sampling: expected natural number or 0\n");
                return -1;
            }
            argd->params.port = v;
            break;
		case '?':
		case ':':
		default:
			return -1;
		}
	}

	if(mode_compress)
		argd->mode = 0;
	else if(mode_read)
		argd->mode = 1;

	return optind;
}

typedef struct {
	SerdSyntax syntax;
	const char* name;
	const char* extension;
} Syntax;

static const Syntax syntaxes[] = {
	{SERD_TURTLE, "turtle", ".ttl"},
	{SERD_NTRIPLES, "ntriples", ".nt"},
	{SERD_NQUADS, "nquads", ".nq"},
	{SERD_TRIG, "trig", ".trig"},
    {(SerdSyntax) 5, "hyperedge", ".hyperedge"},
    {(SerdSyntax) 6, "cornell_hypergraph", ".txt"},
	{(SerdSyntax) 0, NULL, NULL}
};

static SerdSyntax get_format(const char* format) {
	for(const Syntax* s = syntaxes; s->name; s++) {
		if(!strncasecmp(s->name, format, strlen(format))) {
			return s->syntax;
		}
	}

	return (SerdSyntax) 0;
}
static SerdSyntax guess_format(const char* filename) {
	const char* ext = strrchr(filename, '.');
	if(ext) {
		for(const Syntax* s = syntaxes; s->name; s++) {
			if(!strncasecmp(s->extension, ext, strlen(ext))) {
				return s->syntax;
			}
		}
	}

	return (SerdSyntax) 0;
}

typedef struct {
	SerdEnv* env;
	CGraphW* handler;
} CGraphParserContext;

static void free_handle(void* v) {
	(void) v;
}

static SerdStatus base_sink(void* handle, const SerdNode* uri) {
	CGraphParserContext* ctx = handle;
	SerdEnv* env = ctx->env;
	return serd_env_set_base_uri(env, uri);
}

static SerdStatus prefix_sink(void* handle, const SerdNode* name, const SerdNode* uri) {
	CGraphParserContext* ctx = handle;
	SerdEnv* env = ctx->env;
	return serd_env_set_prefix(env, name, uri);
}

static SerdStatus statement_sink(void* handle, SerdStatementFlags flags, const SerdNode* graph, const SerdNode* subject, const SerdNode* predicate, const SerdNode* object, const SerdNode* object_datatype, const SerdNode* object_lang) {
	CGraphParserContext* ctx = handle;
	SerdEnv* env = ctx->env;
	CGraphW* handler = ctx->handler;

	const char* s = (const char*) subject->buf;
	const char* p = (const char*) predicate->buf;
	const char* o = (const char*) object->buf;
    const char* so[2] = {s, o};
	if(cgraphw_add_edge(handler, 2, p, so) < 0)
		return SERD_FAILURE;

	return SERD_SUCCESS;
}

static SerdStatus end_sink(void* handle, const SerdNode* node) {
	return SERD_SUCCESS;
}

static SerdStatus error_sink(void* handle, const SerdError* error) {
	fprintf(stderr, "error: %s:%u:%u: %s", error->filename, error->col, error->line, error->fmt);
	return SERD_SUCCESS;
}

static int rdf_parse(const char* filename, SerdSyntax syntax, CGraphW* g) {
	int res = -1;

	SerdURI base_uri = SERD_URI_NULL;
	SerdNode base = SERD_NODE_NULL;

	SerdEnv* env = serd_env_new(&base);
	if(!env)
		return -1;

	CGraphParserContext ctx;
	ctx.env = env;
	ctx.handler = g;

	base = serd_node_new_file_uri((uint8_t *) filename, NULL, &base_uri, true);

	SerdReader* reader = serd_reader_new(syntax, &ctx, free_handle, base_sink, prefix_sink, statement_sink, end_sink);
	if(!reader)
		goto exit_0;

	serd_reader_set_strict(reader, true);

	bool err = false;
	serd_reader_set_error_sink(reader, error_sink, &err);

	FILE* in_fd = fopen(filename, "r");
	if(!in_fd)
		goto exit_1;

	SerdStatus status = serd_reader_start_stream(reader, in_fd, (uint8_t *) filename, false);
	while(status == SERD_SUCCESS)
		status = serd_reader_read_chunk(reader);

	fclose(in_fd);

	if(!err)
		res = 0;

exit_1:
	serd_reader_end_stream(reader);
	serd_reader_free(reader);
exit_0:
	serd_env_free(env);
	serd_node_free(&base);

	return res;
}

#define MAX_LINE_LENGTH (8*LIMIT_MAX_RANK)  // With 8, it gives 1024 for a max_rank of 128.
static int hyperedge_parse(const char* filename, SerdSyntax syntax, CGraphW* g) {
    int res = -1;

    bool err = false;

    FILE* in_fd = fopen((const char*) filename, "r");
    if(!in_fd)
        return res;

    char line[MAX_LINE_LENGTH];
    char* n[LIMIT_MAX_RANK];
    size_t cn = 0;
    while (fgets(line, sizeof(line), in_fd) && !err) {
        cn = 0;
        // Process each line of the hyperedge file
        // Split the line into individual items using strtok
        char* token = strtok(line, " \t\n"); //Use empty space, tab, and newline as delimiter.
        while (token != NULL) {
            n[cn++] = token;
            if (cn == LIMIT_MAX_RANK)
                return -1; //Allowed number of parameters are exceeded.
            token = strtok(NULL, " \t\n");
        }
        if (cgraphw_add_edge(g, cn-1, n[0], (const char **) (n + 1)) < 0) {
            err = true;
        }
    }

    fclose(in_fd);

    if(!err)
        res = 0;

    return res;
}

void prepend_text_to_filename(const char *file_path, const char *text, char *result) {
    // Find the last occurrence of the directory separator '/'
    const char *filename = strrchr(file_path, '/');

    if (filename) {
        // Include the '/' in the filename pointer
        filename++;
    } else {
        // If no '/', the entire file_path is the filename
        filename = file_path;
    }

    // Calculate the length of the directory path
    size_t dir_length = filename - file_path;

    // Copy the directory part to the result
    strncpy(result, file_path, dir_length);
    result[dir_length] = '\0';

    // Append the text to the result
    strcat(result, text);

    // Append the filename to the result
    strcat(result, filename);
}

#define MAX_LINE_LENGTH (8*LIMIT_MAX_RANK)  // With 8, it gives 1024 for a max_rank of 128.
static int cornell_hyperedge_parse(const char* filename, SerdSyntax syntax, CGraphW* g) {
    int res = -1;

    bool err = false;
    char line[MAX_LINE_LENGTH];

    char* fname = malloc((12+strlen(filename)) * sizeof(char));

    prepend_text_to_filename(filename, "label-names-", fname);
    FILE* label_names_file = fopen((const char*) fname, "r");
    if (!label_names_file)
        return res;
    CGraphNode max_rank_one_edge_label_count = -1;
    while (fgets(line, sizeof(line), label_names_file) && !err) {
        CGraphNode node_name = cgraphw_put_label(g, line, false);
        max_rank_one_edge_label_count = node_name > max_rank_one_edge_label_count ? node_name : max_rank_one_edge_label_count;
    }
    fclose(label_names_file);

    printf("- Require the first %lld edge labels to be rank 1 edges.\n", max_rank_one_edge_label_count);

    prepend_text_to_filename(filename, "node-names-", fname);
    FILE* node_names_file = fopen((const char*) fname, "r");
    if (node_names_file)
    {
        while (fgets(line, sizeof(line), node_names_file) && !err) {
            cgraphw_put_label(g, line, true);
        }
        fclose(node_names_file);
    }
    else
    {
        cgraphw_set_param_no_node_labels(g);
    }

    CGraphNode n[LIMIT_MAX_RANK];
    prepend_text_to_filename(filename, "node-labels-", fname);
    FILE* node_labels_file = fopen((const char*) fname, "r");

    if(!node_labels_file)
        return res;

    CGraphNode cn = 0;
    while (fgets(line, sizeof(line), node_labels_file) && !err) {
        n[0] = cn++;
        if (cgraphw_add_edge_id(g, 1, strtoll(line, NULL, 0)-1, n) < 0) { // Edge label is ID, Only Node is the id. -1, because the files are 1-based instead of 0-based.
            err = true;
        }
    }
    fclose(node_labels_file);

    prepend_text_to_filename(filename, "hyperedges-", fname);
    FILE* edge_file = fopen((const char*) fname, "r");

    if(!edge_file)
        return res;

    cn = 0;
    while (fgets(line, sizeof(line), edge_file) && !err) {
        cn = 0;
        // Process each line of the hyperedge file
        // Split the line into individual items using strtok
        char* token = strtok(line, " ,\t\n"); //Use empty space, tab, and newline as delimiter.
        while (token != NULL) {
            n[cn++] = strtoll(token, NULL, 0);
            if (cn == LIMIT_MAX_RANK)
                return -1; //Allowed number of parameters are exceeded.
            token = strtok(NULL, " ,\t\n");
        }
        if (cgraphw_add_edge_id(g, cn, max_rank_one_edge_label_count + cn, n) < 0) { // TODO: Label is always cn for this parser, because we need labels depending on the rank.
            err = true;
        }
    }
    fclose(edge_file);

    if(!err)
        res = 0;

    return res;
}

static int do_compress(const char* input, const char* output, const CGraphArgs* argd) {
	if(!argd->overwrite) {
		if(access(output, F_OK) == 0) {
			fprintf(stderr, "Output file \"%s\" already exists.\n", output);
			return -1;
		}
	}

	SerdSyntax syntax;
	if(argd->format)
		syntax = get_format(argd->format);
	else
    {
        syntax = guess_format(input);
        if (argd->verbose && syntax)
        {
            printf("Guessing file format: %s\n", syntaxes[syntax].name);
        }
    }
	if(!syntax)
    {
        syntax = SERD_TURTLE;
        if (argd->verbose)
        {
            printf("Guessing file format: %s\n", syntaxes[syntax].name);
        }
    }

	if(argd->verbose) {
		printf("Compression parameters:\n");
		printf("- max-rank: %d\n", argd->params.max_rank);
		printf("- monograms: %s\n", argd->params.monograms ? "true" : "false");
		printf("- factor: %d\n", argd->params.factor);
		printf("- sampling: %d\n", argd->params.sampling);
		printf("- rle: %s\n", argd->params.rle ? "true" : "false");
		printf("- nt-table: %s\n", argd->params.nt_table ? "true" : "false");
#ifdef RRR
		printf("- rrr: %s\n", argd->params.rrr ? "true" : "false");
#endif
	}

	CGraphW* g = cgraphw_init();
	if(!g)
		return -1;

	cgraphw_set_params(g, &argd->params);

	int res = -1;

    if (syntax < 5)
    {
        if(argd->verbose)
            printf("Parsing RDF file %s\n", input);
        if(rdf_parse(input, syntax, g) < 0) {
            fprintf(stderr, "Failed to read file \"%s\".\n", input);
            goto exit_0;
        }
    }
    else if (syntax == 5)
    {
        if(argd->verbose)
            printf("Parsing Hyperedge file %s\n", input);
        if(hyperedge_parse(input, syntax, g) < 0) {
            fprintf(stderr, "Failed to read file \"%s\".\n", input);
            goto exit_0;
        }
    }
    else if (syntax == 6)
    {
        if(argd->verbose)
            printf("Parsing Cornell Hyperedge file %s\n", input);
        if(cornell_hyperedge_parse(input, syntax, g) < 0) {
            fprintf(stderr, "Failed to read file \"%s\".\n", input);
            goto exit_0;
        }
    }


	if(argd->verbose)
		printf("Applying repair compression\n");

	if(cgraphw_compress(g) < 0) {
		fprintf(stderr, "failed to compress graph\n");
		goto exit_0;
	}

	if(argd->verbose)
		printf("Writing compressed graph to %s\n", output);

	if(cgraphw_write(g, output, argd->verbose) < 0) {
		fprintf(stderr, "failed to write compressed graph\n");
		goto exit_0;
	}

	res = 0;

exit_0:
	cgraphw_destroy(g);
	return res;
}

static char* rdf_node(CGraphR* g, uint64_t v, bool is_node, SerdNode* node) {
	char* s;
	if(is_node)
		s = cgraphr_extract_node(g, v, NULL);
	else
		s = cgraphr_extract_edge_label(g, v, NULL);
	if(!s)
		return NULL;

    if (node != NULL)
    {
        if (!*s) // empty
            *node = serd_node_from_string(SERD_BLANK, (const uint8_t *) s);
        else if (!strncmp("http://", s, strlen("http://")) || !strncmp("http://", s, strlen("http://"))) // is url
            *node = serd_node_from_string(SERD_URI, (const uint8_t *) s);
        else // literal
            *node = serd_node_from_string(SERD_LITERAL, (const uint8_t *) s);
    }

	return s;
}

static int do_decompress(CGraphR* g, const char* output, const char* format, bool overwrite) {
	int res = -1;

	if(!overwrite) {
		if(access(output, F_OK) == 0) {
			fprintf(stderr, "Output file \"%s\" already exists.\n", output);
			return -1;
		}
	}

	SerdSyntax syntax;
	if(format)
		syntax = get_format(format);
	else
		syntax = guess_format(output);
	if(!syntax)
		syntax = SERD_TURTLE;


    FILE* out_fd = fopen((const char*) output, "w+");
    if(!out_fd) {
        fprintf(stderr, "Failed to write to file \"%s\".\n", output);
        goto exit_0;
    }

    if (syntax == 5) // Syntax is hyperedge file
    {
        char *label, *txt;
        CGraphEdgeIterator *it = cgraphr_edges_all(g);
        if (!it) {
            goto exit_0;
        }
        uint64_t number_of_edges = 0;
        CGraphEdge n;
        while (cgraphr_edges_next(it, &n)) {
            label = rdf_node(g, n.label, false, NULL);
            //write label here.
            if (fprintf(out_fd, "%s", label) == EOF)
            {
                free(label);
                cgraphr_edges_finish(it);
                goto exit_0;
            }
            free(label);
            for (CGraphRank i = 0; i < n.rank; i++)
            {
                txt = rdf_node(g, n.nodes[i], true, NULL);
                //Write empty space plus txt here;
                int suc = fprintf(out_fd, " %s", txt);
                if (suc  == EOF) {
                    free(txt);
                    cgraphr_edges_finish(it);
                    goto exit_0;
                }
                free(txt);
            }
            if (fprintf(out_fd, "\n") == EOF)
            {
                cgraphr_edges_finish(it);
                goto exit_0;
            }
            number_of_edges++;
        }
        printf("Decompressed %llu edges.\n", number_of_edges);
        res = 0;
    }
    else {

        SerdURI base_uri = SERD_URI_NULL;
        SerdNode base = SERD_NODE_NULL;

        SerdEnv *env = serd_env_new(&base);
        if (!env)
            return -1;

        base = serd_node_new_file_uri((const uint8_t *) output, NULL, &base_uri, true);

        SerdStyle output_style = SERD_STYLE_ABBREVIATED | SERD_STYLE_ASCII | SERD_STYLE_BULK;

        SerdWriter *writer = serd_writer_new(syntax, output_style, env, &base_uri, serd_file_sink, out_fd);
        if (!writer)
            goto exit_1;

        bool err = false;
        serd_writer_set_error_sink(writer, error_sink, &err);

        char *txt_s, *txt_p, *txt_o;
        SerdNode s, p, o;

        CGraphEdgeIterator *it = cgraphr_edges_all(g);
        if (!it) {
            goto exit_2;
        }

        CGraphEdge n;
        while (cgraphr_edges_next(it, &n)) {
            txt_s = rdf_node(g, n.nodes[0], true, &s);
            txt_p = rdf_node(g, n.label, false, &p);
            txt_o = rdf_node(g, n.nodes[1], true, &o);

            int res = serd_writer_write_statement(writer, 0, &base, &s, &p, &o, NULL, NULL);
            free(txt_s);
            free(txt_p);
            free(txt_o);

            if (res != SERD_SUCCESS || err) {
                cgraphr_edges_finish(it);
                goto exit_2;
            }
        }

        res = 0;

        exit_2:
        serd_writer_finish(writer);
        serd_writer_free(writer);
        exit_1:
        serd_env_free(env);
        serd_node_free(&base);
    }
exit_0:
    fclose(out_fd);
	return res;
}

#ifdef WEB_SERVICE

struct MHDParams {
    const CGraphArgs *argd;
    CGraphR* graph;
};

static enum MHD_Result
generate_server_answer(void * cls,
         struct MHD_Connection * connection,
         const char * url,
         const char * method,
         const char * version,
         const char * upload_data,
         size_t * upload_data_size,
         void ** ptr);

static void do_webservice(CGraphR* g, const CGraphArgs* argd) {
    struct MHD_Daemon * d;
    struct MHDParams params = {argd, g};
    d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
                         argd->params.port,
                         NULL,
                         NULL,
                         &generate_server_answer,
                         &params,
                         MHD_OPTION_END);
    if (d == NULL)
    {
        fprintf(stderr, "Failed to start Webserver daemon.");
        return;
    }
    if (argd->verbose)
        printf("Server started on port %d. Type %c to quit.\n", argd->params.port, DEFAULT_QUIT_SERVER_CHAR);
    while (getc (stdin) != DEFAULT_QUIT_SERVER_CHAR) {}
    MHD_stop_daemon(d);
}
#endif

// ******* Helper functions *******

typedef struct {
	uint64_t node_src;
	uint64_t node_dst;
	uint64_t label;
} EdgeArg;

int parse_edge_arg(const char* s, EdgeArg* arg) {
	uint64_t node_src, node_dst, label;

	s = parse_int(s, &node_src);
	if(!s)
		return -1;

	switch(*s) {
	case '\0':
		node_dst = -1;
		label = -1;
		goto exit;
	case ',':
		break;
	default:
		return -1;
	}

	if(*(++s) == '?') {
		s++;
		node_dst = -1;
	}
	else {
		s = parse_int(s, &node_dst);
		if(!s)
			return -1;
	}

	switch(*s) {
	case '\0':
		label = -1;
		goto exit;
	case ',':
		break;
	default:
		return -1;
	}

	s = parse_int(s + 1, &label);
	if(!s || *s != '\0')
		return -1;

exit:
	arg->node_src = node_src;
	arg->node_dst = node_dst;
	arg->label = label;
	return 0;
}

typedef struct {

    /* The rank of the edge. */
    CGraphRank rank;

    /* The edge label of the edge. */
    CGraphEdgeLabel label;

    /* The nodes of the edge. */
    CGraphNode nodes[LIMIT_MAX_RANK];
} HyperedgeArg;

int parse_hyperedge_arg(const char* s, HyperedgeArg* arg, bool* exists_query, bool* predicate_query) {
    uint64_t rank, label;
    *exists_query = true;
    *predicate_query = true;

    s = parse_int(s, &rank);
    if(!s)
        return -1;

    for (int j=0; j < rank; j++) {
        arg->nodes[j] = -1;
    }

    switch(*s) {
        case '\0':
            rank = -1;
            label = -1;
            goto exit;
        case ',':
            break;
        default:
            return -1;
    }

    if(*(++s) == '?') {
        s++;
        label = -1;
        *exists_query = false;
    }
    else {
        s = parse_int(s, &label);
        if(!s)
            return -1;
    }

    for (int npc = 0; npc < rank && * s == ','; npc++) {
        if(*(++s) == '?') {
            s++;
            *exists_query = false;
            // Already initialized all values with -1.
        }
        else {
            s = parse_int(s, (uint64_t *) &arg->nodes[npc]);
            *predicate_query = false;
            if (!s)
                return -1;
        }
    }

    exit:
    arg->rank = rank;
    arg->label = label;
    return 0;
}

typedef struct {
    /* Should output node. */
    bool output_s;
    bool output_p;
    bool output_o;

    /* The nodes of the requested triples. */
    char *subject;
    char *predicate;
    char *object;
} SPARQLArg;

int parse_sparql_arg(const char *query, SPARQLArg *arg, bool *existence_query, bool *predicate_query,
                     bool *decompression_query) {
    arg->output_s = false;
    arg->output_p = false;
    arg->output_o = false;
    *existence_query = true;
    *predicate_query = true;
    *decompression_query = true;

    // Step 1: Check if the query starts with "SELECT"
    if (strncmp(query, "SELECT", 6) != 0) {
        // printf("Query must start with 'SELECT'\n");
        return 1;
    }

    // Step 2: Find the WHERE clause
    const char *end = strstr(query, "WHERE"); // Find position of WHERE.
    if (!end) {
        // printf("Query must contain a 'WHERE' clause\n");
        return 1;
    }

    // Step 3: Extract the SELECT clause content
    const char *start = query + 6; // Skip the SELECT

    char content[LINE_MAX];
    strncpy(content, start + 1, end - start - 1);
    content[end - start - 1] = '\0';
    char *token = strtok(content, SPARQL_WHITESPACE_CHARS);
    while (token != NULL)
    {
        if (strcmp(token, "?s") == 0)
            arg->output_s = true;
        else if (strcmp(token, "?p") == 0)
            arg->output_p = true;
        else if (strcmp(token, "?o") == 0)
            arg->output_o = true;
        token = strtok(NULL, SPARQL_WHITESPACE_CHARS);
    }

    // Step 4: Extract the WHERE clause content
    start = strchr(end, '{');
    end = strchr(end, '}');
    if (!start || !end || start > end) {
        // printf("WHERE clause must contain '{' and '}' with triple pattern\n");
        return 1;
    }

    // Copy the WHERE clause content between braces
    strncpy(content, start + 1, end - start - 2);
    content[end - start - 1] = '\0';

    // Step 4a: Tokenize WHERE clause and assign values to s, p, o
    token = strtok(content, SPARQL_WHITESPACE_CHARS);
    if (strcmp(token, "?s") == 0) {
        *existence_query = false;
        arg->subject = NULL;
    } else {
        *predicate_query = false;
        *decompression_query = false;
        char *s = malloc(strlen(token)+1);
        strcpy(s, token);
        arg->subject = s;
    }
    token = strtok(NULL, SPARQL_WHITESPACE_CHARS);
    if (strcmp(token, "?p") == 0) {
        *existence_query = false;
        arg->predicate = NULL;
    } else {
        *decompression_query = false;
        char *s = malloc(strlen(token)+1);
        strcpy(s, token);
        arg->predicate = s;
    }
    token = strtok(NULL, SPARQL_WHITESPACE_CHARS);
    if (strcmp(token, "?o") == 0) {
        *existence_query = false;
        arg->object = NULL;
    } else {
        *predicate_query = false;
        *decompression_query = false;
        char *s = malloc(strlen(token)+1);
        strcpy(s, token);
        arg->object = s;
    }
    return 0;
}

// comparing two nodes
static int cmp_node(const void* e1, const void* e2)  {
	CGraphNode* v1 = (CGraphNode*) e1;
	CGraphNode* v2 = (CGraphNode*) e2;

	int64_t cmpn = *v1 - *v2;
	if(cmpn > 0) return  1;
	if(cmpn < 0) return -1;
	return 0;
}

typedef struct {
	size_t len;
	size_t cap;
	CGraphNode* data;
} NodeList;

// low level list with capacity
void node_append(NodeList* l, CGraphNode n) {
	size_t cap = l->cap;
	size_t len = l->len;

	if(cap == len) {
		cap = !cap ? 8 : (cap + (cap >> 1));
		CGraphNode* data = realloc(l->data, cap * sizeof(*data));
		if(!data)
			exit(1);

		l->cap = cap;
		l->data = data;
	}

	l->data[l->len++] = n;
}

// comparing two edges to sort it
//  1. by label (higher label first)
//  2. by node (higher node first) and
//  3. by rank (higher rank first)
static int cmp_edge(const void* e1, const void* e2)  {

	CGraphEdge* v1 = (CGraphEdge*) e1;
	CGraphEdge* v2 = (CGraphEdge*) e2;

    int64_t cmp = v1->label - v2->label;
	if(cmp > 0) return  1;
	if(cmp < 0) return -1;
    for (int j=0; j < MIN(v1->rank, v2->rank); j++)
    {
        cmp = v1->nodes[j] - v2->nodes[j];
        if(cmp > 0) return  1;
        if(cmp < 0) return -1;
    }
    cmp = v1->rank - v2->rank;
    if(cmp > 0) return  1;
    if(cmp < 0) return -1;
	return 0;
}

typedef struct {
	size_t len;
	size_t cap;
	CGraphEdge* data;
} EdgeList;

// low level list with capacity
void edge_append(EdgeList* l, CGraphEdge* e) {
	size_t cap = l->cap;
	size_t len = l->len;

	if(cap == len) {
		cap = !cap ? 8 : (cap + (cap >> 1));
		CGraphEdge* data = realloc(l->data, cap * sizeof(*data));
		if(!data)
			exit(1);

		l->cap = cap;
		l->data = data;
	}

	l->data[l->len++] = *e;
}

bool do_search(CGraphR* g, CGraphRank rank, CGraphEdgeLabel label, CGraphNode* nodes, bool exist_query, bool predicate_query, bool no_hyperedge_order, bool sort_result, EdgeList* result)
{
    if (exist_query) {
        return cgraphr_edge_exists(g, rank, label, nodes, no_hyperedge_order);
    }
    CGraphEdgeIterator* it;
    if (predicate_query)
    {
        it = cgraphr_edges_by_predicate(g, label);
    }
    else
    {
        it = cgraphr_edges(g, rank, label, nodes, no_hyperedge_order) ;
    }

    if(!it)
        return false;

    CGraphEdge n;
    while(cgraphr_edges_next(it, &n))
        edge_append(result, &n);

    // sort the edges
    if (sort_result)
    {
        qsort(result->data, result->len, sizeof(CGraphEdge), cmp_edge);
    }
    return result->len > 0;
}

#ifdef WEB_SERVICE
void ip_of_client(struct MHD_Connection *connection, char* client_ip)
{
    const union MHD_ConnectionInfo *conn_info = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    if (conn_info && conn_info->client_addr) {

        // Check if the address is IPv4 or IPv6 and convert it to a string
        // Check if it's an IPv4 or IPv6 address and convert it to string
        if (conn_info->client_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr_in = (struct sockaddr_in *)conn_info->client_addr;
            inet_ntop(AF_INET, &(addr_in->sin_addr), client_ip, INET_ADDRSTRLEN);
        } else if (conn_info->client_addr->sa_family == AF_INET6) {
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)conn_info->client_addr;
            inet_ntop(AF_INET6, &(addr_in6->sin6_addr), client_ip, INET6_ADDRSTRLEN);
        }
    }
}

enum MHD_Result
generate_server_answer(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
                       const char *version, const char *upload_data, size_t *upload_data_size, void **ptr) {

    const CGraphArgs *argd = ((struct MHDParams *)cls)->argd;
    CGraphR* g = ((struct MHDParams *) cls)->graph;
    struct MHD_Response *response;
    int ret;

    const char *sparql_query = NULL;
    char client_ip[INET6_ADDRSTRLEN];
    if (strcmp(method, "POST") == 0) {
        if (argd->verbose)
        {
            ip_of_client(connection, client_ip);
            printf("Query via POST from %s\n", client_ip);
        }
        sparql_query = strndup(upload_data, *upload_data_size);
    } else if (strcmp(method, "GET") == 0) {
        if (argd->verbose)
        {
            ip_of_client(connection, client_ip);
            printf("Query via GET from %s\n", client_ip);
        }
        sparql_query = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "query");
    }
    if (sparql_query == NULL)
    {
        return MHD_NO;
    }

    SPARQLArg *sparqlArg = malloc(sizeof (SPARQLArg));
    if (sparqlArg == NULL)
        return MHD_NO;
    bool existence_query, predicate_query, decompression_query;

    // Checks for correct parsing here.
    if (parse_sparql_arg(sparql_query, sparqlArg, &existence_query, &predicate_query, &decompression_query) != 0) {
        return MHD_NO;
    }

    EdgeList ls = {0};
    CGraphNode s_id = CGRAPH_NODES_ALL;
    CGraphEdgeLabel p_id = CGRAPH_LABELS_ALL;
    CGraphNode o_id = CGRAPH_NODES_ALL;
    if (sparqlArg->subject != NULL) {
        s_id = cgraphr_locate_node(g, sparqlArg->subject);
        if (s_id == -1) {
            goto empty_answer;
        }
    }
    if (sparqlArg->predicate != NULL) {
        p_id = cgraphr_locate_edge_label(g, sparqlArg->predicate);
        if (p_id == -1) {
            goto empty_answer;
        }
    }
    if (sparqlArg->object != NULL) {
        o_id = cgraphr_locate_node(g, sparqlArg->object);
        if (o_id == -1) {
            goto empty_answer;
        }
    }

    CGraphNode nodes[] = {s_id, o_id};
    bool found = do_search(g, 2, p_id, nodes, existence_query, predicate_query, false, false, &ls);

    empty_answer:;  // position to jump to, if lookup failed.
    if (argd->verbose)
    {
        printf("Found %zu answers to %s query.\n", ls.len, client_ip);
    }
    size_t json_length = 1024 + ls.len * 100;  // Adjust size if needed
    char *json = malloc(json_length);
    signed int json_current_position = 0;
    signed int json_additionally_written = 0;
    // S is the char array to build string at, L is S length, CP is the current working position, AW the number of last added symbols, F the snprintf format string and all additional arguments are passed to snprintf.
#define json_append(S, L, CP, AW, F, ...) do { \
        snprintf(S + CP, L - CP, F "%n", ##__VA_ARGS__, &AW); \
        CP += AW; \
    } while (0)

    json_append(json, json_length, json_current_position, json_additionally_written, "{ \"head\": { \"vars\": [ %s%s%s%s%s ] }, \"results\": { \"bindings\": [",
             (sparqlArg->output_s ? "\"s\"" : ""),
             (sparqlArg->output_s && (sparqlArg->output_p || sparqlArg->output_o) ? ", " : ""), // Only add a comma if s and (p or o) are written.
             (sparqlArg->output_p ? "\"p\"" : ""),
             (sparqlArg->output_p && sparqlArg->output_o ? ", " : ""), // Only add a comma if p and o are written.
             (sparqlArg->output_o ? "\"o\"" : ""));

    char* subj, *pred, *obje;
    for(size_t i = 0; i < ls.len || (existence_query && found && i == 0); i++) {
        json_append(json, json_length, json_current_position, json_additionally_written, "{");


        if (sparqlArg->output_s) {
            if (sparqlArg->subject != NULL) {
                subj = sparqlArg->subject;
            } else {
                subj = cgraphr_extract_node(g, ls.data[i].nodes[0], NULL);
            }
            json_append(json, json_length, json_current_position, json_additionally_written, "\"s\": %s", subj);
            if (!sparqlArg->subject) {
                free(subj);
            }

            if (sparqlArg->output_p || sparqlArg->output_o)
            {
                json_append(json, json_length, json_current_position, json_additionally_written, ", ");
            }
        }
        if (sparqlArg->output_p) {
            if (sparqlArg->predicate != NULL) {
                pred = sparqlArg->predicate;
            } else {
                pred = cgraphr_extract_edge_label(g, ls.data[i].label, NULL);
            }
            json_append(json, json_length, json_current_position, json_additionally_written, "\"p\": %s", pred);
            if (!sparqlArg->predicate) {
                free(pred);
            }

            if (sparqlArg->output_o)
            {
                json_append(json, json_length, json_current_position, json_additionally_written, ", ");
            }
        }
        if (sparqlArg->output_o) {
            if (sparqlArg->object) {
                obje = sparqlArg->object;
            } else {
                obje = cgraphr_extract_node(g, ls.data[i].nodes[1], NULL);
            }
            json_append(json, json_length, json_current_position, json_additionally_written, "\"o\": %s", obje);
            if (!sparqlArg->object) {
                free(obje);
            }
        }

        json_append(json, json_length, json_current_position, json_additionally_written, "}");
        if (i < ls.len - 1) {
            json_append(json, json_length, json_current_position, json_additionally_written, ", ");
        }
    }
    json_append(json, json_length, json_current_position, json_additionally_written, "] } }");

    free(sparqlArg->subject); free(sparqlArg->predicate); free(sparqlArg->object);
    free(sparqlArg);
    *ptr = NULL; /* clear context pointer */
    response = MHD_create_response_from_buffer(json_current_position,
                                               (void *) json,
                                               MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(connection,
                             MHD_HTTP_OK,
                             response);

    for (int i = 0; i < ls.len; i++)
    {
        free(ls.data[i].nodes);
    }
    if(ls.data)
        free(ls.data);
    MHD_destroy_response(response);
    free(json);
    return ret;
}
#endif

static int do_read(const char* input, const CGraphArgs* argd) {
	CGraphR* g = cgraphr_init(input);
	if(!g) {
		fprintf(stderr, "failed to read compressed graph %s\n", input);
		return -1;
	}

	if(argd->command_count == 0) {
		fprintf(stderr, "no commands given\n");
		return -1;
	}

	int res = -1;

	for(int i = 0; i < argd->command_count; i++) {
		const CGraphCommand* cmd = &argd->commands[i];

		switch(cmd->cmd) {
		case CMD_DECOMPRESS:
			if(do_decompress(g, cmd->arg_str, argd->format, argd->overwrite) < 0)
				goto exit; // terminate the reading of the graph

			res = 0;
			break;
		case CMD_EXTRACT_NODE: {
			char* v = cgraphr_extract_node(g, cmd->arg_int, NULL);
			if(v) {
				printf("%s\n", v);
				free(v);
				res = 0;
			}
			else
				fprintf(stderr, "no node found for id %" PRIu64 "\n", cmd->arg_int);
			break;
		}
		case CMD_EXTRACT_EDGE: {
			char* v = cgraphr_extract_edge_label(g, cmd->arg_int, NULL);
			if(v) {
				printf("%s\n", v);
				free(v);
				res = 0;
			}
			else
				fprintf(stderr, "no edge found for id %" PRIu64 "\n", cmd->arg_int);
			break;
		}
		case CMD_LOCATE_NODE: {
			CGraphNode n = cgraphr_locate_node(g, cmd->arg_str);
			if(n >= 0) {
				printf("%" PRId64 "\n", n);
				res = 0;
			}
			else
				fprintf(stderr, "node \"%s\" does not exists\n", cmd->arg_str);
			break;
		}
		case CMD_LOCATE_EDGE: {
			CGraphNode n = cgraphr_locate_edge_label(g, cmd->arg_str);
			if(n >= 0) {
				printf("%" PRId64 "\n", n);
				res = 0;
			}
			else
				fprintf(stderr, "edge label \"%s\" does not exists\n", cmd->arg_str);
			break;
		}
		case CMD_LOCATEP_NODE:
			// fallthrough
		case CMD_SEARCH_NODE: {
			CGraphNodeIterator* it = (cmd->cmd == CMD_LOCATEP_NODE) ?
				cgraphr_locate_node_prefix(g, cmd->arg_str) :
				cgraphr_search_node(g, cmd->arg_str);
			if(!it)
				break;

			CGraphNode n;
			NodeList ls = {0}; // list is empty
			while(cgraphr_node_next(it, &n))
				node_append(&ls, n);

			// sort the nodes
			qsort(ls.data, ls.len, sizeof(CGraphNode), cmp_node);

			for(size_t i = 0; i < ls.len; i++)
				printf("%" PRId64 "\n", ls.data[i]);

			if(ls.data)
				free(ls.data);

			res = 0;
			break;
		}
		case CMD_EDGES:
            // fallthrough
//        {
//			EdgeArg arg;
//			if(parse_edge_arg(cmd->arg_str, &arg) < 0) {
//				fprintf(stderr, "failed to parse edge argument \"%s\"\n", cmd->arg_str);
//				break;
//			}
//
//			if(arg.label != CGRAPH_LABELS_ALL && arg.node_dst != -1) {
//				bool exists = cgraphr_edge_exists(g, arg.node_src, arg.node_dst, arg.label);
//				printf("%d\n", exists ? 1 : 0);
//				res = 0;
//				break;
//			}
//
//			CGraphEdgeIterator* it = (arg.node_dst == -1) ?
//				cgraphr_edges(g, arg.node_src, arg.label) :
//				cgraphr_edges_connecting(g, arg.node_src, arg.node_dst);
//			if(!it)
//				break;
//
//			CGraphEdge n;
//			EdgeList ls = {0}; // list is empty
//			while(cgraphr_edges_next(it, &n))
//				edge_append(&ls, &n);
//
//			// sort the edges
//			qsort(ls.data, ls.len, sizeof(CGraphEdge), cmp_edge);
//
//			for(size_t i = 0; i < ls.len; i++)
//				printf("%" PRId64 "\t%" PRId64 "\t%" PRId64 "\n", ls.data[i].node1, ls.data[i].node2, ls.data[i].label);
//
//			if(ls.data)
//				free(ls.data);
//
//			res = 0;
//			break;
//		}
        case CMD_HYPEREDGES: {
            HyperedgeArg arg;
            bool exist_query = false;
            bool predicate_query = false;
            if(parse_hyperedge_arg(cmd->arg_str, &arg, &exist_query, &predicate_query) < 0) {
                fprintf(stderr, "failed to parse edge argument \"%s\"\n", cmd->arg_str);
                break;
            }

            EdgeList ls = {0};
            bool has_result = do_search(g, arg.rank, arg.label, arg.nodes, exist_query, predicate_query, argd->params.no_hyperedge_order, argd->params.sort_result, &ls);

            if (exist_query)
            {
                printf("%d\n", has_result ? 1 : 0);
            }
            else
            {
                for(size_t i = 0; i < ls.len; i++) {
                    printf("(%" PRId64, ls.data[i].label);
                    for (CGraphRank j = 0; j < ls.data[i].rank; j++) {
                        printf(",\t%" PRId64, ls.data[i].nodes[j]);
                    }
                    printf(")\n");
                }

                if (argd->verbose)
                {
                    printf("Found %zu results", ls.len);
                }

                for (int i = 0; i < ls.len; i++)
                {
                    free(ls.data[i].nodes);
                }
                if(ls.data)
                    free(ls.data);

            }
            res = 0;
            break;
        }
		case CMD_NODE_COUNT:
			printf("%zu\n", cgraphr_node_count(g));
			res = 0;
			break;
		case CMD_EDGE_LABELS:
			printf("%zu\n", cgraphr_edge_label_count(g));
			res = 0;
			break;
#ifdef WEB_SERVICE
        case CMD_WEB_SERVICE:
            do_webservice(g, argd);
            goto exit;
            break;
#endif
		case CMD_NONE:
			goto exit;
		}	
	}

exit:
	cgraphr_destroy(g);
	return res;
}

int main(int argc, char** argv) {
	if(argc <= 1) {
		print_usage(true);
		return EXIT_FAILURE;
	}

	CGraphArgs argd;
	int arg_indx = parse_args(argc, argv, &argd); // ignore first program arg
	if(arg_indx < 0)
		return EXIT_FAILURE;

	char** cmd_argv = argv + arg_indx;
	int cmd_argc = argc - arg_indx;

	if(argd.mode == -1) // unknown mode, determine by the number of arguments
		argd.mode = cmd_argc == 2 ? 0 : 1;
	if(argd.mode == 0) {
		if(cmd_argc != 2) {
			fprintf(stderr, "expected 2 parameters when compressing RDF graphs\n");
			return EXIT_FAILURE;
		}

		if(do_compress(cmd_argv[0], cmd_argv[1], &argd) < 0)
			return EXIT_FAILURE;
	}
	else {
		if(cmd_argc != 1) {
			fprintf(stderr, "expected 1 parameter when reading compressed RDF graphs\n");
			return EXIT_FAILURE;
		}

		if(do_read(cmd_argv[0], &argd) < 0)
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
