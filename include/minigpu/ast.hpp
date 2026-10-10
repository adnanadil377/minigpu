#pragma once

#include "minigpu/types.hpp"
#include <string>
#include <vector>
#include <memory>

namespace minigpu {

enum class BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual,
    And,
    Or
};

const char* binary_op_to_string(BinaryOp op);

enum class UnaryOp {
    Neg,
    Not
};

const char* unary_op_to_string(UnaryOp op);

enum class AssignOp {
    Assign,
    AddAssign,
    SubAssign,
    MulAssign,
    DivAssign
};

const char* assign_op_to_string(AssignOp op);

// Forward declarations of AST nodes
class IntLiteralExpr;
class FloatLiteralExpr;
class BoolLiteralExpr;
class VarExpr;
class BinaryExpr;
class UnaryExpr;
class ArrayAccessExpr;
class AssignExpr;
class CallExpr;

class ExprStmt;
class VarDeclStmt;
class BlockStmt;
class IfStmt;
class WhileStmt;
class ForStmt;
class ReturnStmt;
class BreakStmt;
class ContinueStmt;

class KernelDecl;
class ProgramAST;

/// AST Visitor interface
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(IntLiteralExpr& node) = 0;
    virtual void visit(FloatLiteralExpr& node) = 0;
    virtual void visit(BoolLiteralExpr& node) = 0;
    virtual void visit(VarExpr& node) = 0;
    virtual void visit(BinaryExpr& node) = 0;
    virtual void visit(UnaryExpr& node) = 0;
    virtual void visit(ArrayAccessExpr& node) = 0;
    virtual void visit(AssignExpr& node) = 0;
    virtual void visit(CallExpr& node) = 0;

    virtual void visit(ExprStmt& node) = 0;
    virtual void visit(VarDeclStmt& node) = 0;
    virtual void visit(BlockStmt& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(WhileStmt& node) = 0;
    virtual void visit(ForStmt& node) = 0;
    virtual void visit(ReturnStmt& node) = 0;
    virtual void visit(BreakStmt& node) = 0;
    virtual void visit(ContinueStmt& node) = 0;

    virtual void visit(KernelDecl& node) = 0;
    virtual void visit(ProgramAST& node) = 0;
};

/// Base AST node
class ASTNode {
public:
    explicit ASTNode(SourceLocation loc) : loc_(std::move(loc)) {}
    virtual ~ASTNode() = default;

    const SourceLocation& loc() const { return loc_; }
    virtual void accept(ASTVisitor& visitor) = 0;

private:
    SourceLocation loc_;
};

/// Base Expression node
class ExprAST : public ASTNode {
public:
    explicit ExprAST(SourceLocation loc) : ASTNode(std::move(loc)) {}

    const std::shared_ptr<Type>& inferred_type() const { return inferred_type_; }
    void set_inferred_type(std::shared_ptr<Type> t) { inferred_type_ = std::move(t); }

private:
    std::shared_ptr<Type> inferred_type_{nullptr};
};

/// Base Statement node
class StmtAST : public ASTNode {
public:
    explicit StmtAST(SourceLocation loc) : ASTNode(std::move(loc)) {}
};

// --- Expression Nodes ---

class IntLiteralExpr : public ExprAST {
public:
    IntLiteralExpr(int64_t val, SourceLocation loc)
        : ExprAST(std::move(loc)), val_(val) {}

    int64_t value() const { return val_; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    int64_t val_;
};

class FloatLiteralExpr : public ExprAST {
public:
    FloatLiteralExpr(double val, SourceLocation loc)
        : ExprAST(std::move(loc)), val_(val) {}

    double value() const { return val_; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    double val_;
};

class BoolLiteralExpr : public ExprAST {
public:
    BoolLiteralExpr(bool val, SourceLocation loc)
        : ExprAST(std::move(loc)), val_(val) {}

    bool value() const { return val_; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    bool val_;
};

class VarExpr : public ExprAST {
public:
    VarExpr(std::string name, SourceLocation loc)
        : ExprAST(std::move(loc)), name_(std::move(name)) {}

