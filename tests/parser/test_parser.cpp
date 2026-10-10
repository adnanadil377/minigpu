#include "minigpu/lexer.hpp"
#include "minigpu/parser.hpp"
#include <cassert>
#include <iostream>

void test_parse_kernel() {
    std::string src = "kernel k1(float* A, int N) { int i = 0; if (i < N) { A[i] = 1.0; } }";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    assert(!lexer.has_errors());

    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();

    assert(!parser.has_errors());
    assert(ast != nullptr);
    assert(ast->kernels().size() == 1);
    assert(ast->kernels()[0]->name() == "k1");
    assert(ast->kernels()[0]->params().size() == 2);
    std::cout << "[PASS] test_parse_kernel\n";
}

void test_parse_loops() {
    std::string src = "kernel loop_k(int N) {\n"
                      "  for (int i = 0; i < N; i++) { if (i == 5) break; }\n"
                      "  while (N > 0) { N -= 1; }\n"
                      "}";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();

    assert(!parser.has_errors());
    assert(ast->kernels()[0]->body().statements().size() == 2);
    std::cout << "[PASS] test_parse_loops\n";
}

void test_parse_precedence() {
    std::string src = "kernel prec() { int x = 1 + 2 * 3; }";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();

    assert(!parser.has_errors());
    std::cout << "[PASS] test_parse_precedence\n";
}

void test_syntax_errors() {
    std::string src = "kernel missing_semi(int x) { int y = 5 }"; // missing semicolon
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();

    assert(parser.has_errors());
    std::cout << "[PASS] test_syntax_errors (correctly detected error)\n";
}

int main() {
    std::cout << "--- Running Parser Tests ---\n";
    test_parse_kernel();
    test_parse_loops();
    test_parse_precedence();
    test_syntax_errors();
    std::cout << "All Parser tests passed!\n";
    return 0;
}
