#include "minigpu/ast.hpp"

namespace minigpu {

const char* binary_op_to_string(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add: return "+";
        case BinaryOp::Sub: return "-";
        case BinaryOp::Mul: return "*";
        case BinaryOp::Div: return "/";
        case BinaryOp::Mod: return "%";
        case BinaryOp::Less: return "<";
        case BinaryOp::LessEqual: return "<=";
        case BinaryOp::Greater: return ">";
        case BinaryOp::GreaterEqual: return ">=";
        case BinaryOp::Equal: return "==";
        case BinaryOp::NotEqual: return "!=";
        case BinaryOp::And: return "&&";
        case BinaryOp::Or: return "||";
    }
    return "?";
}

const char* unary_op_to_string(UnaryOp op) {
    switch (op) {
        case UnaryOp::Neg: return "-";
        case UnaryOp::Not: return "!";
    }
    return "?";
}

const char* assign_op_to_string(AssignOp op) {
    switch (op) {
        case AssignOp::Assign: return "=";
        case AssignOp::AddAssign: return "+=";
        case AssignOp::SubAssign: return "-=";
        case AssignOp::MulAssign: return "*=";
        case AssignOp::DivAssign: return "/=";
    }
    return "=";
}

void ASTPrinter::indent() {
    for (int i = 0; i < indent_level_; ++i) {
        out_ << "  ";
    }
}

void ASTPrinter::print(ProgramAST& program) {
    program.accept(*this);
}

void ASTPrinter::visit(IntLiteralExpr& node) {
    out_ << node.value();
    if (node.inferred_type()) {
        out_ << " :" << node.inferred_type()->to_string();
    }
}

void ASTPrinter::visit(FloatLiteralExpr& node) {
    out_ << node.value();
    if (node.inferred_type()) {
        out_ << " :" << node.inferred_type()->to_string();
    }
}

void ASTPrinter::visit(BoolLiteralExpr& node) {
    out_ << (node.value() ? "true" : "false");
    if (node.inferred_type()) {
        out_ << " :" << node.inferred_type()->to_string();
    }
}

void ASTPrinter::visit(VarExpr& node) {
    out_ << node.name();
    if (node.inferred_type()) {
        out_ << " :" << node.inferred_type()->to_string();
    }
}

void ASTPrinter::visit(BinaryExpr& node) {
    out_ << "(";
    node.lhs().accept(*this);
    out_ << " " << binary_op_to_string(node.op()) << " ";
    node.rhs().accept(*this);
    out_ << ")";
    if (node.inferred_type()) {
        out_ << " :" << node.inferred_type()->to_string();
    }
}

void ASTPrinter::visit(UnaryExpr& node) {
    out_ << "(" << unary_op_to_string(node.op());
    node.operand().accept(*this);
    out_ << ")";
    if (node.inferred_type()) {
        out_ << " :" << node.inferred_type()->to_string();
    }
}

void ASTPrinter::visit(ArrayAccessExpr& node) {
    node.array().accept(*this);
    out_ << "[";
    node.index().accept(*this);
    out_ << "]";
    if (node.inferred_type()) {
        out_ << " :" << node.inferred_type()->to_string();
    }
}

void ASTPrinter::visit(AssignExpr& node) {
    node.target().accept(*this);
    out_ << " " << assign_op_to_string(node.op()) << " ";
    node.value().accept(*this);
    if (node.inferred_type()) {
        out_ << " :" << node.inferred_type()->to_string();
    }
}

void ASTPrinter::visit(CallExpr& node) {
    out_ << node.callee() << "(";
    for (size_t i = 0; i < node.args().size(); ++i) {
        if (i > 0) out_ << ", ";
        node.args()[i]->accept(*this);
    }
    out_ << ")";
    if (node.inferred_type()) {
        out_ << " :" << node.inferred_type()->to_string();
    }
}

void ASTPrinter::visit(ExprStmt& node) {
    indent();
    node.expr().accept(*this);
    out_ << ";\n";
}

void ASTPrinter::visit(VarDeclStmt& node) {
    indent();
    out_ << node.type()->to_string() << " " << node.name();
    if (node.has_init()) {
        out_ << " = ";
        node.init()->accept(*this);
    }
    out_ << ";\n";
}

void ASTPrinter::visit(BlockStmt& node) {
    indent();
    out_ << "{\n";
    indent_level_++;
    for (auto& stmt : node.statements()) {
        stmt->accept(*this);
    }
    indent_level_--;
    indent();
    out_ << "}\n";
}

void ASTPrinter::visit(IfStmt& node) {
    indent();
    out_ << "if (";
    node.cond().accept(*this);
    out_ << ")\n";
    indent_level_++;
    node.then_branch().accept(*this);
    indent_level_--;
    if (node.else_branch()) {
        indent();
        out_ << "else\n";
        indent_level_++;
        node.else_branch()->accept(*this);
        indent_level_--;
    }
}

void ASTPrinter::visit(WhileStmt& node) {
    indent();
    out_ << "while (";
    node.cond().accept(*this);
    out_ << ")\n";
    indent_level_++;
    node.body().accept(*this);
    indent_level_--;
}

void ASTPrinter::visit(ForStmt& node) {
    indent();
    out_ << "for (";
    if (node.init()) {
        // If init is VarDecl, we don't want newline
        if (auto* v = dynamic_cast<VarDeclStmt*>(node.init())) {
            out_ << v->type()->to_string() << " " << v->name();
            if (v->has_init()) {
                out_ << " = ";
                v->init()->accept(*this);
            }
        } else if (auto* e = dynamic_cast<ExprStmt*>(node.init())) {
            e->expr().accept(*this);
        }
    }
    out_ << "; ";
    if (node.cond()) {
        node.cond()->accept(*this);
    }
    out_ << "; ";
    if (node.step()) {
        node.step()->accept(*this);
    }
    out_ << ")\n";
    indent_level_++;
    node.body().accept(*this);
    indent_level_--;
}

void ASTPrinter::visit(ReturnStmt& node) {
    indent();
    out_ << "return";
    if (node.value()) {
        out_ << " ";
        node.value()->accept(*this);
    }
    out_ << ";\n";
}

void ASTPrinter::visit(BreakStmt&) {
    indent();
    out_ << "break;\n";
}

void ASTPrinter::visit(ContinueStmt&) {
    indent();
    out_ << "continue;\n";
}

void ASTPrinter::visit(KernelDecl& node) {
    indent();
    out_ << "kernel " << node.name() << "(";
    for (size_t i = 0; i < node.params().size(); ++i) {
        if (i > 0) out_ << ", ";
        out_ << node.params()[i].type->to_string() << " " << node.params()[i].name;
    }
    out_ << ") {\n";
    indent_level_++;
    for (auto& s : node.body().statements()) {
        s->accept(*this);
    }
    indent_level_--;
    indent();
    out_ << "}\n";
}

void ASTPrinter::visit(ProgramAST& node) {
    for (auto& k : node.kernels()) {
        k->accept(*this);
        out_ << "\n";
    }
}

} // namespace minigpu
