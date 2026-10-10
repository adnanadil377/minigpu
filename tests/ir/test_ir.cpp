#include "minigpu/lexer.hpp"
#include "minigpu/parser.hpp"
#include "minigpu/semantic_analysis.hpp"
#include "minigpu/ir_builder.hpp"
#include "minigpu/ir_verifier.hpp"
#include "minigpu/cfg.hpp"
#include <cassert>
#include <iostream>

void test_ir_generation_and_verifier() {
    std::string src = "kernel k(float* A, int N) { int i = thread_id_x(); if (i < N) { A[i] = A[i] + 1.0; } }";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();
    minigpu::SemanticAnalyzer sema;
    assert(sema.analyze(*ast));

    minigpu::IRGen irgen;
    auto module = irgen.emit_module(*ast);
    assert(module->functions().size() == 1);

    minigpu::IRVerifier verifier;
    bool valid = verifier.verify_module(*module);
    assert(valid);
    std::cout << "[PASS] test_ir_generation_and_verifier\n";
}

void test_cfg_reachability() {
    std::string src = "kernel k(int N) { if (N > 0) { return; } else { return; } int dead = 10; }";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();
    minigpu::SemanticAnalyzer sema;
    assert(sema.analyze(*ast));

    minigpu::IRGen irgen;
    auto module = irgen.emit_module(*ast);

    minigpu::CFG cfg(module->functions()[0].get());
    assert(cfg.reachable_blocks().size() > 0);
    std::cout << "[PASS] test_cfg_reachability\n";
}

int main() {
    std::cout << "--- Running IR Tests ---\n";
    test_ir_generation_and_verifier();
    test_cfg_reachability();
    std::cout << "All IR tests passed!\n";
    return 0;
}
