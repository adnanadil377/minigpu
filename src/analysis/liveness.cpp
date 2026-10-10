#include "minigpu/liveness.hpp"
#include <algorithm>
#include <map>

namespace minigpu {

LivenessAnalysis::LivenessAnalysis(IRFunction* fn) : fn_(fn), cfg_(fn) {}

void LivenessAnalysis::run() {
    cfg_.build();
    compute_block_liveness();
    compute_instruction_intervals();
}

void LivenessAnalysis::compute_block_liveness() {
    block_liveness_.clear();

    // Initialize USE and DEF sets from CFG
    for (const auto& bb : fn_->blocks()) {
        block_liveness_[bb->label()] = cfg_.get_dataflow_info(bb->label());
    }

    // Fixed-point iteration
    bool changed = true;
    while (changed) {
        changed = false;

        // Iterate backward or over blocks
        for (auto it = fn_->blocks().rbegin(); it != fn_->blocks().rend(); ++it) {
            auto& bb = *it;
            auto& info = block_liveness_[bb->label()];

            // LIVE_OUT = union of LIVE_IN of all successors
            std::unordered_set<int> new_live_out;
            for (auto succ : bb->successors()) {
                const auto& succ_info = block_liveness_[succ->label()];
                for (int r : succ_info.live_in) {
                    new_live_out.insert(r);
                }
            }

            // LIVE_IN = USE union (LIVE_OUT - DEF)
            std::unordered_set<int> new_live_in = info.use;
            for (int r : new_live_out) {
                if (info.def.find(r) == info.def.end()) {
                    new_live_in.insert(r);
                }
            }

            if (new_live_out != info.live_out || new_live_in != info.live_in) {
                info.live_out = std::move(new_live_out);
                info.live_in = std::move(new_live_in);
                changed = true;
            }
        }
    }
}

void LivenessAnalysis::compute_instruction_intervals() {
    intervals_.clear();

    // Map instruction positions
    int inst_idx = 0;
    std::unordered_map<int, int> def_points;
    std::unordered_map<int, int> last_use_points;
    std::unordered_map<int, std::shared_ptr<Type>> reg_types;

    for (const auto& bb : fn_->blocks()) {
        for (const auto& inst : bb->instructions()) {
            if (inst->has_dest() && inst->dest().is_reg()) {
                int r = inst->dest().reg_id();
                if (def_points.find(r) == def_points.end()) {
                    def_points[r] = inst_idx;
                    reg_types[r] = inst->dest().type();
                }
            }

            for (const auto& op : inst->operands()) {
                if (op.is_reg()) {
                    int r = op.reg_id();
                    last_use_points[r] = inst_idx;
                    if (reg_types.find(r) == reg_types.end()) {
                        reg_types[r] = op.type();
                    }
                }
            }
            inst_idx++;
        }
    }

    // Build intervals
    std::unordered_set<int> all_regs;
    for (const auto& p : def_points) all_regs.insert(p.first);
    for (const auto& p : last_use_points) all_regs.insert(p.first);

    for (int r : all_regs) {
        int start = def_points.count(r) ? def_points[r] : 0;
        int end = last_use_points.count(r) ? last_use_points[r] : start;
        intervals_[r] = {r, start, std::max(start, end), reg_types[r]};
    }
}

void LivenessAnalysis::dump(std::ostream& out) const {
    out << "Liveness Analysis for @" << fn_->name() << ":\n\n";

    out << "--- Basic Block Liveness Sets ---\n";
    for (const auto& bb : fn_->blocks()) {
        auto it = block_liveness_.find(bb->label());
        if (it == block_liveness_.end()) continue;
        const auto& info = it->second;

        out << "Block [" << bb->label() << "]:\n";

        out << "  LIVE_IN:  { ";
        std::vector<int> in_sorted(info.live_in.begin(), info.live_in.end());
        std::sort(in_sorted.begin(), in_sorted.end());
        for (int r : in_sorted) out << "%v" << r << " ";
        out << "}\n";

        out << "  DEF:      { ";
        std::vector<int> def_sorted(info.def.begin(), info.def.end());
        std::sort(def_sorted.begin(), def_sorted.end());
        for (int r : def_sorted) out << "%v" << r << " ";
        out << "}\n";

        out << "  USE:      { ";
        std::vector<int> use_sorted(info.use.begin(), info.use.end());
        std::sort(use_sorted.begin(), use_sorted.end());
        for (int r : use_sorted) out << "%v" << r << " ";
        out << "}\n";

        out << "  LIVE_OUT: { ";
        std::vector<int> out_sorted(info.live_out.begin(), info.live_out.end());
        std::sort(out_sorted.begin(), out_sorted.end());
        for (int r : out_sorted) out << "%v" << r << " ";
        out << "}\n\n";
    }

    out << "--- Virtual Register Live Intervals ---\n";
    std::map<int, LiveInterval> sorted_intervals(intervals_.begin(), intervals_.end());
    for (const auto& pair : sorted_intervals) {
        const auto& iv = pair.second;
        out << "  %v" << iv.reg_id << ": [" << iv.start_point << ", " << iv.end_point
            << "] (" << (iv.type ? iv.type->to_string() : "unknown") << ")\n";
    }
}

} // namespace minigpu
