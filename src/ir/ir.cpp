#include "minigpu/ir.hpp"
#include <sstream>

namespace minigpu {

const char* ir_opcode_to_string(IROpcode op) {
    switch (op) {
        case IROpcode::Add: return "add";
        case IROpcode::Sub: return "sub";
        case IROpcode::Mul: return "mul";
        case IROpcode::SDiv: return "sdiv";
        case IROpcode::SRem: return "srem";
        case IROpcode::FAdd: return "fadd";
        case IROpcode::FSub: return "fsub";
        case IROpcode::FMul: return "fmul";
        case IROpcode::FDiv: return "fdiv";
        case IROpcode::FMA: return "fma";
        case IROpcode::ICmpEQ: return "icmp_eq";
        case IROpcode::ICmpNE: return "icmp_ne";
        case IROpcode::ICmpSLT: return "icmp_slt";
        case IROpcode::ICmpSLE: return "icmp_sle";
        case IROpcode::ICmpSGT: return "icmp_sgt";
        case IROpcode::ICmpSGE: return "icmp_sge";
        case IROpcode::FCmpEQ: return "fcmp_eq";
        case IROpcode::FCmpNE: return "fcmp_ne";
        case IROpcode::FCmpOLT: return "fcmp_olt";
        case IROpcode::FCmpOLE: return "fcmp_ole";
        case IROpcode::FCmpOGT: return "fcmp_ogt";
        case IROpcode::FCmpOGE: return "fcmp_oge";
        case IROpcode::And: return "and";
        case IROpcode::Or: return "or";
        case IROpcode::Xor: return "xor";
        case IROpcode::SIToFP: return "sitofp";
        case IROpcode::FPToSI: return "fptosi";
        case IROpcode::Alloca: return "alloca";
        case IROpcode::Load: return "load";
        case IROpcode::Store: return "store";
        case IROpcode::GetElementPtr: return "gep";
        case IROpcode::SpecialReg: return "special_reg";
        case IROpcode::Copy: return "copy";
        case IROpcode::ParamLoad: return "param_load";
        case IROpcode::Branch: return "br";
        case IROpcode::BranchCond: return "br_cond";
        case IROpcode::Return: return "ret";
    }
    return "unknown_op";
}

std::string IRValue::to_string() const {
    switch (kind_) {
        case IRValueKind::None: return "none";
        case IRValueKind::Register:
            return "%v" + std::to_string(id_);
        case IRValueKind::ConstantInt:
            return std::to_string(int_val_);
        case IRValueKind::ConstantFloat: {
            std::ostringstream ss;
            ss << float_val_;
            if (ss.str().find('.') == std::string::npos) {
                ss << ".0";
            }
            ss << "f";
            return ss.str();
        }
        case IRValueKind::ConstantBool:
            return bool_val_ ? "true" : "false";
        case IRValueKind::Parameter:
            return "%" + str_val_;
    }
    return "?";
}

bool IRValue::equals(const IRValue& other) const {
    if (kind_ != other.kind_) return false;
    switch (kind_) {
        case IRValueKind::None: return true;
        case IRValueKind::Register: return id_ == other.id_;
        case IRValueKind::ConstantInt: return int_val_ == other.int_val_;
        case IRValueKind::ConstantFloat: return float_val_ == other.float_val_;
        case IRValueKind::ConstantBool: return bool_val_ == other.bool_val_;
        case IRValueKind::Parameter: return str_val_ == other.str_val_;
    }
    return false;
}

std::string IRInstruction::to_string() const {
    std::ostringstream ss;

    if (opcode_ == IROpcode::Branch) {
        ss << "br " << true_label_;
        return ss.str();
    }
    if (opcode_ == IROpcode::BranchCond) {
        ss << "br_cond ";
        if (!operands_.empty()) {
            ss << operands_[0].to_string();
        }
        ss << ", " << true_label_ << ", " << false_label_;
        return ss.str();
    }
    if (opcode_ == IROpcode::Return) {
        ss << "ret";
        if (!operands_.empty() && operands_[0].kind() != IRValueKind::None) {
            ss << " " << operands_[0].to_string();
        }
        return ss.str();
    }
    if (opcode_ == IROpcode::Store) {
        // store %val, %ptr
        ss << "store " << operands_[0].to_string() << ", " << operands_[1].to_string();
        return ss.str();
    }

    // Instructions with destination
    if (has_dest()) {
        ss << dest_.to_string() << " = ";
    }

    ss << ir_opcode_to_string(opcode_);

    if (opcode_ == IROpcode::SpecialReg) {
        ss << " " << special_reg_;
    } else if (opcode_ == IROpcode::Alloca) {
        if (dest_.type() && dest_.type()->element_type()) {
            ss << " " << dest_.type()->element_type()->to_string();
        }
    } else {
        for (size_t i = 0; i < operands_.size(); ++i) {
            ss << (i == 0 ? " " : ", ") << operands_[i].to_string();
        }
    }

    return ss.str();
}

void IRPrinter::print(const IRModule& module, std::ostream& out) {
    out << "; Module: " << module.name() << "\n\n";
    for (const auto& fn : module.functions()) {
        print(*fn, out);
        out << "\n";
    }
}

void IRPrinter::print(const IRFunction& fn, std::ostream& out) {
    out << "function @" << fn.name() << "(";
    for (size_t i = 0; i < fn.params().size(); ++i) {
        if (i > 0) out << ", ";
        const auto& p = fn.params()[i];
        out << p.type()->to_string() << " " << p.to_string();
    }
    out << ") {\n";

    for (const auto& bb : fn.blocks()) {
        print(*bb, out);
    }

    out << "}\n";
}

void IRPrinter::print(const IRBasicBlock& bb, std::ostream& out) {
    out << bb.label() << ":\n";
    for (const auto& inst : bb.instructions()) {
        out << "  " << inst->to_string() << "\n";
    }
}

void IRPrinter::print(const IRInstruction& inst, std::ostream& out) {
    out << inst.to_string() << "\n";
}

} // namespace minigpu
