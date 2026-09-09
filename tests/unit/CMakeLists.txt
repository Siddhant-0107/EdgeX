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

add_executable(edgex_router_test
    router_test.cpp
)

target_link_libraries(edgex_router_test
    PRIVATE
        EdgeX::core
)

edgex_enable_warnings(edgex_router_test)

add_test(
    NAME router_test
    COMMAND edgex_router_test
)

add_executable(edgex_static_file_server_test
    static_file_server_test.cpp
)

target_link_libraries(edgex_static_file_server_test
    PRIVATE
        EdgeX::core
)

target_compile_definitions(edgex_static_file_server_test
    PRIVATE
        EDGEX_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

edgex_enable_warnings(edgex_static_file_server_test)

add_test(
    NAME static_file_server_test
    COMMAND edgex_static_file_server_test
)