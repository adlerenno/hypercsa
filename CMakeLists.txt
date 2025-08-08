cmake_minimum_required(VERSION 3.10)
project(hypercsa VERSION 1.0.0 DESCRIPTION "hypercsa compressing and querying undirected hypergraphs.")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17) # At least 17 for parallel support.
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Error on undefined references
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(CMAKE_SHARED_LINKER_FLAGS "-Wl")
else()
  set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")
endif()

# Allow support for large files
add_definitions(-D_LARGEFILE_SOURCE -D_LARGE_FILES -D_FILE_OFFSET_BITS=64)

# Possibility to enable -march=native to increase performance of popcount
option(OPTIMIZE_FOR_NATIVE "Build with -march=native and -mtune=native" ON)
option(CLI "Adds a command-line-interface executable" ON)
option(TRACE_SYMBOLS "Add the trace symbols for the panic function" OFF)
option(TRACK_MEMORY "Activates SDSLs Memory Manager during construction." OFF)
option(VERBOSE "Adds a few command line outputs." OFF)
option(VERBOSE_DEBUG "Adds much output to the command line, including whole results of steps" OFF)

# configure_file("include/cgraph.cmake.h" "${CMAKE_CURRENT_BINARY_DIR}/cgraph.h" @ONLY)

# get files with "find include src -type f -name '*.c' | sort"
set(SOURCES
        compress/compress.cpp
        compress/compress.hpp
        util/panic.c
        util/panic.h
        util/trap.h
        util/type_definitions.hpp
        read/read.cpp
        util/prints.hpp
        util/prints.cpp
        util/test.h
        util/test.cpp
        compress/parse.cpp
        compress/parse.hpp
        hypercsa.cpp
        hypercsa.h
)

set(INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        util
        compress
        read
        /usr/local/include
)


add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE sdsl divsufsort divsufsort64)
target_link_directories(${PROJECT_NAME} PRIVATE "/usr/local/lib")  # necessary on MAC to find libraries.

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES})

target_link_libraries(${PROJECT_NAME} PRIVATE m) # link with math library

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER ${CMAKE_CURRENT_BINARY_DIR}/hypercsa.h)

# build with -march=native if OPTIMIZE_FOR_NATIVE option set
if(OPTIMIZE_FOR_NATIVE)
  target_compile_options(${PROJECT_NAME} PRIVATE -march=native -mtune=native)
endif()

# adding the trace symbols
if(TRACE_SYMBOLS)
  target_link_options(${PROJECT_NAME} PRIVATE -rdynamic)
endif()

if (TRACK_MEMORY)
  add_definitions(-DTRACK_MEMORY)
endif()

if (VERBOSE)
  add_definitions(-DVERBOSE)
endif()

if (VERBOSE_DEBUG)
  add_definitions(-DVERBOSE_DEBUG)
endif()

# CLI
if(CLI)
  add_executable(${PROJECT_NAME}-cli
          main.cpp
  )
  add_dependencies(${PROJECT_NAME}-cli ${PROJECT_NAME}) # add library add dependency

  target_include_directories(${PROJECT_NAME}-cli PRIVATE ${INCLUDES})
  target_link_directories(${PROJECT_NAME}-cli PRIVATE "/usr/local/lib")  # necessary on MAC to find libraries.

  target_link_libraries(${PROJECT_NAME}-cli PRIVATE ${PROJECT_NAME})
  target_link_libraries(${PROJECT_NAME}-cli PRIVATE sdsl divsufsort divsufsort64)

  if(TRACE_SYMBOLS)
    target_link_options(${PROJECT_NAME}-cli PRIVATE -rdynamic)
  endif()
endif()
