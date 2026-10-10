#include "minigpu/parser.hpp"
#include <iostream>

namespace minigpu {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

const Token& Parser::peek() const {
    if (current_ >= tokens_.size()) return tokens_.back();
    return tokens_[current_];
}

const Token& Parser::peek_next() const {
    if (current_ + 1 >= tokens_.size()) return tokens_.back();
    return tokens_[current_ + 1];
}

const Token& Parser::previous() const {
    if (current_ == 0) return tokens_.front();
    return tokens_[current_ - 1];
}

bool Parser::is_at_end() const {
    return peek().kind == TokenKind::Eof;
}

const Token& Parser::advance() {
    if (!is_at_end()) current_++;
    return previous();
}

bool Parser::check(TokenKind kind) const {
    if (is_at_end()) return false;
    return peek().kind == kind;
}

bool Parser::match(TokenKind kind) {
    if (check(kind)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::consume(TokenKind kind, const std::string& error_msg) {
    if (check(kind)) return advance();
    add_error(peek().loc, error_msg + " (got '" + peek().text + "')");
    return peek();
}

void Parser::add_error(const SourceLocation& loc, const std::string& msg) {
    diagnostics_.push_back({loc, msg, true});
    has_errors_ = true;
}

void Parser::synchronize() {
    advance();
    while (!is_at_end()) {
        if (previous().kind == TokenKind::Semicolon) return;
        switch (peek().kind) {
            case TokenKind::KwKernel:
            case TokenKind::KwIf:
            case TokenKind::KwWhile:
            case TokenKind::KwFor:
            case TokenKind::KwReturn:
            case TokenKind::KwInt:
            case TokenKind::KwFloat:
            case TokenKind::KwBool:
                return;
            default:
                advance();
                break;
        }
    }
}

std::unique_ptr<ProgramAST> Parser::parse_program() {
    std::vector<std::unique_ptr<KernelDecl>> kernels;
    while (!is_at_end()) {
        if (peek().kind == TokenKind::KwKernel) {
            auto k = parse_kernel();
            if (k) {
                kernels.push_back(std::move(k));
            } else {
                synchronize();
            }
        } else {
            add_error(peek().loc, "expected 'kernel' declaration at top level");
            advance();
        }
    }
    return std::make_unique<ProgramAST>(std::move(kernels));
}

std::unique_ptr<KernelDecl> Parser::parse_kernel() {
    SourceLocation loc = peek().loc;
    consume(TokenKind::KwKernel, "expected 'kernel'");

    if (!check(TokenKind::Identifier)) {
        add_error(peek().loc, "expected kernel name");
        return nullptr;
    }
    std::string name = advance().text;

    consume(TokenKind::LParen, "expected '(' after kernel name");

    std::vector<ParamDecl> params;
    if (!check(TokenKind::RParen)) {
        do {
            params.push_back(parse_param());
        } while (match(TokenKind::Comma));
    }

    consume(TokenKind::RParen, "expected ')' after parameter list");

    auto body = parse_block();
    if (!body) return nullptr;

    return std::make_unique<KernelDecl>(name, std::move(params), std::move(body), loc);
}

ParamDecl Parser::parse_param() {
    SourceLocation loc = peek().loc;
    auto type = parse_type();
    std::string name;
    if (check(TokenKind::Identifier)) {
        name = advance().text;
    } else {
        add_error(peek().loc, "expected parameter name");
    }
    return ParamDecl(type, name, loc);
}

std::shared_ptr<Type> Parser::parse_type() {
    std::shared_ptr<Type> base_type = nullptr;
    if (match(TokenKind::KwInt)) {
        base_type = Type::getInt();
    } else if (match(TokenKind::KwFloat)) {
        base_type = Type::getFloat();
    } else if (match(TokenKind::KwBool)) {
        base_type = Type::getBool();
    } else if (match(TokenKind::KwVoid)) {
        base_type = Type::getVoid();
    } else {
        add_error(peek().loc, "expected type specifier (int, float, bool, void)");
        base_type = Type::getInt(); // fallback
    }

    while (match(TokenKind::Star)) {
        base_type = Type::getPointer(base_type);
    }
    return base_type;
}

std::unique_ptr<BlockStmt> Parser::parse_block() {
    SourceLocation loc = peek().loc;
    consume(TokenKind::LBrace, "expected '{'");

    std::vector<std::unique_ptr<StmtAST>> stmts;
    while (!check(TokenKind::RBrace) && !is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            stmts.push_back(std::move(stmt));
        } else {
            synchronize();
        }
    }

    consume(TokenKind::RBrace, "expected '}' after block");
    return std::make_unique<BlockStmt>(std::move(stmts), loc);
}

std::unique_ptr<StmtAST> Parser::parse_statement() {
    if (check(TokenKind::LBrace)) {
        return parse_block();
    }
    if (match(TokenKind::KwIf)) {
        return parse_if();
    }
    if (match(TokenKind::KwWhile)) {
        return parse_while();
    }
    if (match(TokenKind::KwFor)) {
        return parse_for();
    }
    if (match(TokenKind::KwReturn)) {
        return parse_return();
    }
    if (match(TokenKind::KwBreak)) {
        SourceLocation loc = previous().loc;
        consume(TokenKind::Semicolon, "expected ';' after 'break'");
        return std::make_unique<BreakStmt>(loc);
    }
    if (match(TokenKind::KwContinue)) {
        SourceLocation loc = previous().loc;
        consume(TokenKind::Semicolon, "expected ';' after 'continue'");
        return std::make_unique<ContinueStmt>(loc);
    }

    // Check variable declaration
    if (check(TokenKind::KwInt) || check(TokenKind::KwFloat) ||
        check(TokenKind::KwBool) || check(TokenKind::KwVoid)) {
        return parse_var_decl();
    }

    return parse_expr_statement();
}

std::unique_ptr<VarDeclStmt> Parser::parse_var_decl() {
    SourceLocation loc = peek().loc;
    auto type = parse_type();

    if (!check(TokenKind::Identifier)) {
        add_error(peek().loc, "expected variable name");
        return nullptr;
    }
    std::string name = advance().text;

    std::unique_ptr<ExprAST> init = nullptr;
    if (match(TokenKind::Equal)) {
        init = parse_expression();
    }

    consume(TokenKind::Semicolon, "expected ';' after variable declaration");
    return std::make_unique<VarDeclStmt>(type, name, std::move(init), loc);
}

std::unique_ptr<IfStmt> Parser::parse_if() {
    SourceLocation loc = previous().loc;
    consume(TokenKind::LParen, "expected '(' after 'if'");
    auto cond = parse_expression();
    consume(TokenKind::RParen, "expected ')' after if condition");

    auto then_branch = parse_statement();
    std::unique_ptr<StmtAST> else_branch = nullptr;
    if (match(TokenKind::KwElse)) {
        else_branch = parse_statement();
    }

    return std::make_unique<IfStmt>(std::move(cond), std::move(then_branch), std::move(else_branch), loc);
}

std::unique_ptr<WhileStmt> Parser::parse_while() {
    SourceLocation loc = previous().loc;
    consume(TokenKind::LParen, "expected '(' after 'while'");
    auto cond = parse_expression();
    consume(TokenKind::RParen, "expected ')' after while condition");

    auto body = parse_statement();
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body), loc);
}

