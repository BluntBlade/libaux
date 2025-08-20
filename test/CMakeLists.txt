include_directories (../src)

file (GLOB_RECURSE TEST_ASCII_SOURCE_FILES str/ascii.c)
add_executable (ascii ${TEST_ASCII_SOURCE_FILES})
target_link_libraries (ascii criterion)

file (GLOB_RECURSE TEST_UTF8_SOURCE_FILES str/utf8.c)
add_executable (utf8 ${TEST_UTF8_SOURCE_FILES})
target_link_libraries (utf8 criterion)
