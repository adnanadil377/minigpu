#include "minigpu/ir_builder.hpp"

namespace minigpu {

std::unique_ptr<IRModule> IRGen::emit_module(ProgramAST& program) {
    module_ = std::make_unique<IRModule>("minigpu_module");
    program.accept(*this);
    return std::move(module_);
}

void IRGen::emit_instruction(std::unique_ptr<IRInstruction> inst) {
    if (current_bb_ && !current_bb_->is_terminated()) {
        current_bb_->add_instruction(std::move(inst));
    }
}

void IRGen::set_current_block(IRBasicBlock* bb) {
    current_bb_ = bb;
}

void IRGen::visit(ProgramAST& node) {
    for (auto& k : node.kernels()) {
        k->accept(*this);
    }
}

void IRGen::visit(KernelDecl& node) {
    var_table_.clear();
    loop_stack_.clear();

    auto fn = std::make_unique<IRFunction>(node.name(), Type::getVoid());
    current_fn_ = fn.get();

    // Entry basic block
    auto entry_bb = current_fn_->create_block("entry");
    set_current_block(entry_bb);

    // Setup parameters
    for (const auto& param : node.params()) {
        auto param_val = IRValue::make_param(param.name, param.type);
        current_fn_->add_param(param_val);

        // Allocate slot for parameter
        auto ptr_type = Type::getPointer(param.type);
        auto alloca_reg = current_fn_->create_register(ptr_type, param.name + "_addr");
        emit_instruction(std::make_unique<IRInstruction>(
            IROpcode::Alloca, alloca_reg, std::vector<IRValue>{}, param.loc));

        // Store parameter value into slot
        emit_instruction(std::make_unique<IRInstruction>(
            IROpcode::Store, IRValue(), std::vector<IRValue>{param_val, alloca_reg}, param.loc));

        var_table_[param.name] = alloca_reg;
    }

    // Lower body
    node.body().accept(*this);

    // Terminate entry/last block with return if not terminated
    if (current_bb_ && !current_bb_->is_terminated()) {
        emit_instruction(std::make_unique<IRInstruction>(
            IROpcode::Return, IRValue(), std::vector<IRValue>{}));
    }

    module_->add_function(std::move(fn));
    current_fn_ = nullptr;
    current_bb_ = nullptr;
}

void IRGen::visit(BlockStmt& node) {
    for (auto& stmt : node.statements()) {
        stmt->accept(*this);
    }
}

void IRGen::visit(VarDeclStmt& node) {
    auto ptr_type = Type::getPointer(node.type());
    auto alloca_reg = current_fn_->create_register(ptr_type, node.name() + "_addr");

    emit_instruction(std::make_unique<IRInstruction>(
        IROpcode::Alloca, alloca_reg, std::vector<IRValue>{}, node.loc()));

    var_table_[node.name()] = alloca_reg;

    if (node.has_init()) {
        node.init()->accept(*this);
        IRValue init_val = last_value_;

        // If target is float and init is int, cast it
        if (node.type()->is_float() && init_val.type()->is_int()) {
            if (init_val.is_const()) {
                init_val = IRValue::make_float_const(static_cast<double>(init_val.int_val()));
            } else {
                auto cast_reg = current_fn_->create_register(Type::getFloat(), "conv_flt");
                emit_instruction(std::make_unique<IRInstruction>(
                    IROpcode::SIToFP, cast_reg, std::vector<IRValue>{init_val}, node.loc()));
                init_val = cast_reg;
            }
        }

        emit_instruction(std::make_unique<IRInstruction>(
            IROpcode::Store, IRValue(), std::vector<IRValue>{init_val, alloca_reg}, node.loc()));
    }
}

void IRGen::visit(ExprStmt& node) {
    node.expr().accept(*this);
}

void IRGen::visit(IfStmt& node) {
    node.cond().accept(*this);
    IRValue cond_val = last_value_;

    auto then_bb = current_fn_->create_block("bb_then");
    auto else_bb = node.else_branch() ? current_fn_->create_block("bb_else") : nullptr;
    auto merge_bb = current_fn_->create_block("bb_merge");

    std::string false_target = else_bb ? else_bb->label() : merge_bb->label();

    auto br_cond = std::make_unique<IRInstruction>(
        IROpcode::BranchCond, IRValue(), std::vector<IRValue>{cond_val}, node.loc());
    br_cond->set_true_label(then_bb->label());
    br_cond->set_false_label(false_target);
    emit_instruction(std::move(br_cond));

    // Then branch
    set_current_block(then_bb);
    node.then_branch().accept(*this);
    if (current_bb_ && !current_bb_->is_terminated()) {
        auto br = std::make_unique<IRInstruction>(IROpcode::Branch, IRValue(), std::vector<IRValue>{});
        br->set_true_label(merge_bb->label());
        emit_instruction(std::move(br));
    }

    // Else branch
    if (else_bb) {
        set_current_block(else_bb);
        node.else_branch()->accept(*this);
        if (current_bb_ && !current_bb_->is_terminated()) {
            auto br = std::make_unique<IRInstruction>(IROpcode::Branch, IRValue(), std::vector<IRValue>{});
            br->set_true_label(merge_bb->label());
            emit_instruction(std::move(br));
        }
    }

    set_current_block(merge_bb);
}

void IRGen::visit(WhileStmt& node) {
    auto header_bb = current_fn_->create_block("bb_while_cond");
    auto body_bb = current_fn_->create_block("bb_while_body");
    auto exit_bb = current_fn_->create_block("bb_while_exit");

    // Jump from current to header
    auto br_header = std::make_unique<IRInstruction>(IROpcode::Branch, IRValue(), std::vector<IRValue>{});
    br_header->set_true_label(header_bb->label());
    emit_instruction(std::move(br_header));

    // Header BB
    set_current_block(header_bb);
    node.cond().accept(*this);
    IRValue cond_val = last_value_;
    auto br_cond = std::make_unique<IRInstruction>(
        IROpcode::BranchCond, IRValue(), std::vector<IRValue>{cond_val}, node.loc());
    br_cond->set_true_label(body_bb->label());
    br_cond->set_false_label(exit_bb->label());
    emit_instruction(std::move(br_cond));

    // Body BB
    set_current_block(body_bb);
    loop_stack_.push_back({header_bb->label(), exit_bb->label()});
    node.body().accept(*this);
    loop_stack_.pop_back();

    if (current_bb_ && !current_bb_->is_terminated()) {
        auto br_loop = std::make_unique<IRInstruction>(IROpcode::Branch, IRValue(), std::vector<IRValue>{});
        br_loop->set_true_label(header_bb->label());
        emit_instruction(std::move(br_loop));
    }

    set_current_block(exit_bb);
}

void IRGen::visit(ForStmt& node) {
    if (node.init()) {
        node.init()->accept(*this);
    }

    auto cond_bb = current_fn_->create_block("bb_for_cond");
    auto body_bb = current_fn_->create_block("bb_for_body");
    auto step_bb = current_fn_->create_block("bb_for_step");
    auto exit_bb = current_fn_->create_block("bb_for_exit");

    auto br_cond_init = std::make_unique<IRInstruction>(IROpcode::Branch, IRValue(), std::vector<IRValue>{});
    br_cond_init->set_true_label(cond_bb->label());
    emit_instruction(std::move(br_cond_init));

    // Cond BB
    set_current_block(cond_bb);
    IRValue cond_val;
    if (node.cond()) {
        node.cond()->accept(*this);
        cond_val = last_value_;
    } else {
        cond_val = IRValue::make_bool_const(true);
    }
    auto br_cond = std::make_unique<IRInstruction>(
        IROpcode::BranchCond, IRValue(), std::vector<IRValue>{cond_val}, node.loc());
    br_cond->set_true_label(body_bb->label());
    br_cond->set_false_label(exit_bb->label());
    emit_instruction(std::move(br_cond));

    // Body BB
    set_current_block(body_bb);
    loop_stack_.push_back({step_bb->label(), exit_bb->label()});
    node.body().accept(*this);
    loop_stack_.pop_back();

    if (current_bb_ && !current_bb_->is_terminated()) {
        auto br_to_step = std::make_unique<IRInstruction>(IROpcode::Branch, IRValue(), std::vector<IRValue>{});
        br_to_step->set_true_label(step_bb->label());
        emit_instruction(std::move(br_to_step));
    }

    // Step BB
    set_current_block(step_bb);
    if (node.step()) {
        node.step()->accept(*this);
    }
    if (current_bb_ && !current_bb_->is_terminated()) {
        auto br_to_cond = std::make_unique<IRInstruction>(IROpcode::Branch, IRValue(), std::vector<IRValue>{});
        br_to_cond->set_true_label(cond_bb->label());
        emit_instruction(std::move(br_to_cond));
    }

    set_current_block(exit_bb);
}

void IRGen::visit(ReturnStmt& node) {
    if (node.value()) {
        node.value()->accept(*this);
        emit_instruction(std::make_unique<IRInstruction>(
            IROpcode::Return, IRValue(), std::vector<IRValue>{last_value_}, node.loc()));
    } else {
        emit_instruction(std::make_unique<IRInstruction>(
            IROpcode::Return, IRValue(), std::vector<IRValue>{}, node.loc()));
    }
}

void IRGen::visit(BreakStmt& node) {
    if (!loop_stack_.empty()) {
        auto br = std::make_unique<IRInstruction>(IROpcode::Branch, IRValue(), std::vector<IRValue>{}, node.loc());
        br->set_true_label(loop_stack_.back().break_target);
        emit_instruction(std::move(br));
    }
}

void IRGen::visit(ContinueStmt& node) {
    if (!loop_stack_.empty()) {
        auto br = std::make_unique<IRInstruction>(IROpcode::Branch, IRValue(), std::vector<IRValue>{}, node.loc());
        br->set_true_label(loop_stack_.back().continue_target);
        emit_instruction(std::move(br));
    }
}

void IRGen::visit(IntLiteralExpr& node) {
    last_value_ = IRValue::make_int_const(node.value());
}

void IRGen::visit(FloatLiteralExpr& node) {
    last_value_ = IRValue::make_float_const(node.value());
}

void IRGen::visit(BoolLiteralExpr& node) {
    last_value_ = IRValue::make_bool_const(node.value());
}

void IRGen::visit(VarExpr& node) {
    auto it = var_table_.find(node.name());
    if (it != var_table_.end()) {
        auto ptr = it->second;
        auto elem_type = ptr.type()->element_type();
        auto load_reg = current_fn_->create_register(elem_type, node.name());
        emit_instruction(std::make_unique<IRInstruction>(
            IROpcode::Load, load_reg, std::vector<IRValue>{ptr}, node.loc()));
        last_value_ = load_reg;
    } else {
        last_value_ = IRValue::make_int_const(0);
    }
}

IRValue IRGen::emit_lvalue(ExprAST& expr) {
    if (auto* var = dynamic_cast<VarExpr*>(&expr)) {
        auto it = var_table_.find(var->name());
        if (it != var_table_.end()) {
            return it->second;
        }
    } else if (auto* arr = dynamic_cast<ArrayAccessExpr*>(&expr)) {
        // Base array pointer
        arr->array().accept(*this);
        IRValue base_ptr = last_value_;

        // Index
        arr->index().accept(*this);
        IRValue idx = last_value_;

        auto elem_type = arr->array().inferred_type()->element_type();
        auto gep_reg = current_fn_->create_register(Type::getPointer(elem_type), "gep_addr");

        emit_instruction(std::make_unique<IRInstruction>(
            IROpcode::GetElementPtr, gep_reg, std::vector<IRValue>{base_ptr, idx}, arr->loc()));
        return gep_reg;
    }
    return IRValue();
}

void IRGen::visit(ArrayAccessExpr& node) {
    auto ptr = emit_lvalue(node);
    auto elem_type = node.inferred_type();
    auto load_reg = current_fn_->create_register(elem_type, "arr_elem");
    emit_instruction(std::make_unique<IRInstruction>(
        IROpcode::Load, load_reg, std::vector<IRValue>{ptr}, node.loc()));
    last_value_ = load_reg;
}

void IRGen::visit(AssignExpr& node) {
    node.value().accept(*this);
    IRValue rhs = last_value_;

    // Handle compound assignment (+=, -=, *=, /=)
    if (node.op() != AssignOp::Assign) {
        // Load old value
        node.target().accept(*this);
        IRValue lhs = last_value_;

        IROpcode op = IROpcode::Add;
        bool is_float = (node.inferred_type() && node.inferred_type()->is_float());

        switch (node.op()) {
            case AssignOp::AddAssign: op = is_float ? IROpcode::FAdd : IROpcode::Add; break;
            case AssignOp::SubAssign: op = is_float ? IROpcode::FSub : IROpcode::Sub; break;
            case AssignOp::MulAssign: op = is_float ? IROpcode::FMul : IROpcode::Mul; break;
            case AssignOp::DivAssign: op = is_float ? IROpcode::FDiv : IROpcode::SDiv; break;
            default: break;
        }

        auto res_reg = current_fn_->create_register(node.inferred_type(), "op_assign");
        emit_instruction(std::make_unique<IRInstruction>(
            op, res_reg, std::vector<IRValue>{lhs, rhs}, node.loc()));
        rhs = res_reg;
    }

    // Target lvalue address
    auto target_addr = emit_lvalue(node.target());
    if (target_addr.kind() != IRValueKind::None) {
        emit_instruction(std::make_unique<IRInstruction>(
            IROpcode::Store, IRValue(), std::vector<IRValue>{rhs, target_addr}, node.loc()));
    }

    last_value_ = rhs;
}

void IRGen::visit(BinaryExpr& node) {
    node.lhs().accept(*this);
    IRValue lhs = last_value_;

    node.rhs().accept(*this);
    IRValue rhs = last_value_;

    bool is_float = (lhs.type()->is_float() || rhs.type()->is_float());

    // If one operand is float and other is int, cast int to float
    if (is_float) {
        if (lhs.type()->is_int()) {
            if (lhs.is_const()) {
                lhs = IRValue::make_float_const(static_cast<double>(lhs.int_val()));
            } else {
                auto c = current_fn_->create_register(Type::getFloat(), "conv_flt");
                emit_instruction(std::make_unique<IRInstruction>(IROpcode::SIToFP, c, std::vector<IRValue>{lhs}, node.loc()));
                lhs = c;
            }
        }
        if (rhs.type()->is_int()) {
            if (rhs.is_const()) {
                rhs = IRValue::make_float_const(static_cast<double>(rhs.int_val()));
            } else {
                auto c = current_fn_->create_register(Type::getFloat(), "conv_flt");
                emit_instruction(std::make_unique<IRInstruction>(IROpcode::SIToFP, c, std::vector<IRValue>{rhs}, node.loc()));
                rhs = c;
            }
        }
    }

    IROpcode op = IROpcode::Add;
    std::shared_ptr<Type> res_type = is_float ? Type::getFloat() : Type::getInt();

    switch (node.op()) {
        case BinaryOp::Add: op = is_float ? IROpcode::FAdd : IROpcode::Add; break;
        case BinaryOp::Sub: op = is_float ? IROpcode::FSub : IROpcode::Sub; break;
        case BinaryOp::Mul: op = is_float ? IROpcode::FMul : IROpcode::Mul; break;
        case BinaryOp::Div: op = is_float ? IROpcode::FDiv : IROpcode::SDiv; break;
        case BinaryOp::Mod: op = IROpcode::SRem; res_type = Type::getInt(); break;

        case BinaryOp::Less:
            op = is_float ? IROpcode::FCmpOLT : IROpcode::ICmpSLT;
            res_type = Type::getBool();
            break;
        case BinaryOp::LessEqual:
            op = is_float ? IROpcode::FCmpOLE : IROpcode::ICmpSLE;
            res_type = Type::getBool();
            break;
        case BinaryOp::Greater:
            op = is_float ? IROpcode::FCmpOGT : IROpcode::ICmpSGT;
            res_type = Type::getBool();
            break;
        case BinaryOp::GreaterEqual:
            op = is_float ? IROpcode::FCmpOGE : IROpcode::ICmpSGE;
            res_type = Type::getBool();
            break;
        case BinaryOp::Equal:
            op = is_float ? IROpcode::FCmpEQ : IROpcode::ICmpEQ;
            res_type = Type::getBool();
            break;
        case BinaryOp::NotEqual:
            op = is_float ? IROpcode::FCmpNE : IROpcode::ICmpNE;
            res_type = Type::getBool();
            break;
        case BinaryOp::And:
            op = IROpcode::And;
            res_type = Type::getBool();
            break;
        case BinaryOp::Or:
            op = IROpcode::Or;
            res_type = Type::getBool();
            break;
    }

    auto res_reg = current_fn_->create_register(res_type, "binop");
    emit_instruction(std::make_unique<IRInstruction>(
        op, res_reg, std::vector<IRValue>{lhs, rhs}, node.loc()));
    last_value_ = res_reg;
}

void IRGen::visit(UnaryExpr& node) {
    node.operand().accept(*this);
    IRValue operand = last_value_;

    if (node.op() == UnaryOp::Neg) {
        if (operand.type()->is_float()) {
            auto zero = IRValue::make_float_const(0.0);
            auto res = current_fn_->create_register(Type::getFloat(), "neg");
            emit_instruction(std::make_unique<IRInstruction>(
                IROpcode::FSub, res, std::vector<IRValue>{zero, operand}, node.loc()));
            last_value_ = res;
        } else {
            auto zero = IRValue::make_int_const(0);
            auto res = current_fn_->create_register(Type::getInt(), "neg");
            emit_instruction(std::make_unique<IRInstruction>(
                IROpcode::Sub, res, std::vector<IRValue>{zero, operand}, node.loc()));
            last_value_ = res;
        }
    } else if (node.op() == UnaryOp::Not) {
        auto true_val = IRValue::make_bool_const(true);
        auto res = current_fn_->create_register(Type::getBool(), "not");
        emit_instruction(std::make_unique<IRInstruction>(
            IROpcode::Xor, res, std::vector<IRValue>{operand, true_val}, node.loc()));
        last_value_ = res;
    }
}

void IRGen::visit(CallExpr& node) {
    // Built-in GPU registers
    auto res_reg = current_fn_->create_register(Type::getInt(), node.callee());
    auto inst = std::make_unique<IRInstruction>(
        IROpcode::SpecialReg, res_reg, std::vector<IRValue>{}, node.loc());
    inst->set_special_reg(node.callee());
    emit_instruction(std::move(inst));
    last_value_ = res_reg;
}

} // namespace minigpu