std::unique_ptr<ForStmt> Parser::parse_for() {
    SourceLocation loc = previous().loc;
    consume(TokenKind::LParen, "expected '(' after 'for'");

    // Init
    std::unique_ptr<StmtAST> init = nullptr;
    if (!check(TokenKind::Semicolon)) {
        if (check(TokenKind::KwInt) || check(TokenKind::KwFloat) ||
            check(TokenKind::KwBool) || check(TokenKind::KwVoid)) {
            init = parse_var_decl();
        } else {
            auto expr = parse_expression();
            consume(TokenKind::Semicolon, "expected ';' after for-init expression");
            init = std::make_unique<ExprStmt>(std::move(expr), loc);
        }
    } else {
        consume(TokenKind::Semicolon, "expected ';'");
    }

    // Condition
    std::unique_ptr<ExprAST> cond = nullptr;
    if (!check(TokenKind::Semicolon)) {
        cond = parse_expression();
    }
    consume(TokenKind::Semicolon, "expected ';' after for condition");

    // Step
    std::unique_ptr<ExprAST> step = nullptr;
    if (!check(TokenKind::RParen)) {
        step = parse_expression();
    }
    consume(TokenKind::RParen, "expected ')' after for clauses");

    auto body = parse_statement();
    return std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(step), std::move(body), loc);
}

std::unique_ptr<ReturnStmt> Parser::parse_return() {
    SourceLocation loc = previous().loc;
    std::unique_ptr<ExprAST> val = nullptr;
    if (!check(TokenKind::Semicolon)) {
        val = parse_expression();
    }
    consume(TokenKind::Semicolon, "expected ';' after return statement");
    return std::make_unique<ReturnStmt>(std::move(val), loc);
}

