cmake_minimum_required(VERSION 3.16)
project(MiniGPU VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core compiler library
add_library(minigpu_lib
    src/frontend/token.cpp
    src/frontend/lexer.cpp
    src/frontend/ast.cpp
    src/frontend/parser.cpp
    src/semantic/semantic_analysis.cpp
    src/ir/ir.cpp
    src/ir/ir_builder.cpp
    src/ir/ir_verifier.cpp
    src/analysis/cfg.cpp
    src/analysis/liveness.cpp
    src/passes/optimizer.cpp
    src/backend/target.cpp
    src/backend/regalloc.cpp
    src/backend/codegen.cpp
    src/simulator/simulator.cpp
)

# MiniGPU CLI compiler executable
add_executable(minigpu src/driver/main.cpp)
target_link_libraries(minigpu PRIVATE minigpu_lib)

# Testing
enable_testing()

macro(add_minigpu_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE minigpu_lib)
    if(MSVC)
        target_compile_options(${test_name} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${test_name} PRIVATE -UNDEBUG)
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
endmacro()

add_minigpu_test(test_lexer tests/lexer/test_lexer.cpp)
add_minigpu_test(test_parser tests/parser/test_parser.cpp)
add_minigpu_test(test_semantic tests/semantic/test_semantic.cpp)
add_minigpu_test(test_ir tests/ir/test_ir.cpp)
add_minigpu_test(test_passes tests/passes/test_passes.cpp)
add_minigpu_test(test_regalloc tests/regalloc/test_regalloc.cpp)
add_minigpu_test(test_codegen tests/codegen/test_codegen.cpp)
add_minigpu_test(test_integration tests/integration/test_integration.cpp)
