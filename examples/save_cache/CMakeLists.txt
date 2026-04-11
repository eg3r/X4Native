cmake_minimum_required(VERSION 3.20)
project(x4native_save_cache)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(x4native_save_cache SHARED
    save_cache.cpp
    struct_verify.cpp
    dom_builder.cpp
)

target_include_directories(x4native_save_cache PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk
)

set_target_properties(x4native_save_cache PROPERTIES
    PREFIX ""
    OUTPUT_NAME "x4native_save_cache"
)
