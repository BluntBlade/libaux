include_directories (../src)

file (GLOB_RECURSE TEST_ASCII_SOURCE_FILES str/ascii.c)
add_executable (ascii ${TEST_ASCII_SOURCE_FILES})
target_link_libraries (ascii criterion)
