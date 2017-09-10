FLEX_TARGET(TokenLexer ${CMAKE_SOURCE_DIR}/src/token.flex ${CMAKE_CURRENT_BINARY_DIR}/token-lexer.cpp COMPILE_FLAGS "-d")

if (CMAKE_VERSION VERSION_GREATER "3.4")
    BISON_TARGET(CoolParser cool.y ${CMAKE_CURRENT_BINARY_DIR}/cool-parser.cpp
            COMPILE_FLAGS "-v -y -b cool --debug -p cool_yy"
            DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/cool-parser.hpp")
else()
    BISON_TARGET(CoolParser cool.y ${CMAKE_CURRENT_BINARY_DIR}/cool-parser.cpp
            COMPILE_FLAGS "-v -y -b cool --debug -p cool_yy"
            HEADER "${CMAKE_CURRENT_BINARY_DIR}/cool-parser.hpp")
endif()

ADD_FLEX_BISON_DEPENDENCY(TokenLexer CoolParser)

# Flex-generated code uses deprecated features
set_source_files_properties(
    ${FLEX_TokenLexer_OUTPUTS}
    PROPERTIES
    COMPILE_FLAGS -Wno-deprecated-register
)

add_executable(parser
    parser-main.cc
    ${FLEX_TokenLexer_OUTPUTS}
    ${BISON_CoolParser_OUTPUTS}
    $<TARGET_OBJECTS:cool_objs>
)