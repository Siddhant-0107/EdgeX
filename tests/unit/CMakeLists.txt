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

add_executable(edgex_thread_pool_test
    thread_pool_test.cpp
)

target_link_libraries(edgex_thread_pool_test
    PRIVATE
        EdgeX::core
)

edgex_enable_warnings(edgex_thread_pool_test)

add_test(
    NAME thread_pool_test
    COMMAND edgex_thread_pool_test
)

add_executable(edgex_logger_test
    logger_test.cpp
)

target_link_libraries(edgex_logger_test
    PRIVATE
        EdgeX::core
)

edgex_enable_warnings(edgex_logger_test)

add_test(
    NAME logger_test
    COMMAND edgex_logger_test
)

add_executable(edgex_tcp_client_test
    tcp_client_test.cpp
)

target_link_libraries(edgex_tcp_client_test
    PRIVATE
        EdgeX::core
)

edgex_enable_warnings(edgex_tcp_client_test)

add_test(
    NAME tcp_client_test
    COMMAND edgex_tcp_client_test
)

add_executable(edgex_reverse_proxy_test
    reverse_proxy_test.cpp
)

target_link_libraries(edgex_reverse_proxy_test
    PRIVATE
        EdgeX::core
)

edgex_enable_warnings(edgex_reverse_proxy_test)

add_test(
    NAME reverse_proxy_test
    COMMAND edgex_reverse_proxy_test
)

add_executable(edgex_round_robin_load_balancer_test
    round_robin_load_balancer_test.cpp
)

target_link_libraries(edgex_round_robin_load_balancer_test
    PRIVATE
        EdgeX::core
)

edgex_enable_warnings(edgex_round_robin_load_balancer_test)

add_test(
    NAME round_robin_load_balancer_test
    COMMAND edgex_round_robin_load_balancer_test
)

add_executable(edgex_health_checker_test
    health_checker_test.cpp
)

target_link_libraries(edgex_health_checker_test
    PRIVATE
        EdgeX::core
)

edgex_enable_warnings(edgex_health_checker_test)

add_test(
    NAME health_checker_test
    COMMAND edgex_health_checker_test
)
