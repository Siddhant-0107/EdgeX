cmake_minimum_required(VERSION 3.16)

project(EdgeX VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(edgex_backend STATIC
    src/core/application.cpp
    src/network/network_module.cpp
    src/http/http_module.cpp
    src/utils/version.cpp
)

target_include_directories(edgex_backend
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(edgex_backend PUBLIC cxx_std_17)

add_executable(edgex
    src/main.cpp
)

target_link_libraries(edgex
    PRIVATE
        edgex_backend
)

enable_testing()
