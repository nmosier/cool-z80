BISON_TARGET(CoolParser ${CMAKE_SOURCE_DIR}/pa3/cool.y ${CMAKE_CURRENT_BINARY_DIR}/cool-parser.cpp COMPILE_FLAGS "-d -v -y -b cool --debug -p cool_yy")

include_directories(BISON_CoolParser_OUTPUT_HEADER )
add_library(cool_objs
    OBJECT
    stringtab.cc
    utilities.cc
    ast.cc
)