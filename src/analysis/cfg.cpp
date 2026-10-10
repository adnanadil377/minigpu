#include "minigpu/cfg.hpp"
#include <queue>
#include <iostream>
#include <algorithm>

namespace minigpu {

CFG::CFG(IRFunction* fn) : fn_(fn) {
    build();
}

void CFG::build() {
    if (!fn_) return;

    // Clear existing edges
    for (auto& bb : fn_->blocks()) {
        bb->clear_cfg_edges();
    }

    // Connect successors and predecessors
    for (auto& bb : fn_->blocks()) {
        auto term = bb->get_terminator();
        if (!term) continue;

        if (term->opcode() == IROpcode::Branch) {
            auto target = fn_->find_block(term->true_label());
            if (target) {
                bb->add_successor(target);
                target->add_predecessor(bb.get());
            }
        } else if (term->opcode() == IROpcode::BranchCond) {
            auto target_true = fn_->find_block(term->true_label());
            if (target_true) {
                bb->add_successor(target_true);
                target_true->add_predecessor(bb.get());
            }
            auto target_false = fn_->find_block(term->false_label());
            if (target_false && target_false != target_true) {
                bb->add_successor(target_false);
                target_false->add_predecessor(bb.get());
            }
        }
    }

    compute_reachability();
    compute_block_dataflow();
}

void CFG::compute_reachability() {
    reachable_.clear();
    auto entry = fn_->entry_block();
    if (!entry) return;

    std::queue<IRBasicBlock*> worklist;
    worklist.push(entry);
    reachable_.insert(entry->label());

    while (!worklist.empty()) {
        auto curr = worklist.front();
        worklist.pop();

        for (auto succ : curr->successors()) {
            if (reachable_.find(succ->label()) == reachable_.end()) {
                reachable_.insert(succ->label());
                worklist.push(succ);
            }
        }
    }
}

void CFG::remove_unreachable_blocks() {
    compute_reachability();
    auto& blocks = fn_->blocks();

    blocks.erase(
        std::remove_if(blocks.begin(), blocks.end(),
            [this](const std::unique_ptr<IRBasicBlock>& bb) {
                return reachable_.find(bb->label()) == reachable_.end();
            }),
        blocks.end()
    );

    // Rebuild edges
    build();
}

void CFG::compute_block_dataflow() {
    dataflow_info_.clear();

    for (const auto& bb : fn_->blocks()) {
        BlockDataFlowInfo info;

        for (const auto& inst : bb->instructions()) {
            // Check operands used
            for (const auto& op : inst->operands()) {
                if (op.is_reg()) {
                    int reg = op.reg_id();
                    // If not defined earlier in this block, it's a USE
                    if (info.def.find(reg) == info.def.end()) {
                        info.use.insert(reg);
                    }
                }
            }

            // Check dest defined
            if (inst->has_dest() && inst->dest().is_reg()) {
                info.def.insert(inst->dest().reg_id());
            }
        }

        dataflow_info_[bb->label()] = std::move(info);
    }
}

const BlockDataFlowInfo& CFG::get_dataflow_info(const std::string& bb_label) const {
    static BlockDataFlowInfo empty;
    auto it = dataflow_info_.find(bb_label);
    if (it != dataflow_info_.end()) return it->second;
    return empty;
}

void CFG::dump_text(std::ostream& out) const {
    out << "CFG for @" << fn_->name() << ":\n";
    for (const auto& bb : fn_->blocks()) {
        out << "  Block [" << bb->label() << "]:\n";
        out << "    Predecessors: ";
        for (size_t i = 0; i < bb->predecessors().size(); ++i) {
            if (i > 0) out << ", ";
            out << bb->predecessors()[i]->label();
        }
        out << "\n";

        out << "    Successors:   ";
        for (size_t i = 0; i < bb->successors().size(); ++i) {
            if (i > 0) out << ", ";
            out << bb->successors()[i]->label();
        }
        out << "\n";

        auto it = dataflow_info_.find(bb->label());
        if (it != dataflow_info_.end()) {
            out << "    USE: { ";
            for (int r : it->second.use) out << "%v" << r << " ";
            out << "}\n";
            out << "    DEF: { ";
            for (int r : it->second.def) out << "%v" << r << " ";
            out << "}\n";
        }
    }
}

void CFG::dump_dot(std::ostream& out) const {
    out << "digraph " << fn_->name() << " {\n";
    out << "  node [shape=box, fontname=\"Courier\"];\n";

    for (const auto& bb : fn_->blocks()) {
        out << "  " << bb->label() << " [label=\"" << bb->label() << "\\n";
        for (const auto& inst : bb->instructions()) {
            std::string s = inst->to_string();
            // Escape quotes and backslashes
            std::string escaped;
            for (char c : s) {
                if (c == '"' || c == '\\') escaped += '\\';
                escaped += c;
            }
            out << escaped << "\\l";
        }
        out << "\"];\n";

        for (auto succ : bb->successors()) {
            out << "  " << bb->label() << " -> " << succ->label() << ";\n";
        }
    }

    out << "}\n";
}

} // namespace minigpu
