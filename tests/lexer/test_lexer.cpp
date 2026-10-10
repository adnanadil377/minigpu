#include "minigpu/lexer.hpp"
#include <cassert>
#include <iostream>

void test_basic_tokens() {
    std::string src = "kernel test(float* A, int N) { int i = 42; float f = 3.14; }";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();

    assert(!lexer.has_errors());
    assert(tokens.size() > 0);
    assert(tokens[0].kind == minigpu::TokenKind::KwKernel);
    assert(tokens[1].kind == minigpu::TokenKind::Identifier && tokens[1].text == "test");
    std::cout << "[PASS] test_basic_tokens\n";
}

void test_numbers() {
    std::string src = "42 0x2A 3.14 1.5e2 2.0f";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();

    assert(!lexer.has_errors());
    assert(tokens[0].kind == minigpu::TokenKind::LiteralInt && tokens[0].int_val == 42);
    assert(tokens[1].kind == minigpu::TokenKind::LiteralInt && tokens[1].int_val == 42);
    assert(tokens[2].kind == minigpu::TokenKind::LiteralFloat);
    assert(tokens[3].kind == minigpu::TokenKind::LiteralFloat);
    assert(tokens[4].kind == minigpu::TokenKind::LiteralFloat);
    std::cout << "[PASS] test_numbers\n";
}

void test_comments() {
    std::string src = "// single line comment\n/* multi\nline\ncomment */ int x = 10;";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();

    assert(!lexer.has_errors());
    assert(tokens[0].kind == minigpu::TokenKind::KwInt);
    assert(tokens[1].kind == minigpu::TokenKind::Identifier && tokens[1].text == "x");
    assert(tokens[2].kind == minigpu::TokenKind::Equal);
    assert(tokens[3].kind == minigpu::TokenKind::LiteralInt && tokens[3].int_val == 10);
    std::cout << "[PASS] test_comments\n";
}

void test_invalid_tokens() {
    std::string src = "@ # $";
    minigpu::Lexer lexer(src);
    auto tokens = lexer.tokenize();

    assert(lexer.has_errors());
    assert(lexer.diagnostics().size() >= 3);
    std::cout << "[PASS] test_invalid_tokens\n";
}

int main() {
    std::cout << "--- Running Lexer Tests ---\n";
    test_basic_tokens();
    test_numbers();
    test_comments();
    test_invalid_tokens();
    std::cout << "All Lexer tests passed!\n";
    return 0;
}
