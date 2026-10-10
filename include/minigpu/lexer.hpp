#pragma once

#include "minigpu/token.hpp"
#include <string>
#include <vector>
#include <string_view>

namespace minigpu {

struct Diagnostic {
    SourceLocation loc;
    std::string message;
    bool is_error{true};

    std::string to_string() const {
        return loc.to_string() + ": " + (is_error ? "error: " : "warning: ") + message;
    }
};

class Lexer {
public:
    Lexer(std::string source, std::string filename = "<stdin>");

    /// Tokenizes the entire source and returns the tokens.
    std::vector<Token> tokenize();

    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }
    bool has_errors() const { return has_errors_; }

private:
    char peek() const;
    char peek_next() const;
    char advance();
    bool match(char expected);
    bool is_at_end() const;

    void skip_whitespace_and_comments();
    Token scan_token();
    Token scan_identifier_or_keyword();
    Token scan_number();

    void add_error(const SourceLocation& loc, const std::string& msg);

    std::string source_;
    std::string filename_;
    size_t cursor_{0};
    int line_{1};
    int column_{1};

    std::vector<Diagnostic> diagnostics_;
    bool has_errors_{false};
};

} // namespace minigpu
