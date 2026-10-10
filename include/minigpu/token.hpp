#pragma once

#include "minigpu/types.hpp"
#include <string>
#include <cstdint>

namespace minigpu {

enum class TokenKind {
    // End of file / error
    Eof,
    Invalid,

    // Keywords
    KwKernel,
    KwInt,
    KwFloat,
    KwBool,
    KwVoid,
    KwIf,
    KwElse,
    KwWhile,
    KwFor,
    KwReturn,
    KwBreak,
    KwContinue,

    // Built-ins
    KwThreadIdX,
    KwThreadIdY,
    KwThreadIdZ,
    KwBlockIdX,
    KwBlockIdY,
    KwBlockIdZ,
    KwBlockDimX,
    KwBlockDimY,
    KwBlockDimZ,
    KwGridDimX,
    KwGridDimY,
    KwGridDimZ,

    // Literals
    LiteralInt,
    LiteralFloat,
    LiteralBool,

    // Identifier
    Identifier,

    // Operators & Punctuation
    Plus,          // +
    PlusPlus,      // ++
    Minus,         // -
    MinusMinus,    // --
    Star,          // *
    Slash,         // /
    Percent,       // %

    Equal,         // =
    PlusEqual,     // +=
    MinusEqual,    // -=
    StarEqual,     // *=
    SlashEqual,    // /=

    EqualEqual,    // ==
    ExclaimEqual,  // !=
    Less,          // <
    LessEqual,     // <=
    Greater,       // >
    GreaterEqual,  // >=

    AmpAmp,        // &&
    PipePipe,      // ||
    Exclaim,       // !

    LParen,        // (
    RParen,        // )
    LBracket,      // [
    RBracket,      // ]
    LBrace,        // {
    RBrace,        // }
    Comma,         // ,
    Semicolon      // ;
};

const char* token_kind_to_string(TokenKind kind);

struct Token {
    TokenKind kind{TokenKind::Invalid};
    std::string text;
    SourceLocation loc;

    int64_t int_val{0};
    double float_val{0.0};
    bool bool_val{false};

    Token() = default;
    Token(TokenKind k, std::string t, SourceLocation l)
        : kind(k), text(std::move(t)), loc(std::move(l)) {}

    std::string to_string() const;
};

} // namespace minigpu
