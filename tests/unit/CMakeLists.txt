add_executable(edgex_http_parser_test
    http_parser_test.cpp
)

target_link_libraries(edgex_http_parser_test
    PRIVATE
        EdgeX::core
)

edgex_enable_warnings(edgex_http_parser_test)

add_test(
    NAME http_parser_test
    COMMAND edgex_http_parser_test
)
add_executable(edgex_http_response_test
    http_response_test.cpp
)

target_link_libraries(edgex_http_response_test
    PRIVATE
        EdgeX::core
)

edgex_enable_warnings(edgex_http_response_test)

add_test(
    NAME http_response_test
    COMMAND edgex_http_response_test
)
