#pragma once

#include "minigpu/ast.hpp"
#include "minigpu/types.hpp"
#include "minigpu/lexer.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace minigpu {

struct Symbol {
    std::string name;
    std::shared_ptr<Type> type;
    bool is_param{false};
    SourceLocation loc;
};

class Scope {
public:
    explicit Scope(Scope* parent = nullptr) : parent_(parent) {}

    Scope* parent() const { return parent_; }

    bool declare(const Symbol& sym) {
        if (symbols_.find(sym.name) != symbols_.end()) {
            return false; // Duplicate in current scope
        }
        symbols_[sym.name] = sym;
        return true;
    }

    const Symbol* lookup(const std::string& name) const {
        auto it = symbols_.find(name);
        if (it != symbols_.end()) return &it->second;
        if (parent_) return parent_->lookup(name);
        return nullptr;
    }

private:
    Scope* parent_{nullptr};
    std::unordered_map<std::string, Symbol> symbols_;
};

class SemanticAnalyzer : public ASTVisitor {
public:
    SemanticAnalyzer() = default;

    bool analyze(ProgramAST& program);

    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }
    bool has_errors() const { return has_errors_; }

    // AST Visitor methods
    void visit(IntLiteralExpr& node) override;
    void visit(FloatLiteralExpr& node) override;
    void visit(BoolLiteralExpr& node) override;
    void visit(VarExpr& node) override;
    void visit(BinaryExpr& node) override;
    void visit(UnaryExpr& node) override;
    void visit(ArrayAccessExpr& node) override;
    void visit(AssignExpr& node) override;
    void visit(CallExpr& node) override;

    void visit(ExprStmt& node) override;
    void visit(VarDeclStmt& node) override;
    void visit(BlockStmt& node) override;
    void visit(IfStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(ForStmt& node) override;
    void visit(ReturnStmt& node) override;
    void visit(BreakStmt& node) override;
    void visit(ContinueStmt& node) override;

    void visit(KernelDecl& node) override;
    void visit(ProgramAST& node) override;

private:
    void enter_scope();
    void exit_scope();
    void add_error(const SourceLocation& loc, const std::string& msg);

    std::vector<std::unique_ptr<Scope>> scope_storage_;
    Scope* current_scope_{nullptr};
    std::vector<Diagnostic> diagnostics_;
    bool has_errors_{false};
    int loop_depth_{0};
    std::string current_kernel_name_;
};

} // namespace minigpu
