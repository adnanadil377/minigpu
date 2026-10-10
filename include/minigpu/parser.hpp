#pragma once

#include "minigpu/token.hpp"
#include "minigpu/ast.hpp"
#include "minigpu/lexer.hpp"
#include <vector>
#include <memory>
#include <string>

namespace minigpu {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::unique_ptr<ProgramAST> parse_program();

    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }
    bool has_errors() const { return has_errors_; }

private:
    const Token& peek() const;
    const Token& peek_next() const;
    const Token& previous() const;
    bool is_at_end() const;
    const Token& advance();
    bool check(TokenKind kind) const;
    bool match(TokenKind kind);
    const Token& consume(TokenKind kind, const std::string& error_msg);

    void synchronize();
    void add_error(const SourceLocation& loc, const std::string& msg);

    // Parsing methods
    std::unique_ptr<KernelDecl> parse_kernel();
    ParamDecl parse_param();
    std::shared_ptr<Type> parse_type();

    std::unique_ptr<StmtAST> parse_statement();
    std::unique_ptr<BlockStmt> parse_block();
    std::unique_ptr<VarDeclStmt> parse_var_decl();
    std::unique_ptr<IfStmt> parse_if();
    std::unique_ptr<WhileStmt> parse_while();
    std::unique_ptr<ForStmt> parse_for();
    std::unique_ptr<ReturnStmt> parse_return();
    std::unique_ptr<ExprStmt> parse_expr_statement();

    std::unique_ptr<ExprAST> parse_expression();
    std::unique_ptr<ExprAST> parse_assignment();
    std::unique_ptr<ExprAST> parse_logical_or();
    std::unique_ptr<ExprAST> parse_logical_and();
    std::unique_ptr<ExprAST> parse_equality();
    std::unique_ptr<ExprAST> parse_comparison();
    std::unique_ptr<ExprAST> parse_additive();
    std::unique_ptr<ExprAST> parse_multiplicative();
    std::unique_ptr<ExprAST> parse_unary();
    std::unique_ptr<ExprAST> parse_postfix();
    std::unique_ptr<ExprAST> parse_primary();

    std::vector<Token> tokens_;
    size_t current_{0};
    std::vector<Diagnostic> diagnostics_;
    bool has_errors_{false};
};

} // namespace minigpu