std::unique_ptr<ExprStmt> Parser::parse_expr_statement() {
    SourceLocation loc = peek().loc;
    auto expr = parse_expression();
    consume(TokenKind::Semicolon, "expected ';' after expression");
    return std::make_unique<ExprStmt>(std::move(expr), loc);
}

std::unique_ptr<ExprAST> Parser::parse_expression() {
    return parse_assignment();
}

std::unique_ptr<ExprAST> Parser::parse_assignment() {
    auto expr = parse_logical_or();

    if (match(TokenKind::Equal) || match(TokenKind::PlusEqual) ||
        match(TokenKind::MinusEqual) || match(TokenKind::StarEqual) ||
        match(TokenKind::SlashEqual)) {
        Token op_tok = previous();
        SourceLocation loc = op_tok.loc;
        AssignOp op = AssignOp::Assign;
        if (op_tok.kind == TokenKind::PlusEqual) op = AssignOp::AddAssign;
        else if (op_tok.kind == TokenKind::MinusEqual) op = AssignOp::SubAssign;
        else if (op_tok.kind == TokenKind::StarEqual) op = AssignOp::MulAssign;
        else if (op_tok.kind == TokenKind::SlashEqual) op = AssignOp::DivAssign;

        auto value = parse_assignment();
        return std::make_unique<AssignExpr>(std::move(expr), std::move(value), op, loc);
    }

    return expr;
}

std::unique_ptr<ExprAST> Parser::parse_logical_or() {
    auto expr = parse_logical_and();
    while (match(TokenKind::PipePipe)) {
        SourceLocation loc = previous().loc;
        auto right = parse_logical_and();
        expr = std::make_unique<BinaryExpr>(BinaryOp::Or, std::move(expr), std::move(right), loc);
    }
    return expr;
}

std::unique_ptr<ExprAST> Parser::parse_logical_and() {
    auto expr = parse_equality();
    while (match(TokenKind::AmpAmp)) {
        SourceLocation loc = previous().loc;
        auto right = parse_equality();
        expr = std::make_unique<BinaryExpr>(BinaryOp::And, std::move(expr), std::move(right), loc);
    }
    return expr;
}

std::unique_ptr<ExprAST> Parser::parse_equality() {
    auto expr = parse_comparison();
    while (match(TokenKind::EqualEqual) || match(TokenKind::ExclaimEqual)) {
        Token op_tok = previous();
        BinaryOp op = (op_tok.kind == TokenKind::EqualEqual) ? BinaryOp::Equal : BinaryOp::NotEqual;
        auto right = parse_comparison();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right), op_tok.loc);
    }
    return expr;
}

std::unique_ptr<ExprAST> Parser::parse_comparison() {
    auto expr = parse_additive();
    while (match(TokenKind::Less) || match(TokenKind::LessEqual) ||
           match(TokenKind::Greater) || match(TokenKind::GreaterEqual)) {
        Token op_tok = previous();
        BinaryOp op = BinaryOp::Less;
        if (op_tok.kind == TokenKind::LessEqual) op = BinaryOp::LessEqual;
        else if (op_tok.kind == TokenKind::Greater) op = BinaryOp::Greater;
        else if (op_tok.kind == TokenKind::GreaterEqual) op = BinaryOp::GreaterEqual;

        auto right = parse_additive();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right), op_tok.loc);
    }
    return expr;
}

std::unique_ptr<ExprAST> Parser::parse_additive() {
    auto expr = parse_multiplicative();
    while (match(TokenKind::Plus) || match(TokenKind::Minus)) {
        Token op_tok = previous();
        BinaryOp op = (op_tok.kind == TokenKind::Plus) ? BinaryOp::Add : BinaryOp::Sub;
        auto right = parse_multiplicative();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right), op_tok.loc);
    }
    return expr;
}

std::unique_ptr<ExprAST> Parser::parse_multiplicative() {
    auto expr = parse_unary();
    while (match(TokenKind::Star) || match(TokenKind::Slash) || match(TokenKind::Percent)) {
        Token op_tok = previous();
        BinaryOp op = BinaryOp::Mul;
        if (op_tok.kind == TokenKind::Slash) op = BinaryOp::Div;
        else if (op_tok.kind == TokenKind::Percent) op = BinaryOp::Mod;

        auto right = parse_unary();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right), op_tok.loc);
    }
    return expr;
}

