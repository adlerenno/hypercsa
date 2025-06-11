# hypercsa

This repository contains the code to the paper `Compressing Hypergraphs using Suffix Sorting`. 
The paper is available at https://www.arxiv.org/abs/2506.05023. 
To reproduce the tests of the mentioned paper, please use the snakemake code in https://github.com/adlerenno/hypercsa-test.

# Compile

This is a CMake project, so you need `cmake` installed. 
The minimum c++ version is 17. 
You need the SDSL library installed (https://github.com/simongog/sdsl-lite).
Clone this repository and then run the normal cmake build:

```bash
mkdir -p build
cd build
cmake ..
make
```

There are the following options available:

- OPTIMIZE_FOR_NATIVE "Build with -march=native and -mtune=native" (Default: ON)

- TRACE_SYMBOLS "Add the trace symbols for the panic function" (Default: OFF)

- CLI "Adds a command-line-interface executable" (Default: ON)

- TRACK_MEMORY "Activates SDSLs Memory Manager during construction." (Default: OFF)

- VERBOSE "Adds a few command line outputs." (Default: OFF)

- VERBOSE_DEBUG "Adds much output to the command line, including whole results of steps" (Default: OFF)

For example, `-DVERBOSE=on` in `cmake -DVERBOSE=on ..`enables the command line outputs.

# CLI

```
Usage: hypercsa-cli
   -h,                                       show this help
   -i [input] -o [output]                    to compress a hypergraph. hypergraph format is one edge per line, nodes are integers and comma separated
   -i [input] -t [type] -q [list of nodes]   evaluate query on compressed hypergraph. query is a comma-separated-list of integers
   -i [input] -t [type] -f [queryfile]       evaluates all queries in the file, one query per line.
                                             Type 0 is exists query, Type 1 is contains query.
```

# Library

The current header file is `hypercsa.h`. The library will be always build by cmake. 
If you need different methods in the header for your application, please ask or open a request in this repository.