add_test(
    NAME lexer_integration_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/runner.sh ${CMAKE_CURRENT_SOURCE_DIR}/lexer
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer-test.sh
        -L "$<TARGET_FILE:lexer>"
)

# Use reference lexer by default. You can change the -L argument to use your lexer implementation
add_test(
    NAME parser_integration_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/runner.sh ${CMAKE_CURRENT_SOURCE_DIR}/parser
        ${CMAKE_CURRENT_SOURCE_DIR}/parser-test.sh
        -L "${CMAKE_SOURCE_DIR}/bin/lexer"
        -P "$<TARGET_FILE:parser>"
)