#include "minigpu/semantic_analysis.hpp"

namespace minigpu {

static const std::unordered_map<std::string, bool> kBuiltinFunctions = {
    {"thread_id_x", true},
    {"thread_id_y", true},
    {"thread_id_z", true},
    {"block_id_x", true},
    {"block_id_y", true},
    {"block_id_z", true},
    {"block_dim_x", true},
    {"block_dim_y", true},
    {"block_dim_z", true},
    {"grid_dim_x", true},
    {"grid_dim_y", true},
    {"grid_dim_z", true}
};

void SemanticAnalyzer::enter_scope() {
    scope_storage_.push_back(std::make_unique<Scope>(current_scope_));
    current_scope_ = scope_storage_.back().get();
}

void SemanticAnalyzer::exit_scope() {
    if (current_scope_) {
        current_scope_ = current_scope_->parent();
    }
}

void SemanticAnalyzer::add_error(const SourceLocation& loc, const std::string& msg) {
    diagnostics_.push_back({loc, msg, true});
    has_errors_ = true;
}

bool SemanticAnalyzer::analyze(ProgramAST& program) {
    program.accept(*this);
    return !has_errors_;
}

void SemanticAnalyzer::visit(ProgramAST& node) {
    for (auto& k : node.kernels()) {
        k->accept(*this);
    }
}

void SemanticAnalyzer::visit(KernelDecl& node) {
    current_kernel_name_ = node.name();
    enter_scope(); // Kernel function scope

    // Register parameters
    for (const auto& param : node.params()) {
        if (!current_scope_->declare({param.name, param.type, true, param.loc})) {
            add_error(param.loc, "duplicate parameter name '" + param.name + "'");
        }
    }

    // Analyze body
    node.body().accept(*this);

    exit_scope();
}

void SemanticAnalyzer::visit(BlockStmt& node) {
    enter_scope();
    for (auto& stmt : node.statements()) {
        stmt->accept(*this);
    }
    exit_scope();
}

void SemanticAnalyzer::visit(VarDeclStmt& node) {
    if (node.has_init()) {
        node.init()->accept(*this);
        auto init_type = node.init()->inferred_type();
        if (init_type) {
            // Check type compatibility
            if (!node.type()->equals(*init_type)) {
                // Allow int literal -> float promotion
                if (node.type()->is_float() && init_type->is_int()) {
                    // Allowed widening conversion
                } else {
                    add_error(node.loc(), "cannot initialize variable of type '" +
                              node.type()->to_string() + "' with value of type '" +
                              init_type->to_string() + "'");
                }
            }
        }
    }

    if (!current_scope_->declare({node.name(), node.type(), false, node.loc()})) {
        add_error(node.loc(), "redefinition of variable '" + node.name() + "' in the same scope");
    }
}

void SemanticAnalyzer::visit(ExprStmt& node) {
    node.expr().accept(*this);
}

void SemanticAnalyzer::visit(IfStmt& node) {
    node.cond().accept(*this);
    if (node.cond().inferred_type()) {
        if (!node.cond().inferred_type()->is_bool() && !node.cond().inferred_type()->is_int()) {
            add_error(node.cond().loc(), "if condition must be boolean or integer, got '" +
                      node.cond().inferred_type()->to_string() + "'");
        }
    }

    node.then_branch().accept(*this);
    if (node.else_branch()) {
        node.else_branch()->accept(*this);
    }
}

void SemanticAnalyzer::visit(WhileStmt& node) {
    node.cond().accept(*this);
    if (node.cond().inferred_type()) {
        if (!node.cond().inferred_type()->is_bool() && !node.cond().inferred_type()->is_int()) {
            add_error(node.cond().loc(), "while condition must be boolean or integer, got '" +
                      node.cond().inferred_type()->to_string() + "'");
        }
    }

    loop_depth_++;
    node.body().accept(*this);
    loop_depth_--;
}

void SemanticAnalyzer::visit(ForStmt& node) {
    enter_scope(); // for statement introduces its own scope for loop var

    if (node.init()) {
        node.init()->accept(*this);
    }
    if (node.cond()) {
        node.cond()->accept(*this);
        if (node.cond()->inferred_type()) {
            if (!node.cond()->inferred_type()->is_bool() && !node.cond()->inferred_type()->is_int()) {
                add_error(node.cond()->loc(), "for condition must be boolean or integer");
            }
        }
    }
    if (node.step()) {
        node.step()->accept(*this);
    }

    loop_depth_++;
    node.body().accept(*this);
    loop_depth_--;

    exit_scope();
}

void SemanticAnalyzer::visit(ReturnStmt& node) {
    if (node.value()) {
        node.value()->accept(*this);
        add_error(node.loc(), "kernel cannot return a value");
    }
}

void SemanticAnalyzer::visit(BreakStmt& node) {
    if (loop_depth_ == 0) {
        add_error(node.loc(), "'break' statement not within a loop");
    }
}

void SemanticAnalyzer::visit(ContinueStmt& node) {
    if (loop_depth_ == 0) {
        add_error(node.loc(), "'continue' statement not within a loop");
    }
}

void SemanticAnalyzer::visit(IntLiteralExpr& node) {
    node.set_inferred_type(Type::getInt());
}

void SemanticAnalyzer::visit(FloatLiteralExpr& node) {
    node.set_inferred_type(Type::getFloat());
}

void SemanticAnalyzer::visit(BoolLiteralExpr& node) {
    node.set_inferred_type(Type::getBool());
}

void SemanticAnalyzer::visit(VarExpr& node) {
    const Symbol* sym = current_scope_ ? current_scope_->lookup(node.name()) : nullptr;
    if (!sym) {
        add_error(node.loc(), "use of undeclared identifier '" + node.name() + "'");
        node.set_inferred_type(Type::getInt()); // fallback
        return;
    }
    node.set_inferred_type(sym->type);
}

void SemanticAnalyzer::visit(BinaryExpr& node) {
    node.lhs().accept(*this);
    node.rhs().accept(*this);

    auto lt = node.lhs().inferred_type();
    auto rt = node.rhs().inferred_type();

    if (!lt || !rt) {
        node.set_inferred_type(Type::getInt());
        return;
    }

    switch (node.op()) {
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div: {
            if (lt->is_numeric() && rt->is_numeric()) {
                if (lt->is_float() || rt->is_float()) {
                    node.set_inferred_type(Type::getFloat());
                } else {
                    node.set_inferred_type(Type::getInt());
                }
            } else {
                add_error(node.loc(), "arithmetic operator requires numeric operands, got '" +
                          lt->to_string() + "' and '" + rt->to_string() + "'");
                node.set_inferred_type(Type::getInt());
            }
            break;
        }
        case BinaryOp::Mod: {
            if (lt->is_int() && rt->is_int()) {
                node.set_inferred_type(Type::getInt());
            } else {
                add_error(node.loc(), "modulo operator '%' requires integer operands");
                node.set_inferred_type(Type::getInt());
            }
            break;
        }
        case BinaryOp::Less:
        case BinaryOp::LessEqual:
        case BinaryOp::Greater:
        case BinaryOp::GreaterEqual:
        case BinaryOp::Equal:
        case BinaryOp::NotEqual: {
            if ((lt->is_numeric() && rt->is_numeric()) || (lt->is_bool() && rt->is_bool()) ||
                (lt->is_pointer() && rt->is_pointer())) {
                node.set_inferred_type(Type::getBool());
            } else {
                add_error(node.loc(), "incompatible types for comparison: '" +
                          lt->to_string() + "' and '" + rt->to_string() + "'");
                node.set_inferred_type(Type::getBool());
            }
            break;
        }
        case BinaryOp::And:
        case BinaryOp::Or: {
            if ((lt->is_bool() || lt->is_int()) && (rt->is_bool() || rt->is_int())) {
                node.set_inferred_type(Type::getBool());
            } else {
                add_error(node.loc(), "logical operator requires boolean or integer operands");
                node.set_inferred_type(Type::getBool());
            }
            break;
        }
    }
}

void SemanticAnalyzer::visit(UnaryExpr& node) {
    node.operand().accept(*this);
    auto t = node.operand().inferred_type();
    if (!t) {
        node.set_inferred_type(Type::getInt());
        return;
    }

    if (node.op() == UnaryOp::Neg) {
        if (!t->is_numeric()) {
            add_error(node.loc(), "unary negation '-' requires numeric operand, got '" + t->to_string() + "'");
        }
        node.set_inferred_type(t);
    } else if (node.op() == UnaryOp::Not) {
        if (!t->is_bool() && !t->is_int()) {
            add_error(node.loc(), "unary logical not '!' requires boolean or integer operand");
        }
        node.set_inferred_type(Type::getBool());
    }
}

void SemanticAnalyzer::visit(ArrayAccessExpr& node) {
    node.array().accept(*this);
    node.index().accept(*this);

    auto arr_type = node.array().inferred_type();
    auto idx_type = node.index().inferred_type();

    if (!arr_type || !arr_type->is_pointer()) {
        add_error(node.loc(), "subscripted value is not a pointer, got '" +
                  (arr_type ? arr_type->to_string() : "unknown") + "'");
        node.set_inferred_type(Type::getInt());
        return;
    }

    if (!idx_type || !idx_type->is_int()) {
        add_error(node.loc(), "array subscript must be an integer, got '" +
                  (idx_type ? idx_type->to_string() : "unknown") + "'");
    }

    node.set_inferred_type(arr_type->element_type());
}

void SemanticAnalyzer::visit(AssignExpr& node) {
    node.target().accept(*this);
    node.value().accept(*this);

    // Validate lvalue
    bool is_lvalue = (dynamic_cast<VarExpr*>(&node.target()) != nullptr) ||
                     (dynamic_cast<ArrayAccessExpr*>(&node.target()) != nullptr);
    if (!is_lvalue) {
        add_error(node.loc(), "assignment target must be an lvalue");
    }

    auto target_type = node.target().inferred_type();
    auto value_type = node.value().inferred_type();

    if (target_type && value_type) {
        if (!target_type->equals(*value_type)) {
            // Allow int literal to float conversion
            if (target_type->is_float() && value_type->is_int()) {
                // allowed
            } else {
                add_error(node.loc(), "cannot assign value of type '" + value_type->to_string() +
                          "' to target of type '" + target_type->to_string() + "'");
            }
        }
    }

    node.set_inferred_type(target_type);
}

void SemanticAnalyzer::visit(CallExpr& node) {
    for (auto& arg : node.args()) {
        arg->accept(*this);
    }

    if (kBuiltinFunctions.find(node.callee()) != kBuiltinFunctions.end()) {
        if (!node.args().empty()) {
            add_error(node.loc(), "built-in function '" + node.callee() + "' takes 0 arguments");
        }
        node.set_inferred_type(Type::getInt());
    } else {
        add_error(node.loc(), "call to undefined function '" + node.callee() + "'");
        node.set_inferred_type(Type::getInt());
    }
}

} // namespace minigpu
