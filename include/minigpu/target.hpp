#pragma once

#include "minigpu/types.hpp"
#include <string>
#include <vector>

namespace minigpu {

enum class TargetArch {
    SM_70, // Volta
    SM_75, // Turing (default)
    SM_80, // Ampere
    SM_86, // Ampere
    SM_90  // Hopper
};

std::string target_arch_to_string(TargetArch arch);
TargetArch parse_target_arch(const std::string& str);

enum class PTXRegKind {
    Pred, // %p0, %p1
    B32,  // %r0, %r1 (int, bit32)
    F32,  // %f0, %f1 (float)
    B64   // %rd0, %rd1 (64-bit address)
};

struct PTXOperand {
    enum class Kind {
        Reg,
        ImmInt,
        ImmFloat,
        ImmBool,
        Param,
        Label
    };

    Kind kind{Kind::Reg};
    PTXRegKind reg_kind{PTXRegKind::B32};
    int reg_id{0};
    int64_t imm_int{0};
    double imm_float{0.0};
    bool imm_bool{false};
    std::string text;

    static PTXOperand make_reg(PTXRegKind rk, int id) {
        PTXOperand op;
        op.kind = Kind::Reg;
        op.reg_kind = rk;
        op.reg_id = id;
        return op;
    }

    static PTXOperand make_imm_int(int64_t v) {
        PTXOperand op;
        op.kind = Kind::ImmInt;
        op.imm_int = v;
        return op;
    }

    static PTXOperand make_imm_float(double v) {
        PTXOperand op;
        op.kind = Kind::ImmFloat;
        op.imm_float = v;
        return op;
    }

    static PTXOperand make_imm_bool(bool v) {
        PTXOperand op;
        op.kind = Kind::ImmBool;
        op.imm_bool = v;
        return op;
    }

    static PTXOperand make_label(std::string lbl) {
        PTXOperand op;
        op.kind = Kind::Label;
        op.text = std::move(lbl);
        return op;
    }

    static PTXOperand make_param(std::string p) {
        PTXOperand op;
        op.kind = Kind::Param;
        op.text = std::move(p);
        return op;
    }

    std::string to_string() const;
};

struct PTXInst {
    std::string predicate; // e.g. "@p0" or "@!p0"
    std::string opcode;    // e.g. "add.s32", "ld.global.f32", "bra"
    std::vector<PTXOperand> operands;
    std::string comment;

    std::string to_string() const;
};

} // namespace minigpu
