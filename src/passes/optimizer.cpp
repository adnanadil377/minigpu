#include "minigpu/optimizer.hpp"
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <cmath>

namespace minigpu {

// --- Constant Folding ---

bool ConstantFoldingPass::run_on_function(IRFunction& fn) {
    bool changed = false;

    for (auto& bb : fn.blocks()) {
        for (auto& inst : bb->instructions()) {
            if (!inst->has_dest() || inst->operands().size() < 2) continue;

            const auto& op0 = inst->operands()[0];
            const auto& op1 = inst->operands()[1];

            if (op0.kind() == IRValueKind::ConstantInt && op1.kind() == IRValueKind::ConstantInt) {
                int64_t v0 = op0.int_val();
                int64_t v1 = op1.int_val();
                bool folded = false;
                int64_t res = 0;
                bool bool_res = false;
                bool is_bool = false;

                switch (inst->opcode()) {
                    case IROpcode::Add: res = v0 + v1; folded = true; break;
                    case IROpcode::Sub: res = v0 - v1; folded = true; break;
                    case IROpcode::Mul: res = v0 * v1; folded = true; break;
                    case IROpcode::SDiv: if (v1 != 0) { res = v0 / v1; folded = true; } break;
                    case IROpcode::SRem: if (v1 != 0) { res = v0 % v1; folded = true; } break;
                    case IROpcode::ICmpEQ: bool_res = (v0 == v1); is_bool = true; folded = true; break;
                    case IROpcode::ICmpNE: bool_res = (v0 != v1); is_bool = true; folded = true; break;
                    case IROpcode::ICmpSLT: bool_res = (v0 < v1); is_bool = true; folded = true; break;
                    case IROpcode::ICmpSLE: bool_res = (v0 <= v1); is_bool = true; folded = true; break;
                    case IROpcode::ICmpSGT: bool_res = (v0 > v1); is_bool = true; folded = true; break;
                    case IROpcode::ICmpSGE: bool_res = (v0 >= v1); is_bool = true; folded = true; break;
                    default: break;
                }

                if (folded) {
                    inst->set_opcode(IROpcode::Copy);
                    if (is_bool) {
                        inst->operands() = {IRValue::make_bool_const(bool_res)};
                    } else {
                        inst->operands() = {IRValue::make_int_const(res)};
                    }
                    changed = true;
                }
            } else if (op0.kind() == IRValueKind::ConstantFloat && op1.kind() == IRValueKind::ConstantFloat) {
                double v0 = op0.float_val();
                double v1 = op1.float_val();
                bool folded = false;
                double res = 0.0;
                bool bool_res = false;
                bool is_bool = false;

                switch (inst->opcode()) {
                    case IROpcode::FAdd: res = v0 + v1; folded = true; break;
                    case IROpcode::FSub: res = v0 - v1; folded = true; break;
                    case IROpcode::FMul: res = v0 * v1; folded = true; break;
                    case IROpcode::FDiv: if (v1 != 0.0) { res = v0 / v1; folded = true; } break;
                    case IROpcode::FCmpEQ: bool_res = (v0 == v1); is_bool = true; folded = true; break;
                    case IROpcode::FCmpNE: bool_res = (v0 != v1); is_bool = true; folded = true; break;
                    case IROpcode::FCmpOLT: bool_res = (v0 < v1); is_bool = true; folded = true; break;
                    case IROpcode::FCmpOLE: bool_res = (v0 <= v1); is_bool = true; folded = true; break;
                    case IROpcode::FCmpOGT: bool_res = (v0 > v1); is_bool = true; folded = true; break;
                    case IROpcode::FCmpOGE: bool_res = (v0 >= v1); is_bool = true; folded = true; break;
                    default: break;
                }

                if (folded) {
                    inst->set_opcode(IROpcode::Copy);
                    if (is_bool) {
                        inst->operands() = {IRValue::make_bool_const(bool_res)};
                    } else {
                        inst->operands() = {IRValue::make_float_const(res)};
                    }
                    changed = true;
                }
            } else if (inst->opcode() == IROpcode::FMA && inst->operands().size() == 3) {
                const auto& o0 = inst->operands()[0];
                const auto& o1 = inst->operands()[1];
                const auto& o2 = inst->operands()[2];
                if (o0.kind() == IRValueKind::ConstantFloat &&
                    o1.kind() == IRValueKind::ConstantFloat &&
                    o2.kind() == IRValueKind::ConstantFloat) {
                    double res = std::fma(o0.float_val(), o1.float_val(), o2.float_val());
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {IRValue::make_float_const(res)};
                    changed = true;
                }
            }
        }
    }
    return changed;
}

// --- Constant Propagation ---

bool ConstantPropagationPass::run_on_function(IRFunction& fn) {
    bool changed = false;
    std::unordered_map<int, IRValue> const_map;

    // Collect copy constants
    for (auto& bb : fn.blocks()) {
        for (auto& inst : bb->instructions()) {
            if (inst->opcode() == IROpcode::Copy && inst->has_dest() && inst->dest().is_reg()) {
                if (!inst->operands().empty() && inst->operands()[0].is_const()) {
                    const_map[inst->dest().reg_id()] = inst->operands()[0];
                }
            }
        }
    }

    if (const_map.empty()) return false;

    // Substitute uses
    for (auto& bb : fn.blocks()) {
        for (auto& inst : bb->instructions()) {
            for (size_t i = 0; i < inst->operands().size(); ++i) {
                auto& op = inst->operands()[i];
                if (op.is_reg()) {
                    auto it = const_map.find(op.reg_id());
                    if (it != const_map.end()) {
                        op = it->second;
                        changed = true;
                    }
                }
            }
        }
    }

    return changed;
}

// --- Algebraic Simplification ---

bool AlgebraicSimplificationPass::run_on_function(IRFunction& fn) {
    bool changed = false;

    for (auto& bb : fn.blocks()) {
        for (auto& inst : bb->instructions()) {
            if (!inst->has_dest() || inst->operands().size() < 2) continue;

            const auto& op0 = inst->operands()[0];
            const auto& op1 = inst->operands()[1];

            // x + 0 -> x
            if (inst->opcode() == IROpcode::Add) {
                if (op1.kind() == IRValueKind::ConstantInt && op1.int_val() == 0) {
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {op0};
                    changed = true;
                } else if (op0.kind() == IRValueKind::ConstantInt && op0.int_val() == 0) {
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {op1};
                    changed = true;
                }
            }
            // x + 0.0 -> x
            else if (inst->opcode() == IROpcode::FAdd) {
                if (op1.kind() == IRValueKind::ConstantFloat && op1.float_val() == 0.0) {
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {op0};
                    changed = true;
                } else if (op0.kind() == IRValueKind::ConstantFloat && op0.float_val() == 0.0) {
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {op1};
                    changed = true;
                }
            }
            // x - 0 -> x
            else if (inst->opcode() == IROpcode::Sub) {
                if (op1.kind() == IRValueKind::ConstantInt && op1.int_val() == 0) {
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {op0};
                    changed = true;
                } else if (op0.is_reg() && op1.is_reg() && op0.reg_id() == op1.reg_id()) {
                    // x - x -> 0
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {IRValue::make_int_const(0)};
                    changed = true;
                }
            }
            // x * 1 -> x, x * 0 -> 0
            else if (inst->opcode() == IROpcode::Mul) {
                if (op1.kind() == IRValueKind::ConstantInt && op1.int_val() == 1) {
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {op0};
                    changed = true;
                } else if (op0.kind() == IRValueKind::ConstantInt && op0.int_val() == 1) {
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {op1};
                    changed = true;
                } else if (op1.kind() == IRValueKind::ConstantInt && op1.int_val() == 0) {
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {IRValue::make_int_const(0)};
                    changed = true;
                } else if (op0.kind() == IRValueKind::ConstantInt && op0.int_val() == 0) {
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {IRValue::make_int_const(0)};
                    changed = true;
                }
            }
            // x * 1.0 -> x
            else if (inst->opcode() == IROpcode::FMul) {
                if (op1.kind() == IRValueKind::ConstantFloat && op1.float_val() == 1.0) {
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {op0};
                    changed = true;
                } else if (op0.kind() == IRValueKind::ConstantFloat && op0.float_val() == 1.0) {
                    inst->set_opcode(IROpcode::Copy);
                    inst->operands() = {op1};
                    changed = true;
                }
            }
        }
    }
    return changed;
}

// --- Common Subexpression Elimination ---

bool CommonSubexpressionEliminationPass::run_on_function(IRFunction& fn) {
    bool changed = false;

    for (auto& bb : fn.blocks()) {
        struct ExprKey {
            int op;
            std::string op0;
            std::string op1;
            bool operator==(const ExprKey& o) const {
                return op == o.op && op0 == o.op0 && op1 == o.op1;
            }
        };

        struct ExprHasher {
            size_t operator()(const ExprKey& k) const {
                return std::hash<int>()(k.op) ^ (std::hash<std::string>()(k.op0) << 1) ^ (std::hash<std::string>()(k.op1) << 2);
            }
        };

        std::unordered_map<ExprKey, IRValue, ExprHasher> available_exprs;

        for (auto& inst : bb->instructions()) {
            if (!inst->has_dest() || inst->operands().size() < 2) continue;

            // Only pure binary expressions
            switch (inst->opcode()) {
                case IROpcode::Add:
                case IROpcode::Sub:
                case IROpcode::Mul:
                case IROpcode::SDiv:
                case IROpcode::FAdd:
                case IROpcode::FSub:
                case IROpcode::FMul:
                case IROpcode::FDiv:
                case IROpcode::GetElementPtr: {
                    ExprKey key{
                        static_cast<int>(inst->opcode()),
                        inst->operands()[0].to_string(),
                        inst->operands()[1].to_string()
                    };

                    auto it = available_exprs.find(key);
                    if (it != available_exprs.end()) {
                        // Replace with copy from earlier result
                        inst->set_opcode(IROpcode::Copy);
                        inst->operands() = {it->second};
                        changed = true;
                    } else {
                        available_exprs[key] = inst->dest();
                    }
                    break;
                }
                default: break;
            }
        }
    }

    return changed;
}

// --- Mem2Reg (Forward local alloca loads) ---

bool Mem2RegPass::run_on_function(IRFunction& fn) {
    bool changed = false;

    for (auto& bb : fn.blocks()) {
        std::unordered_map<int, IRValue> store_map;

        for (auto& inst : bb->instructions()) {
            if (inst->opcode() == IROpcode::Store) {
                // store %val, %ptr
                const auto& val = inst->operands()[0];
                const auto& ptr = inst->operands()[1];
                if (ptr.is_reg()) {
                    store_map[ptr.reg_id()] = val;
                }
            } else if (inst->opcode() == IROpcode::Load) {
                // %loaded = load %ptr
                const auto& ptr = inst->operands()[0];
                if (ptr.is_reg()) {
                    auto it = store_map.find(ptr.reg_id());
                    if (it != store_map.end()) {
                        inst->set_opcode(IROpcode::Copy);
                        inst->operands() = {it->second};
                        changed = true;
                    }
                }
            }
        }
    }

    return changed;
}

// --- Fused Multiply-Add (FMA) Contraction ---
// Fuses (a * b) + c or c + (a * b) into a single hardware GPU instruction: fma(a, b, c)

bool FMAContractionPass::run_on_function(IRFunction& fn) {
    bool changed = false;

    for (auto& bb : fn.blocks()) {
        std::unordered_map<int, IRInstruction*> fmul_defs;

        for (auto& inst : bb->instructions()) {
            if (inst->opcode() == IROpcode::FMul && inst->has_dest() && inst->dest().is_reg()) {
                fmul_defs[inst->dest().reg_id()] = inst.get();
                continue;
            }

            if (inst->opcode() == IROpcode::FAdd && inst->operands().size() == 2) {
                const auto& op0 = inst->operands()[0];
                const auto& op1 = inst->operands()[1];

                // Case 1: op0 is produced by FMul: (a * b) + c
                if (op0.is_reg() && fmul_defs.count(op0.reg_id())) {
                    auto* mul = fmul_defs[op0.reg_id()];
                    if (mul->operands().size() == 2) {
                        IRValue a = mul->operands()[0];
                        IRValue b = mul->operands()[1];
                        IRValue c = op1;

                        inst->set_opcode(IROpcode::FMA);
                        inst->operands() = {a, b, c};
                        changed = true;
                        continue;
                    }
                }

                // Case 2: op1 is produced by FMul: c + (a * b)
                if (op1.is_reg() && fmul_defs.count(op1.reg_id())) {
                    auto* mul = fmul_defs[op1.reg_id()];
                    if (mul->operands().size() == 2) {
                        IRValue a = mul->operands()[0];
                        IRValue b = mul->operands()[1];
                        IRValue c = op0;

                        inst->set_opcode(IROpcode::FMA);
                        inst->operands() = {a, b, c};
                        changed = true;
                        continue;
                    }
                }
            }
        }
    }

    return changed;
}

// --- Dead Code Elimination ---

bool DeadCodeEliminationPass::run_on_function(IRFunction& fn) {
    bool any_changed = false;
    bool changed = true;

    while (changed) {
        changed = false;

        // Collect all used registers
        std::unordered_set<int> used_regs;
        for (auto& bb : fn.blocks()) {
            for (auto& inst : bb->instructions()) {
                for (const auto& op : inst->operands()) {
                    if (op.is_reg()) {
                        used_regs.insert(op.reg_id());
                    }
                }
            }
        }

        // Remove dead instructions
        for (auto& bb : fn.blocks()) {
            auto& instructions = bb->instructions();
            for (auto it = instructions.begin(); it != instructions.end();) {
                auto& inst = *it;

                // Instructions with side effects cannot be removed
                if (inst->is_terminator() ||
                    inst->opcode() == IROpcode::Store ||
                    inst->opcode() == IROpcode::SpecialReg) {
                    ++it;
                    continue;
                }

                // If instruction defines a register that is never used, remove it!
                if (inst->has_dest() && inst->dest().is_reg()) {
                    int dest_id = inst->dest().reg_id();
                    if (used_regs.find(dest_id) == used_regs.end()) {
                        it = instructions.erase(it);
                        changed = true;
                        any_changed = true;
                        continue;
                    }
                }
                ++it;
            }
        }
    }

    return any_changed;
}

// --- Pass Manager ---

void PassManager::add_default_passes() {
    add_pass(std::make_unique<Mem2RegPass>());
    add_pass(std::make_unique<ConstantPropagationPass>());
    add_pass(std::make_unique<ConstantFoldingPass>());
    add_pass(std::make_unique<AlgebraicSimplificationPass>());
    add_pass(std::make_unique<CommonSubexpressionEliminationPass>());
    add_pass(std::make_unique<FMAContractionPass>());
    add_pass(std::make_unique<ConstantPropagationPass>());
    add_pass(std::make_unique<DeadCodeEliminationPass>());
}

void PassManager::run(IRModule& module, std::ostream& out) {
    for (auto& fn : module.functions()) {
        for (auto& pass : passes_) {
            if (dump_passes_) {
                out << "; --- Before pass: " << pass->name() << " (@" << fn->name() << ") ---\n";
                IRPrinter::print(*fn, out);
            }

            bool changed = pass->run_on_function(*fn);

            if (dump_passes_) {
                out << "; --- After pass: " << pass->name() << " (changed: " << (changed ? "true" : "false") << ") ---\n";
                IRPrinter::print(*fn, out);
                out << "\n";
            }
        }
    }
}

} // namespace minigpu
