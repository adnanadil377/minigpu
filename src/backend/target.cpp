#include "minigpu/target.hpp"
#include <sstream>

namespace minigpu {

std::string target_arch_to_string(TargetArch arch) {
    switch (arch) {
        case TargetArch::SM_70: return "sm_70";
        case TargetArch::SM_75: return "sm_75";
        case TargetArch::SM_80: return "sm_80";
        case TargetArch::SM_86: return "sm_86";
        case TargetArch::SM_90: return "sm_90";
    }
    return "sm_75";
}

TargetArch parse_target_arch(const std::string& str) {
    if (str == "sm_70") return TargetArch::SM_70;
    if (str == "sm_75") return TargetArch::SM_75;
    if (str == "sm_80") return TargetArch::SM_80;
    if (str == "sm_86") return TargetArch::SM_86;
    if (str == "sm_90") return TargetArch::SM_90;
    return TargetArch::SM_75;
}

std::string PTXOperand::to_string() const {
    switch (kind) {
        case Kind::Reg: {
            switch (reg_kind) {
                case PTXRegKind::Pred: return "%p" + std::to_string(reg_id);
                case PTXRegKind::B32:  return "%r" + std::to_string(reg_id);
                case PTXRegKind::F32:  return "%f" + std::to_string(reg_id);
                case PTXRegKind::B64:  return "%rd" + std::to_string(reg_id);
            }
            break;
        }
        case Kind::ImmInt: return std::to_string(imm_int);
        case Kind::ImmFloat: {
            std::ostringstream ss;
            ss << "0f" << std::hexfloat << (float)imm_float;
            return ss.str();
        }
        case Kind::ImmBool: return imm_bool ? "1" : "0";
        case Kind::Param: return "[" + text + "]";
        case Kind::Label: return text;
    }
    return "?";
}

std::string PTXInst::to_string() const {
    std::ostringstream ss;
    if (!predicate.empty()) {
        ss << predicate << " ";
    }
    ss << opcode;
    for (size_t i = 0; i < operands.size(); ++i) {
        ss << (i == 0 ? " " : ", ") << operands[i].to_string();
    }
    ss << ";";
    if (!comment.empty()) {
        ss << " // " << comment;
    }
    return ss.str();
}

} // namespace minigpu
