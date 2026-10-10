#include "minigpu/lexer.hpp"
#include <cctype>
#include <unordered_map>
#include <cstdlib>

namespace minigpu {

static const std::unordered_map<std::string, TokenKind> kKeywords = {
    {"kernel", TokenKind::KwKernel},
    {"int", TokenKind::KwInt},
    {"float", TokenKind::KwFloat},
    {"bool", TokenKind::KwBool},
    {"void", TokenKind::KwVoid},
    {"if", TokenKind::KwIf},
    {"else", TokenKind::KwElse},
    {"while", TokenKind::KwWhile},
    {"for", TokenKind::KwFor},
    {"return", TokenKind::KwReturn},
    {"break", TokenKind::KwBreak},
    {"continue", TokenKind::KwContinue},
    {"thread_id_x", TokenKind::KwThreadIdX},
    {"thread_id_y", TokenKind::KwThreadIdY},
    {"thread_id_z", TokenKind::KwThreadIdZ},
    {"block_id_x", TokenKind::KwBlockIdX},
    {"block_id_y", TokenKind::KwBlockIdY},
    {"block_id_z", TokenKind::KwBlockIdZ},
    {"block_dim_x", TokenKind::KwBlockDimX},
    {"block_dim_y", TokenKind::KwBlockDimY},
    {"block_dim_z", TokenKind::KwBlockDimZ},
    {"grid_dim_x", TokenKind::KwGridDimX},
    {"grid_dim_y", TokenKind::KwGridDimY},
    {"grid_dim_z", TokenKind::KwGridDimZ},
    {"true", TokenKind::LiteralBool},
    {"false", TokenKind::LiteralBool}
};

Lexer::Lexer(std::string source, std::string filename)
    : source_(std::move(source)), filename_(std::move(filename)) {}

bool Lexer::is_at_end() const {
    return cursor_ >= source_.size();
}

char Lexer::peek() const {
    if (is_at_end()) return '\0';
    return source_[cursor_];
}

char Lexer::peek_next() const {
    if (cursor_ + 1 >= source_.size()) return '\0';
    return source_[cursor_ + 1];
}

char Lexer::advance() {
    if (is_at_end()) return '\0';
    char c = source_[cursor_++];
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (is_at_end() || source_[cursor_] != expected) return false;
    advance();
    return true;
}

void Lexer::add_error(const SourceLocation& loc, const std::string& msg) {
    diagnostics_.push_back({loc, msg, true});
    has_errors_ = true;
}

void Lexer::skip_whitespace_and_comments() {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek_next() == '/') {
            // Single-line comment
            advance(); // '/'
            advance(); // '/'
            while (!is_at_end() && peek() != '\n') {
                advance();
            }
        } else if (c == '/' && peek_next() == '*') {
            // Multi-line comment
            SourceLocation start_loc(filename_, line_, column_);
            advance(); // '/'
            advance(); // '*'
            bool closed = false;
            while (!is_at_end()) {
                if (peek() == '*' && peek_next() == '/') {
                    advance(); // '*'
                    advance(); // '/'
                    closed = true;
                    break;
                }
                advance();
            }
            if (!closed) {
                add_error(start_loc, "unterminated block comment");
                return;
            }
        } else {
            break;
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skip_whitespace_and_comments();
        if (is_at_end()) {
            tokens.push_back(Token(TokenKind::Eof, "", SourceLocation(filename_, line_, column_)));
            break;
        }
        tokens.push_back(scan_token());
    }
    return tokens;
}

