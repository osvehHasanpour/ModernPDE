cmake_minimum_required(VERSION 3.16)

project(ModernPDE)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

file(GLOB SOURCES
    src/*.cpp
)

# Exclude both mains from the shared list (avoids duplicate main at link time)
list(FILTER SOURCES EXCLUDE REGEX "src/main\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX "src/main_phase8\\.cpp$")

add_executable(ModernPDE
    ${SOURCES}
    src/main.cpp
)

add_executable(ModernPDE_Phase8
    ${SOURCES}
    src/main_phase8.cpp
)

enable_testing()
add_subdirectory(tests)
