#include "minigpu/token.hpp"
#include <sstream>

namespace minigpu {

const char* token_kind_to_string(TokenKind kind) {
    switch (kind) {
        case TokenKind::Eof: return "EOF";
        case TokenKind::Invalid: return "INVALID";
        case TokenKind::KwKernel: return "kernel";
        case TokenKind::KwInt: return "int";
        case TokenKind::KwFloat: return "float";
        case TokenKind::KwBool: return "bool";
        case TokenKind::KwVoid: return "void";
        case TokenKind::KwIf: return "if";
        case TokenKind::KwElse: return "else";
        case TokenKind::KwWhile: return "while";
        case TokenKind::KwFor: return "for";
        case TokenKind::KwReturn: return "return";
        case TokenKind::KwBreak: return "break";
        case TokenKind::KwContinue: return "continue";
        case TokenKind::KwThreadIdX: return "thread_id_x";
        case TokenKind::KwThreadIdY: return "thread_id_y";
        case TokenKind::KwThreadIdZ: return "thread_id_z";
        case TokenKind::KwBlockIdX: return "block_id_x";
        case TokenKind::KwBlockIdY: return "block_id_y";
        case TokenKind::KwBlockIdZ: return "block_id_z";
        case TokenKind::KwBlockDimX: return "block_dim_x";
        case TokenKind::KwBlockDimY: return "block_dim_y";
        case TokenKind::KwBlockDimZ: return "block_dim_z";
        case TokenKind::KwGridDimX: return "grid_dim_x";
        case TokenKind::KwGridDimY: return "grid_dim_y";
        case TokenKind::KwGridDimZ: return "grid_dim_z";
        case TokenKind::LiteralInt: return "INT_LIT";
        case TokenKind::LiteralFloat: return "FLOAT_LIT";
        case TokenKind::LiteralBool: return "BOOL_LIT";
        case TokenKind::Identifier: return "IDENTIFIER";
        case TokenKind::Plus: return "+";
        case TokenKind::PlusPlus: return "++";
        case TokenKind::Minus: return "-";
        case TokenKind::MinusMinus: return "--";
        case TokenKind::Star: return "*";
        case TokenKind::Slash: return "/";
        case TokenKind::Percent: return "%";
        case TokenKind::Equal: return "=";
        case TokenKind::PlusEqual: return "+=";
        case TokenKind::MinusEqual: return "-=";
        case TokenKind::StarEqual: return "*=";
        case TokenKind::SlashEqual: return "/=";
        case TokenKind::EqualEqual: return "==";
        case TokenKind::ExclaimEqual: return "!=";
        case TokenKind::Less: return "<";
        case TokenKind::LessEqual: return "<=";
        case TokenKind::Greater: return ">";
        case TokenKind::GreaterEqual: return ">=";
        case TokenKind::AmpAmp: return "&&";
        case TokenKind::PipePipe: return "||";
        case TokenKind::Exclaim: return "!";
        case TokenKind::LParen: return "(";
        case TokenKind::RParen: return ")";
        case TokenKind::LBracket: return "[";
        case TokenKind::RBracket: return "]";
        case TokenKind::LBrace: return "{";
        case TokenKind::RBrace: return "}";
        case TokenKind::Comma: return ",";
        case TokenKind::Semicolon: return ";";
    }
    return "UNKNOWN";
}

std::string Token::to_string() const {
    std::ostringstream ss;
    ss << loc.to_string() << ": " << token_kind_to_string(kind) << " '" << text << "'";
    if (kind == TokenKind::LiteralInt) {
        ss << " (" << int_val << ")";
    } else if (kind == TokenKind::LiteralFloat) {
        ss << " (" << float_val << ")";
    } else if (kind == TokenKind::LiteralBool) {
        ss << " (" << (bool_val ? "true" : "false") << ")";
    }
    return ss.str();
}

} // namespace minigpu