Token Lexer::scan_token() {
    SourceLocation loc(filename_, line_, column_);
    char c = advance();

    // Identifiers and keywords
    if (std::isalpha(c) || c == '_') {
        cursor_--;
        column_--;
        return scan_identifier_or_keyword();
    }

    // Numbers
    if (std::isdigit(c) || (c == '.' && std::isdigit(peek()))) {
        cursor_--;
        column_--;
        return scan_number();
    }

    switch (c) {
        case '+': {
            if (match('+')) return Token(TokenKind::PlusPlus, "++", loc);
            if (match('=')) return Token(TokenKind::PlusEqual, "+=", loc);
            return Token(TokenKind::Plus, "+", loc);
        }
        case '-': {
            if (match('-')) return Token(TokenKind::MinusMinus, "--", loc);
            if (match('=')) return Token(TokenKind::MinusEqual, "-=", loc);
            return Token(TokenKind::Minus, "-", loc);
        }
        case '*': {
            if (match('=')) return Token(TokenKind::StarEqual, "*=", loc);
            return Token(TokenKind::Star, "*", loc);
        }
        case '/': {
            if (match('=')) return Token(TokenKind::SlashEqual, "/=", loc);
            return Token(TokenKind::Slash, "/", loc);
        }
        case '%': return Token(TokenKind::Percent, "%", loc);

        case '=': {
            if (match('=')) return Token(TokenKind::EqualEqual, "==", loc);
            return Token(TokenKind::Equal, "=", loc);
        }
        case '!': {
            if (match('=')) return Token(TokenKind::ExclaimEqual, "!=", loc);
            return Token(TokenKind::Exclaim, "!", loc);
        }
        case '<': {
            if (match('=')) return Token(TokenKind::LessEqual, "<=", loc);
            return Token(TokenKind::Less, "<", loc);
        }
        case '>': {
            if (match('=')) return Token(TokenKind::GreaterEqual, ">=", loc);
            return Token(TokenKind::Greater, ">", loc);
        }
        case '&': {
            if (match('&')) return Token(TokenKind::AmpAmp, "&&", loc);
            add_error(loc, "unrecognized single '&'; did you mean '&&'?");
            return Token(TokenKind::Invalid, "&", loc);
        }
        case '|': {
            if (match('|')) return Token(TokenKind::PipePipe, "||", loc);
            add_error(loc, "unrecognized single '|'; did you mean '||'?");
            return Token(TokenKind::Invalid, "|", loc);
        }

        case '(': return Token(TokenKind::LParen, "(", loc);
        case ')': return Token(TokenKind::RParen, ")", loc);
        case '[': return Token(TokenKind::LBracket, "[", loc);
        case ']': return Token(TokenKind::RBracket, "]", loc);
        case '{': return Token(TokenKind::LBrace, "{", loc);
        case '}': return Token(TokenKind::RBrace, "}", loc);
        case ',': return Token(TokenKind::Comma, ",", loc);
        case ';': return Token(TokenKind::Semicolon, ";", loc);

        default: {
            std::string s(1, c);
            add_error(loc, "unexpected character '" + s + "'");
            return Token(TokenKind::Invalid, s, loc);
        }
    }
}

Token Lexer::scan_identifier_or_keyword() {
    SourceLocation loc(filename_, line_, column_);
    size_t start = cursor_;
    while (!is_at_end() && (std::isalnum(peek()) || peek() == '_')) {
        advance();
    }
    std::string text = source_.substr(start, cursor_ - start);

    auto it = kKeywords.find(text);
    if (it != kKeywords.end()) {
        Token tok(it->second, text, loc);
        if (it->second == TokenKind::LiteralBool) {
            tok.bool_val = (text == "true");
        }
        return tok;
    }

    return Token(TokenKind::Identifier, text, loc);
}

Token Lexer::scan_number() {
    SourceLocation loc(filename_, line_, column_);
    size_t start = cursor_;
    bool is_float = false;

    // Check hex
    if (peek() == '0' && (peek_next() == 'x' || peek_next() == 'X')) {
        advance(); // '0'
        advance(); // 'x'
        while (!is_at_end() && std::isxdigit(peek())) {
            advance();
        }
        std::string text = source_.substr(start, cursor_ - start);
        Token tok(TokenKind::LiteralInt, text, loc);
        try {
            tok.int_val = std::stoll(text, nullptr, 16);
        } catch (...) {
            add_error(loc, "invalid hexadecimal integer literal: " + text);
        }
        return tok;
    }

    // Decimal digits
    while (!is_at_end() && std::isdigit(peek())) {
        advance();
    }

    // Fractional part
    if (peek() == '.' && std::isdigit(peek_next())) {
        is_float = true;
        advance(); // '.'
        while (!is_at_end() && std::isdigit(peek())) {
            advance();
        }
    }

    // Exponent part
    if (peek() == 'e' || peek() == 'E') {
        is_float = true;
        advance();
        if (peek() == '+' || peek() == '-') {
            advance();
        }
        if (!std::isdigit(peek())) {
            add_error(loc, "exponent requires at least one digit");
        }
        while (!is_at_end() && std::isdigit(peek())) {
            advance();
        }
    }

    // Float suffix 'f' or 'F'
    if (peek() == 'f' || peek() == 'F') {
        is_float = true;
        advance();
    }

    std::string text = source_.substr(start, cursor_ - start);
    if (is_float) {
        Token tok(TokenKind::LiteralFloat, text, loc);
        try {
            // Strip trailing f/F if present for std::stod
            std::string num_str = text;
            if (!num_str.empty() && (num_str.back() == 'f' || num_str.back() == 'F')) {
                num_str.pop_back();
            }
            tok.float_val = std::stod(num_str);
        } catch (...) {
            add_error(loc, "invalid floating-point literal: " + text);
        }
        return tok;
    } else {
        Token tok(TokenKind::LiteralInt, text, loc);
        try {
            tok.int_val = std::stoll(text);
        } catch (...) {
            add_error(loc, "integer literal out of range: " + text);
        }
        return tok;
    }
}

} // namespace minigpu
