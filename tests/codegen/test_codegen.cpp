#include "minigpu/lexer.hpp"
#include "minigpu/parser.hpp"
#include "minigpu/semantic_analysis.hpp"
#include "minigpu/ir_builder.hpp"
#include "minigpu/codegen.hpp"
#include <cassert>
#include <iostream>

void test_ptx_generation() {
    std::string src = "kernel vec_add(float* A, float* B, float* C, int N) {\n"
                      "  int i = thread_id_x();\n"
                      "  if (i < N) {\n"
                      "    C[i] = A[i] + B[i];\n"
                      "  }\n"
                      "}";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();
    minigpu::SemanticAnalyzer sema;
    bool sema_ok = sema.analyze(*ast);
    assert(sema_ok);

    minigpu::IRGen irgen;
    auto module = irgen.emit_module(*ast);

    minigpu::PTXCodeGen codegen(minigpu::TargetArch::SM_75);
    std::string ptx = codegen.generate(*module);

    assert(ptx.find(".version 7.0") != std::string::npos);
    assert(ptx.find(".target sm_75") != std::string::npos);
    assert(ptx.find(".entry vec_add") != std::string::npos);
    assert(ptx.find("%tid.x") != std::string::npos);
    assert(ptx.find("ld.global.f32") != std::string::npos);
    assert(ptx.find("st.global.f32") != std::string::npos);
    std::cout << "[PASS] test_ptx_generation\n";
}

int main() {
    std::cout << "--- Running Codegen Tests ---\n";
    test_ptx_generation();
    std::cout << "All Codegen tests passed!\n";
    return 0;
}