    const std::string& name() const { return name_; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::string name_;
};

class BinaryExpr : public ExprAST {
public:
    BinaryExpr(BinaryOp op, std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> rhs, SourceLocation loc)
        : ExprAST(std::move(loc)), op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    BinaryOp op() const { return op_; }
    ExprAST& lhs() { return *lhs_; }
    const ExprAST& lhs() const { return *lhs_; }
    ExprAST& rhs() { return *rhs_; }
    const ExprAST& rhs() const { return *rhs_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    BinaryOp op_;
    std::unique_ptr<ExprAST> lhs_;
    std::unique_ptr<ExprAST> rhs_;
};

class UnaryExpr : public ExprAST {
public:
    UnaryExpr(UnaryOp op, std::unique_ptr<ExprAST> operand, SourceLocation loc)
        : ExprAST(std::move(loc)), op_(op), operand_(std::move(operand)) {}

    UnaryOp op() const { return op_; }
    ExprAST& operand() { return *operand_; }
    const ExprAST& operand() const { return *operand_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    UnaryOp op_;
    std::unique_ptr<ExprAST> operand_;
};

class ArrayAccessExpr : public ExprAST {
public:
    ArrayAccessExpr(std::unique_ptr<ExprAST> array, std::unique_ptr<ExprAST> index, SourceLocation loc)
        : ExprAST(std::move(loc)), array_(std::move(array)), index_(std::move(index)) {}

    ExprAST& array() { return *array_; }
    const ExprAST& array() const { return *array_; }
    ExprAST& index() { return *index_; }
    const ExprAST& index() const { return *index_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::unique_ptr<ExprAST> array_;
    std::unique_ptr<ExprAST> index_;
};

class AssignExpr : public ExprAST {
public:
    AssignExpr(std::unique_ptr<ExprAST> target, std::unique_ptr<ExprAST> value, AssignOp op, SourceLocation loc)
        : ExprAST(std::move(loc)), target_(std::move(target)), value_(std::move(value)), op_(op) {}

    ExprAST& target() { return *target_; }
    const ExprAST& target() const { return *target_; }
    ExprAST& value() { return *value_; }
    const ExprAST& value() const { return *value_; }
    AssignOp op() const { return op_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::unique_ptr<ExprAST> target_;
    std::unique_ptr<ExprAST> value_;
    AssignOp op_;
};

class CallExpr : public ExprAST {
public:
    CallExpr(std::string callee, std::vector<std::unique_ptr<ExprAST>> args, SourceLocation loc)
        : ExprAST(std::move(loc)), callee_(std::move(callee)), args_(std::move(args)) {}

    const std::string& callee() const { return callee_; }
    const std::vector<std::unique_ptr<ExprAST>>& args() const { return args_; }
    std::vector<std::unique_ptr<ExprAST>>& args() { return args_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::string callee_;
    std::vector<std::unique_ptr<ExprAST>> args_;
};

// --- Statement Nodes ---

class ExprStmt : public StmtAST {
public:
    explicit ExprStmt(std::unique_ptr<ExprAST> expr, SourceLocation loc)
        : StmtAST(std::move(loc)), expr_(std::move(expr)) {}

    ExprAST& expr() { return *expr_; }
    const ExprAST& expr() const { return *expr_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::unique_ptr<ExprAST> expr_;
};

class VarDeclStmt : public StmtAST {
public:
    VarDeclStmt(std::shared_ptr<Type> type, std::string name, std::unique_ptr<ExprAST> init, SourceLocation loc)
        : StmtAST(std::move(loc)), type_(std::move(type)), name_(std::move(name)), init_(std::move(init)) {}

    const std::shared_ptr<Type>& type() const { return type_; }
    const std::string& name() const { return name_; }
    bool has_init() const { return init_ != nullptr; }
    ExprAST* init() { return init_.get(); }
    const ExprAST* init() const { return init_.get(); }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::shared_ptr<Type> type_;
    std::string name_;
    std::unique_ptr<ExprAST> init_;
};

class BlockStmt : public StmtAST {
public:
    BlockStmt(std::vector<std::unique_ptr<StmtAST>> stmts, SourceLocation loc)
        : StmtAST(std::move(loc)), stmts_(std::move(stmts)) {}

    std::vector<std::unique_ptr<StmtAST>>& statements() { return stmts_; }
    const std::vector<std::unique_ptr<StmtAST>>& statements() const { return stmts_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::vector<std::unique_ptr<StmtAST>> stmts_;
};

class IfStmt : public StmtAST {
public:
    IfStmt(std::unique_ptr<ExprAST> cond, std::unique_ptr<StmtAST> then_b, std::unique_ptr<StmtAST> else_b, SourceLocation loc)
        : StmtAST(std::move(loc)), cond_(std::move(cond)), then_b_(std::move(then_b)), else_b_(std::move(else_b)) {}

