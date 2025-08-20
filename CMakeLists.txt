cmake_minimum_required (VERSION 4.1.0)
project (libaux)

include (CheckSymbolExists)

include_directories (/usr/include /usr/local/include SYSTEM)
include_directories (include)

link_directories (/usr/lib /usr/local/lib)

set (CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")

add_compile_options (-D_GNU_SOURCE --std=c99 -Wall)

file (GLOB_RECURSE SOURCE_FILES src/*.c)
add_library (aux SHARED ${SOURCE_FILES})

add_subdirectory (test)