std::unique_ptr<ExprAST> Parser::parse_unary() {
    if (match(TokenKind::Minus)) {
        SourceLocation loc = previous().loc;
        auto operand = parse_unary();
        return std::make_unique<UnaryExpr>(UnaryOp::Neg, std::move(operand), loc);
    }
    if (match(TokenKind::Exclaim)) {
        SourceLocation loc = previous().loc;
        auto operand = parse_unary();
        return std::make_unique<UnaryExpr>(UnaryOp::Not, std::move(operand), loc);
    }
    return parse_postfix();
}

std::unique_ptr<ExprAST> Parser::parse_postfix() {
    auto expr = parse_primary();

    while (true) {
        if (match(TokenKind::LBracket)) {
            SourceLocation loc = previous().loc;
            auto index = parse_expression();
            consume(TokenKind::RBracket, "expected ']' after array index");
            expr = std::make_unique<ArrayAccessExpr>(std::move(expr), std::move(index), loc);
        } else if (match(TokenKind::LParen)) {
            // Function call on identifier
            SourceLocation loc = previous().loc;
            std::vector<std::unique_ptr<ExprAST>> args;
            if (!check(TokenKind::RParen)) {
                do {
                    args.push_back(parse_expression());
                } while (match(TokenKind::Comma));
            }
            consume(TokenKind::RParen, "expected ')' after arguments");

            auto var_node = dynamic_cast<VarExpr*>(expr.get());
            if (var_node) {
                expr = std::make_unique<CallExpr>(var_node->name(), std::move(args), loc);
            } else {
                add_error(loc, "callee is not a valid callable identifier");
            }
        } else if (match(TokenKind::PlusPlus)) {
            // Desugar x++ to x += 1
            SourceLocation loc = previous().loc;
            auto one = std::make_unique<IntLiteralExpr>(1, loc);
            expr = std::make_unique<AssignExpr>(std::move(expr), std::move(one), AssignOp::AddAssign, loc);
        } else if (match(TokenKind::MinusMinus)) {
            // Desugar x-- to x -= 1
            SourceLocation loc = previous().loc;
            auto one = std::make_unique<IntLiteralExpr>(1, loc);
            expr = std::make_unique<AssignExpr>(std::move(expr), std::move(one), AssignOp::SubAssign, loc);
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<ExprAST> Parser::parse_primary() {
    SourceLocation loc = peek().loc;

    if (match(TokenKind::LiteralInt)) {
        return std::make_unique<IntLiteralExpr>(previous().int_val, loc);
    }
    if (match(TokenKind::LiteralFloat)) {
        return std::make_unique<FloatLiteralExpr>(previous().float_val, loc);
    }
    if (match(TokenKind::LiteralBool)) {
        return std::make_unique<BoolLiteralExpr>(previous().bool_val, loc);
    }
    if (match(TokenKind::Identifier)) {
        return std::make_unique<VarExpr>(previous().text, loc);
    }

    // Built-in GPU functions like thread_id_x()
    if (peek().kind == TokenKind::KwThreadIdX || peek().kind == TokenKind::KwThreadIdY ||
        peek().kind == TokenKind::KwThreadIdZ || peek().kind == TokenKind::KwBlockIdX ||
        peek().kind == TokenKind::KwBlockIdY || peek().kind == TokenKind::KwBlockIdZ ||
        peek().kind == TokenKind::KwBlockDimX || peek().kind == TokenKind::KwBlockDimY ||
        peek().kind == TokenKind::KwBlockDimZ || peek().kind == TokenKind::KwGridDimX ||
        peek().kind == TokenKind::KwGridDimY || peek().kind == TokenKind::KwGridDimZ) {
        std::string builtin_name = advance().text;
        consume(TokenKind::LParen, "expected '(' after built-in function " + builtin_name);
        consume(TokenKind::RParen, "expected ')' after '(' in " + builtin_name + " call");
        return std::make_unique<CallExpr>(builtin_name, std::vector<std::unique_ptr<ExprAST>>{}, loc);
    }

    if (match(TokenKind::LParen)) {
        auto expr = parse_expression();
        consume(TokenKind::RParen, "expected ')' after grouping expression");
        return expr;
    }

    add_error(loc, "expected expression");
    advance(); // recover
    return std::make_unique<IntLiteralExpr>(0, loc);
}

} // namespace minigpu
