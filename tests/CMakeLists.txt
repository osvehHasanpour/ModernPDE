cmake_minimum_required(VERSION 3.16)

set(MODERNPDE_INCLUDE "${CMAKE_SOURCE_DIR}/include")
set(MODERNPDE_SRC "${CMAKE_SOURCE_DIR}/src")

function(modernpde_test_executable TARGET)
    add_executable(${TARGET} ${ARGN})
    target_include_directories(
        ${TARGET}
        PRIVATE
        ${MODERNPDE_INCLUDE}
    )
    target_compile_options(
        ${TARGET}
        PRIVATE
        -Wall
        -Wextra
    )
endfunction()

function(modernpde_add_test TARGET)
    add_test(
        NAME ${TARGET}
        COMMAND ${TARGET}
    )
    set_tests_properties(
        ${TARGET}
        PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endfunction()

# ---------------------------------------------------------------------------
# Phase 8 Tests
# ---------------------------------------------------------------------------

modernpde_test_executable(
    phase8_test
    phase8_test.cpp
    ${MODERNPDE_SRC}/Phase8Optimizer.cpp
    ${MODERNPDE_SRC}/CFGCache.cpp
    ${MODERNPDE_SRC}/DUAnalysisCache.cpp
    ${MODERNPDE_SRC}/WorklistDataflow.cpp
    ${MODERNPDE_SRC}/DominatorAnalysis.cpp
    ${MODERNPDE_SRC}/PathEnumerationLazy.cpp
    ${MODERNPDE_SRC}/CFG.cpp
    ${MODERNPDE_SRC}/DUChain.cpp
    ${MODERNPDE_SRC}/PDE.cpp
)
modernpde_add_test(phase8_test)

# ---------------------------------------------------------------------------
# New regression tests
# ---------------------------------------------------------------------------

modernpde_test_executable(
    field_sensitive_test
    field_sensitive_test.cpp
    ${MODERNPDE_SRC}/FieldSensitiveAnalysis.cpp
)
modernpde_add_test(field_sensitive_test)

modernpde_test_executable(
    lexer_parser_test
    lexer_parser_test.cpp
    ${MODERNPDE_SRC}/Lexer.cpp
    ${MODERNPDE_SRC}/Parser.cpp
    ${MODERNPDE_SRC}/CFGBuilder.cpp
    ${MODERNPDE_SRC}/CFG.cpp
)
modernpde_add_test(lexer_parser_test)

modernpde_test_executable(
    callgraph_test
    callgraph_test.cpp
    ${MODERNPDE_SRC}/CallGraph.cpp
)
modernpde_add_test(callgraph_test)

modernpde_test_executable(
    cha_virtual_test
    cha_virtual_test.cpp
    ${MODERNPDE_SRC}/ClassHierarchy.cpp
    ${MODERNPDE_SRC}/VirtualCallAnalysis.cpp
)
modernpde_add_test(cha_virtual_test)

modernpde_test_executable(
    cfg_path_test
    cfg_path_test.cpp
    ${MODERNPDE_SRC}/CFG.cpp
    ${MODERNPDE_SRC}/DUChain.cpp
    ${MODERNPDE_SRC}/PathEnumeration.cpp
)
modernpde_add_test(cfg_path_test)

modernpde_test_executable(
    cfg_phase5_test
    cfg_phase5_test.cpp
    ${MODERNPDE_SRC}/Lexer.cpp
    ${MODERNPDE_SRC}/Parser.cpp
    ${MODERNPDE_SRC}/CFGBuilder.cpp
    ${MODERNPDE_SRC}/CFG.cpp
    ${MODERNPDE_SRC}/DUChain.cpp
    ${MODERNPDE_SRC}/PathEnumeration.cpp
    ${MODERNPDE_SRC}/CFGValidation.cpp
    ${MODERNPDE_SRC}/CFGStatistics.cpp
    ${MODERNPDE_SRC}/CFGPDE.cpp
    ${MODERNPDE_SRC}/PDE.cpp
)
modernpde_add_test(cfg_phase5_test)

# ---------------------------------------------------------------------------
# Existing challenge / integration tests
# ---------------------------------------------------------------------------

modernpde_test_executable(
    challenge_500
    challenge_500.cpp
    ${MODERNPDE_SRC}/PDE.cpp
    ${MODERNPDE_SRC}/Metrics.cpp
)
modernpde_add_test(challenge_500)

modernpde_test_executable(
    challenge_5000
    challenge_5000.cpp
    ${MODERNPDE_SRC}/PDE.cpp
    ${MODERNPDE_SRC}/Metrics.cpp
)
modernpde_add_test(challenge_5000)

modernpde_test_executable(
    challenge_50000
    challenge_50000.cpp
    ${MODERNPDE_SRC}/PDE.cpp
    ${MODERNPDE_SRC}/Metrics.cpp
)
modernpde_add_test(challenge_50000)

modernpde_test_executable(
    master_challenge
    master_challenge.cpp
    ${MODERNPDE_SRC}/PDE.cpp
)
modernpde_add_test(master_challenge)

modernpde_test_executable(
    full_master_challenge
    full_master_challenge.cpp
    ${MODERNPDE_SRC}/PDE.cpp
    ${MODERNPDE_SRC}/ClassHierarchy.cpp
    ${MODERNPDE_SRC}/VirtualCallAnalysis.cpp
    ${MODERNPDE_SRC}/CallGraph.cpp
)
modernpde_add_test(full_master_challenge)

# ---------------------------------------------------------------------------
# Optional external-library tests
# ---------------------------------------------------------------------------

set(CJSON_DIR "${CMAKE_SOURCE_DIR}/../cJSON-master")
set(TINYEXPR_DIR "${CMAKE_SOURCE_DIR}/../tinyexpr-master")

if(EXISTS "${CJSON_DIR}/cJSON.c")
    add_library(modernpde_cjson OBJECT "${CJSON_DIR}/cJSON.c")
    target_include_directories(
        modernpde_cjson
        PUBLIC
        ${CJSON_DIR}
    )

    modernpde_test_executable(
        cjson_pde_test
        cjson_pde_test.cpp
        ${MODERNPDE_SRC}/PDE.cpp
        ${MODERNPDE_SRC}/Metrics.cpp
        $<TARGET_OBJECTS:modernpde_cjson>
    )
    target_include_directories(
        cjson_pde_test
        PRIVATE
        ${CJSON_DIR}
    )
    target_link_libraries(
        cjson_pde_test
        m
    )
    modernpde_add_test(cjson_pde_test)
else()
    message(STATUS "cJSON not found at ${CJSON_DIR} — skipping cjson_pde_test")
endif()

if(EXISTS "${TINYEXPR_DIR}/tinyexpr.c")
    add_library(modernpde_tinyexpr OBJECT "${TINYEXPR_DIR}/tinyexpr.c")
    target_include_directories(
        modernpde_tinyexpr
        PUBLIC
        ${TINYEXPR_DIR}
    )

    modernpde_test_executable(
        tinyexpr_pde_test
        tinyexpr_pde_test.cpp
        ${MODERNPDE_SRC}/PDE.cpp
        ${MODERNPDE_SRC}/Metrics.cpp
        $<TARGET_OBJECTS:modernpde_tinyexpr>
    )
    target_include_directories(
        tinyexpr_pde_test
        PRIVATE
        ${TINYEXPR_DIR}
    )
    target_link_libraries(
        tinyexpr_pde_test
        m
    )
    modernpde_add_test(tinyexpr_pde_test)
else()
    message(STATUS "tinyexpr not found at ${TINYEXPR_DIR} — skipping tinyexpr_pde_test")
endif()
