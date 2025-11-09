include_directories (../src)

add_link_options (-lcriterion)

file (GLOB_RECURSE MISC_SOURCE_FILES str/misc.c)
add_executable (misc.exe ${MISC_SOURCE_FILES})

file (GLOB_RECURSE ASCII_SOURCE_FILES str/ascii.c)
add_executable (ascii.exe ${ASCII_SOURCE_FILES})

file (GLOB_RECURSE UTF8_SOURCE_FILES str/utf8.c)
add_executable (utf8.exe ${UTF8_SOURCE_FILES})

file (GLOB_RECURSE NSTR_SOURCE_FILES str/nstr.c ../src/str/ascii.c ../src/str/utf8.c ../src/str/misc.c)
add_executable (nstr.exe ${NSTR_SOURCE_FILES})
