//
// Created by Enno Adler on 02.04.25.
//

#ifndef HYPERCSA_READ_H
#define HYPERCSA_READ_H

#include "type_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif
int query(string input_file, Edge *nodes);
    int test_query(string input_file);
#ifdef __cplusplus
}
#endif

#endif //HYPERCSA_READ_H
