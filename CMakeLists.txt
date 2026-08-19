cmake_minimum_required(VERSION 3.21)

project(EdgeX VERSION 0.1.0 LANGUAGES CXX)

option(EDGEX_BUILD_TESTS "Build EdgeX tests" ON)
option(EDGEX_BUILD_BENCHMARKS "Build EdgeX benchmarks" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/CompilerWarnings.cmake)

add_library(edgex_core STATIC
    src/common/application.cpp
    src/common/version.cpp
    src/net/network_module.cpp
    src/net/socket.cpp
    src/net/tcp_server.cpp
    src/http/http_module.cpp
)

add_library(EdgeX::core ALIAS edgex_core)

target_include_directories(edgex_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(edgex_core PUBLIC cxx_std_17)

if(WIN32)
    target_link_libraries(edgex_core PUBLIC ws2_32)
endif()

edgex_enable_warnings(edgex_core)

add_executable(edgex src/main.cpp)
target_link_libraries(edgex PRIVATE EdgeX::core)
edgex_enable_warnings(edgex)

if(EDGEX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(EDGEX_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