    ExprAST& cond() { return *cond_; }
    const ExprAST& cond() const { return *cond_; }
    StmtAST& then_branch() { return *then_b_; }
    const StmtAST& then_branch() const { return *then_b_; }
    StmtAST* else_branch() { return else_b_.get(); }
    const StmtAST* else_branch() const { return else_b_.get(); }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::unique_ptr<ExprAST> cond_;
    std::unique_ptr<StmtAST> then_b_;
    std::unique_ptr<StmtAST> else_b_;
};

class WhileStmt : public StmtAST {
public:
    WhileStmt(std::unique_ptr<ExprAST> cond, std::unique_ptr<StmtAST> body, SourceLocation loc)
        : StmtAST(std::move(loc)), cond_(std::move(cond)), body_(std::move(body)) {}

    ExprAST& cond() { return *cond_; }
    const ExprAST& cond() const { return *cond_; }
    StmtAST& body() { return *body_; }
    const StmtAST& body() const { return *body_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::unique_ptr<ExprAST> cond_;
    std::unique_ptr<StmtAST> body_;
};

class ForStmt : public StmtAST {
public:
    ForStmt(std::unique_ptr<StmtAST> init, std::unique_ptr<ExprAST> cond, std::unique_ptr<ExprAST> step,
            std::unique_ptr<StmtAST> body, SourceLocation loc)
        : StmtAST(std::move(loc)), init_(std::move(init)), cond_(std::move(cond)), step_(std::move(step)), body_(std::move(body)) {}

    StmtAST* init() { return init_.get(); }
    const StmtAST* init() const { return init_.get(); }
    ExprAST* cond() { return cond_.get(); }
    const ExprAST* cond() const { return cond_.get(); }
    ExprAST* step() { return step_.get(); }
    const ExprAST* step() const { return step_.get(); }
    StmtAST& body() { return *body_; }
    const StmtAST& body() const { return *body_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::unique_ptr<StmtAST> init_;
    std::unique_ptr<ExprAST> cond_;
    std::unique_ptr<ExprAST> step_;
    std::unique_ptr<StmtAST> body_;
};

class ReturnStmt : public StmtAST {
public:
    ReturnStmt(std::unique_ptr<ExprAST> val, SourceLocation loc)
        : StmtAST(std::move(loc)), val_(std::move(val)) {}

    ExprAST* value() { return val_.get(); }
    const ExprAST* value() const { return val_.get(); }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::unique_ptr<ExprAST> val_;
};

class BreakStmt : public StmtAST {
public:
    explicit BreakStmt(SourceLocation loc) : StmtAST(std::move(loc)) {}
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class ContinueStmt : public StmtAST {
public:
    explicit ContinueStmt(SourceLocation loc) : StmtAST(std::move(loc)) {}
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

// --- Top-level Declarations ---

struct ParamDecl {
    std::shared_ptr<Type> type;
    std::string name;
    SourceLocation loc;

    ParamDecl(std::shared_ptr<Type> t, std::string n, SourceLocation l)
        : type(std::move(t)), name(std::move(n)), loc(std::move(l)) {}
};

class KernelDecl : public ASTNode {
public:
    KernelDecl(std::string name, std::vector<ParamDecl> params, std::unique_ptr<BlockStmt> body, SourceLocation loc)
        : ASTNode(std::move(loc)), name_(std::move(name)), params_(std::move(params)), body_(std::move(body)) {}

    const std::string& name() const { return name_; }
    const std::vector<ParamDecl>& params() const { return params_; }
    BlockStmt& body() { return *body_; }
    const BlockStmt& body() const { return *body_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::string name_;
    std::vector<ParamDecl> params_;
    std::unique_ptr<BlockStmt> body_;
};

class ProgramAST : public ASTNode {
public:
    explicit ProgramAST(std::vector<std::unique_ptr<KernelDecl>> kernels, SourceLocation loc = {})
        : ASTNode(std::move(loc)), kernels_(std::move(kernels)) {}

    std::vector<std::unique_ptr<KernelDecl>>& kernels() { return kernels_; }
    const std::vector<std::unique_ptr<KernelDecl>>& kernels() const { return kernels_; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

private:
    std::vector<std::unique_ptr<KernelDecl>> kernels_;
};

/// AST Printer for `--emit-ast`
class ASTPrinter : public ASTVisitor {
public:
    explicit ASTPrinter(std::ostream& out) : out_(out) {}

    void print(ProgramAST& program);

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
    void indent();
    std::ostream& out_;
    int indent_level_{0};
};

} // namespace minigpu
