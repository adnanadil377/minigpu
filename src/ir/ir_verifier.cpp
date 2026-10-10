#include "minigpu/ir_verifier.hpp"
#include <unordered_set>

namespace minigpu {

void IRVerifier::add_error(const SourceLocation& loc, const std::string& msg) {
    diagnostics_.push_back({loc, msg, true});
    has_errors_ = true;
}

bool IRVerifier::verify_module(const IRModule& module) {
    has_errors_ = false;
    diagnostics_.clear();

    for (const auto& fn : module.functions()) {
        verify_function(*fn);
    }

    return !has_errors_;
}

bool IRVerifier::verify_function(const IRFunction& fn) {
    std::unordered_set<std::string> block_labels;
    for (const auto& bb : fn.blocks()) {
        if (!block_labels.insert(bb->label()).second) {
            add_error({}, "duplicate basic block label '" + bb->label() + "' in function @" + fn.name());
        }
    }

    std::unordered_set<int> defined_regs;

    for (const auto& bb : fn.blocks()) {
        if (bb->instructions().empty()) {
            add_error({}, "basic block [" + bb->label() + "] is empty");
            continue;
        }

        // Verify terminator is at end and nowhere else
        for (size_t i = 0; i < bb->instructions().size(); ++i) {
            const auto& inst = bb->instructions()[i];
            bool is_last = (i == bb->instructions().size() - 1);

            if (inst->is_terminator() && !is_last) {
                add_error(inst->loc(), "terminator instruction appears before end of block [" + bb->label() + "]");
            }

            if (is_last && !inst->is_terminator()) {
                add_error(inst->loc(), "basic block [" + bb->label() + "] does not end with a terminator");
            }

            // Verify branch targets
            if (inst->opcode() == IROpcode::Branch) {
                if (block_labels.find(inst->true_label()) == block_labels.end()) {
                    add_error(inst->loc(), "branch target '" + inst->true_label() + "' does not exist");
                }
            } else if (inst->opcode() == IROpcode::BranchCond) {
                if (block_labels.find(inst->true_label()) == block_labels.end()) {
                    add_error(inst->loc(), "conditional branch true target '" + inst->true_label() + "' does not exist");
                }
                if (block_labels.find(inst->false_label()) == block_labels.end()) {
                    add_error(inst->loc(), "conditional branch false target '" + inst->false_label() + "' does not exist");
                }
            }

            // Register definitions
            if (inst->has_dest() && inst->dest().is_reg()) {
                defined_regs.insert(inst->dest().reg_id());
            }
        }
    }

    return !has_errors_;
}

} // namespace minigpu
