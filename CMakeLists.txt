cmake_minimum_required(VERSION 3.16)

project(ModernPDE)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

file(GLOB SOURCES
    src/*.cpp
)

# Exclude main_phase8.cpp from main executable
list(FILTER SOURCES EXCLUDE REGEX "src/main_phase8.cpp")

add_executable(ModernPDE
    ${SOURCES}
    src/main.cpp
)

# Phase 8 Optimization executable
add_executable(ModernPDE_Phase8
    ${SOURCES}
    src/main_phase8.cpp
)

enable_testing()
add_subdirectory(tests)
