#pragma once

#include "minigpu/ir.hpp"
#include "minigpu/cfg.hpp"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>

namespace minigpu {

struct LiveInterval {
    int reg_id{-1};
    int start_point{0};
    int end_point{0};
    std::shared_ptr<Type> type;

    bool overlaps(const LiveInterval& other) const {
        return !(end_point < other.start_point || other.end_point < start_point);
    }
};

class LivenessAnalysis {
public:
    explicit LivenessAnalysis(IRFunction* fn);

    void run();

    const std::unordered_map<std::string, BlockDataFlowInfo>& block_liveness() const {
        return block_liveness_;
    }

    const std::unordered_map<int, LiveInterval>& intervals() const {
        return intervals_;
    }

    void dump(std::ostream& out) const;

private:
    void compute_block_liveness();
    void compute_instruction_intervals();

    IRFunction* fn_;
    CFG cfg_;
    std::unordered_map<std::string, BlockDataFlowInfo> block_liveness_;
    std::unordered_map<int, LiveInterval> intervals_;
};

} // namespace minigpu
