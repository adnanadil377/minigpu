#include "minigpu/lexer.hpp"
#include "minigpu/parser.hpp"
#include "minigpu/semantic_analysis.hpp"
#include <cassert>
#include <iostream>

void test_valid_program() {
    std::string src = "kernel k(float* A, int N) { int tid = thread_id_x(); if (tid < N) { A[tid] = 2.0; } }";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();

    minigpu::SemanticAnalyzer sema;
    bool ok = sema.analyze(*ast);
    assert(ok);
    assert(!sema.has_errors());
    std::cout << "[PASS] test_valid_program\n";
}

void test_undeclared_variable() {
    std::string src = "kernel k() { x = 10; }";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();

    minigpu::SemanticAnalyzer sema;
    bool ok = sema.analyze(*ast);
    assert(!ok);
    assert(sema.has_errors());
    std::cout << "[PASS] test_undeclared_variable (caught undeclared var)\n";
}

void test_type_mismatch() {
    std::string src = "kernel k(float* A) { A[0] = true; }"; // assigning bool to float array
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();

    minigpu::SemanticAnalyzer sema;
    bool ok = sema.analyze(*ast);
    assert(!ok);
    assert(sema.has_errors());
    std::cout << "[PASS] test_type_mismatch (caught incompatible type assignment)\n";
}

void test_break_outside_loop() {
    std::string src = "kernel k() { break; }";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();

    minigpu::SemanticAnalyzer sema;
    bool ok = sema.analyze(*ast);
    assert(!ok);
    assert(sema.has_errors());
    std::cout << "[PASS] test_break_outside_loop (caught illegal break)\n";
}

int main() {
    std::cout << "--- Running Semantic Analysis Tests ---\n";
    test_valid_program();
    test_undeclared_variable();
    test_type_mismatch();
    test_break_outside_loop();
    std::cout << "All Semantic Analysis tests passed!\n";
    return 0;
}
