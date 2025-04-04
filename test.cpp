//
// Created by Enno Adler on 03.04.25.
//

#include "compress/compress.h"
#include "read/read.h"

int main(int argc, char** argv) {
    construct_hypercsa("", "test.hcsa");
    test_query("test.hcsa");
    return 0;
}