cmake_minimum_required (VERSION 3.0)
project (tuple VERSION 1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(visitors lambda_visitors.cpp)

target_compile_features(visitors
    PUBLIC
        cxx_auto_type
        cxx_variadic_templates
)
