#pragma once

#include "minigpu/ir.hpp"
#include "minigpu/liveness.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <iostream>

namespace minigpu {

struct RegisterAssignment {
    int phys_reg{-1};      // Physical register index (e.g. 0 -> R0)
    bool is_spilled{false};
    int spill_slot{-1};    // Spill slot offset/index if spilled
};

class RegisterAllocator {
public:
    RegisterAllocator(IRFunction* fn, int register_budget = 16);

    void allocate();
    bool verify() const;

    const std::unordered_map<int, std::unordered_set<int>>& interference_graph() const {
        return adj_list_;
    }

    const std::unordered_map<int, RegisterAssignment>& assignment() const {
        return assignment_;
    }

    int register_budget() const { return k_budget_; }
    int num_spills() const { return spill_count_; }
    int num_physical_used() const { return max_phys_used_; }

    void dump(std::ostream& out) const;

private:
    void build_interference_graph();
    void color_graph();

    IRFunction* fn_;
    int k_budget_;
    LivenessAnalysis liveness_;

    std::unordered_map<int, std::unordered_set<int>> adj_list_;
    std::unordered_map<int, RegisterAssignment> assignment_;
    int spill_count_{0};
    int max_phys_used_{0};
};

} // namespace minigpu
