cmake_minimum_required(VERSION 3.16)

project(ModernPDE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

include_directories(include)

set(MODERNPDE_LIB_SOURCES
    src/Benchmark.cpp
    src/BenchmarkPhase8.cpp
    src/CFG.cpp
    src/CFGBuilder.cpp
    src/CFGCache.cpp
    src/CFGPDE.cpp
    src/CFGStatistics.cpp
    src/CFGValidation.cpp
    src/CallGraph.cpp
    src/ClassHierarchy.cpp
    src/DUAnalysisCache.cpp
    src/DUChain.cpp
    src/Dataflow.cpp
    src/DominatorAnalysis.cpp
    src/FaintAnalysis.cpp
    src/FieldSensitiveAnalysis.cpp
    src/IR.cpp
    src/Lexer.cpp
    src/Metrics.cpp
    src/ModernPDE.cpp
    src/PDE.cpp
    src/Parser.cpp
    src/PathEnumeration.cpp
    src/PathEnumerationLazy.cpp
    src/Phase8Optimizer.cpp
    src/SCCP.cpp
    src/Statistics.cpp
    src/VirtualCallAnalysis.cpp
    src/Worklist.cpp
    src/WorklistDataflow.cpp
)

add_executable(ModernPDE
    ${MODERNPDE_LIB_SOURCES}
    src/main.cpp
)

add_executable(ModernPDE_Phase8
    ${MODERNPDE_LIB_SOURCES}
    src/main_phase8.cpp
)

enable_testing()
add_subdirectory(tests)
