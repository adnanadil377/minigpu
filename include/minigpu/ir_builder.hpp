#pragma once

#include "minigpu/ast.hpp"
#include "minigpu/ir.hpp"
#include <unordered_map>
#include <vector>
#include <memory>

namespace minigpu {

class IRGen : public ASTVisitor {
public:
    IRGen() = default;

    std::unique_ptr<IRModule> emit_module(ProgramAST& program);

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
    void emit_instruction(std::unique_ptr<IRInstruction> inst);
    void set_current_block(IRBasicBlock* bb);
    IRValue emit_lvalue(ExprAST& expr);

    struct LoopInfo {
        std::string continue_target;
        std::string break_target;
    };

    std::unique_ptr<IRModule> module_;
    IRFunction* current_fn_{nullptr};
    IRBasicBlock* current_bb_{nullptr};

    // Tracks variable storage (%alloca pointers)
    std::unordered_map<std::string, IRValue> var_table_;
    std::vector<LoopInfo> loop_stack_;
    IRValue last_value_;
};

} // namespace minigpu
