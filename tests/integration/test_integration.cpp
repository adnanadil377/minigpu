#include "minigpu/lexer.hpp"
#include "minigpu/parser.hpp"
#include "minigpu/semantic_analysis.hpp"
#include "minigpu/ir_builder.hpp"
#include "minigpu/optimizer.hpp"
#include "minigpu/codegen.hpp"
#include "minigpu/simulator.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

std::unique_ptr<minigpu::IRModule> compile_source(const std::string& src) {
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    assert(!lexer.has_errors());

    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();
    assert(!parser.has_errors());

    minigpu::SemanticAnalyzer sema;
    bool sema_ok = sema.analyze(*ast);
    assert(sema_ok);

    minigpu::IRGen irgen;
    auto module = irgen.emit_module(*ast);

    minigpu::PassManager pm;
    pm.add_default_passes();
    pm.run(*module);

    return module;
}

void test_milestone_1_vector_add() {
    std::string src = "kernel vector_add(float* A, float* B, float* C, int N) {\n"
                      "  int i = thread_id_x();\n"
                      "  if (i < N) {\n"
                      "    C[i] = A[i] + B[i];\n"
                      "  }\n"
                      "}";
    auto module = compile_source(src);
    auto& fn = module->functions()[0];

    const int N = 256;
    std::vector<float> A(N);
    std::vector<float> B(N);
    std::vector<float> C(N, 0.0f);

    for (int i = 0; i < N; ++i) {
        A[i] = static_cast<float>(i) * 1.5f;
        B[i] = static_cast<float>(i) * 2.5f;
    }
    int n_param = N;

    std::vector<void*> host_ptrs = {A.data(), B.data(), C.data(), &n_param};
    std::vector<size_t> sizes = {N * sizeof(float), N * sizeof(float), N * sizeof(float), sizeof(int)};
    std::vector<bool> is_ptr = {true, true, true, false};

    minigpu::IRSimulator sim;
    bool ok = sim.launch(*fn, minigpu::Dim3(1, 1, 1), minigpu::Dim3(N, 1, 1), host_ptrs, sizes, is_ptr);
    assert(ok);

    // Verify against CPU reference
    for (int i = 0; i < N; ++i) {
        float expected = A[i] + B[i];
        assert(std::fabs(C[i] - expected) < 1e-5f);
    }
    std::cout << "[PASS] Milestone 1: Vector Addition end-to-end correctness\n";
}

void test_milestone_2_elementwise_scale() {
    std::string src = "kernel scale(float* A, float* B, float alpha, int N) {\n"
                      "  int i = thread_id_x();\n"
                      "  if (i < N) {\n"
                      "    B[i] = A[i] * alpha;\n"
                      "  }\n"
                      "}";
    auto module = compile_source(src);
    auto& fn = module->functions()[0];

    const int N = 128;
    std::vector<float> A(N);
    std::vector<float> B(N, 0.0f);
    float alpha = 3.5f;
    int n_param = N;

    for (int i = 0; i < N; ++i) {
        A[i] = static_cast<float>(i + 1);
    }

    std::vector<void*> host_ptrs = {A.data(), B.data(), &alpha, &n_param};
    std::vector<size_t> sizes = {N * sizeof(float), N * sizeof(float), sizeof(float), sizeof(int)};
    std::vector<bool> is_ptr = {true, true, false, false};

    minigpu::IRSimulator sim;
    bool ok = sim.launch(*fn, minigpu::Dim3(1, 1, 1), minigpu::Dim3(N, 1, 1), host_ptrs, sizes, is_ptr);
    assert(ok);

    for (int i = 0; i < N; ++i) {
        float expected = A[i] * alpha;
        assert(std::fabs(B[i] - expected) < 1e-5f);
    }
    std::cout << "[PASS] Milestone 2: Elementwise Scaling end-to-end correctness\n";
}

void test_milestone_3_matrix_multiply() {
    std::string src = "kernel matmul(float* A, float* B, float* C, int N) {\n"
                      "  int row = thread_id_y();\n"
                      "  int col = thread_id_x();\n"
                      "  if (row < N && col < N) {\n"
                      "    float sum = 0.0;\n"
                      "    for (int k = 0; k < N; k++) {\n"
                      "      sum += A[row * N + k] * B[k * N + col];\n"
                      "    }\n"
                      "    C[row * N + col] = sum;\n"
                      "  }\n"
                      "}";
    auto module = compile_source(src);
    auto& fn = module->functions()[0];

    const int N = 8;
    std::vector<float> A(N * N);
    std::vector<float> B(N * N);
    std::vector<float> C(N * N, 0.0f);
    int n_param = N;

    for (int i = 0; i < N * N; ++i) {
        A[i] = static_cast<float>(i % 5);
        B[i] = static_cast<float>((i * 2) % 7);
    }

    std::vector<void*> host_ptrs = {A.data(), B.data(), C.data(), &n_param};
    std::vector<size_t> sizes = {N * N * sizeof(float), N * N * sizeof(float), N * N * sizeof(float), sizeof(int)};
    std::vector<bool> is_ptr = {true, true, true, false};

    minigpu::IRSimulator sim;
    bool ok = sim.launch(*fn, minigpu::Dim3(1, 1, 1), minigpu::Dim3(N, N, 1), host_ptrs, sizes, is_ptr);
    assert(ok);

    // Compute CPU golden reference
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            float golden = 0.0f;
            for (int k = 0; k < N; ++k) {
                golden += A[r * N + k] * B[k * N + c];
            }
            float actual = C[r * N + c];
            assert(std::fabs(actual - golden) < 1e-4f);
        }
    }
    std::cout << "[PASS] Milestone 3: Matrix Multiplication (8x8) end-to-end correctness\n";
}

int main() {
    std::cout << "--- Running End-to-End Milestone Integration Tests ---\n";
    test_milestone_1_vector_add();
    test_milestone_2_elementwise_scale();
    test_milestone_3_matrix_multiply();
    std::cout << "All Integration tests passed successfully!\n";
    return 0;
}
