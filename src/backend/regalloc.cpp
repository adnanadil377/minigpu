#include "minigpu/regalloc.hpp"
#include <stack>
#include <algorithm>
#include <map>

namespace minigpu {

RegisterAllocator::RegisterAllocator(IRFunction* fn, int register_budget)
    : fn_(fn), k_budget_(register_budget), liveness_(fn) {}

void RegisterAllocator::allocate() {
    build_interference_graph();
    color_graph();
}

void RegisterAllocator::build_interference_graph() {
    liveness_.run();
    adj_list_.clear();

    const auto& intervals = liveness_.intervals();

    // Initialize nodes
    for (const auto& pair : intervals) {
        adj_list_[pair.first] = std::unordered_set<int>{};
    }

    // Add edges for overlapping intervals
    for (auto it1 = intervals.begin(); it1 != intervals.end(); ++it1) {
        for (auto it2 = std::next(it1); it2 != intervals.end(); ++it2) {
            if (it1->second.overlaps(it2->second)) {
                adj_list_[it1->first].insert(it2->first);
                adj_list_[it2->first].insert(it1->first);
            }
        }
    }
}

void RegisterAllocator::color_graph() {
    assignment_.clear();
    spill_count_ = 0;
    max_phys_used_ = 0;

    if (adj_list_.empty()) return;

    // Working copies for simplification
    std::unordered_map<int, int> degrees;
    std::unordered_set<int> active_nodes;
    for (const auto& pair : adj_list_) {
        degrees[pair.first] = static_cast<int>(pair.second.size());
        active_nodes.insert(pair.first);
    }

    std::vector<int> select_stack;

    // Simplify & Spill loop
    while (!active_nodes.empty()) {
        // Look for degree < K
        int chosen = -1;
        for (int node : active_nodes) {
            if (degrees[node] < k_budget_) {
                chosen = node;
                break;
            }
        }

        // If none has degree < K, pick spill candidate (highest degree)
        if (chosen == -1) {
            int max_deg = -1;
            for (int node : active_nodes) {
                if (degrees[node] > max_deg) {
                    max_deg = degrees[node];
                    chosen = node;
                }
            }
        }

        // Remove chosen from active graph
        active_nodes.erase(chosen);
        select_stack.push_back(chosen);

        for (int neighbor : adj_list_[chosen]) {
            if (active_nodes.find(neighbor) != active_nodes.end()) {
                degrees[neighbor]--;
            }
        }
    }

    // Select phase (assign colors in reverse stack order)
    std::reverse(select_stack.begin(), select_stack.end());

    for (int node : select_stack) {
        // Collect colors used by colored neighbors
        std::vector<bool> used_colors(k_budget_, false);

        for (int neighbor : adj_list_[node]) {
            auto it = assignment_.find(neighbor);
            if (it != assignment_.end() && !it->second.is_spilled) {
                int c = it->second.phys_reg;
                if (c >= 0 && c < k_budget_) {
                    used_colors[c] = true;
                }
            }
        }

        // Find first available color
        int assigned_color = -1;
        for (int c = 0; c < k_budget_; ++c) {
            if (!used_colors[c]) {
                assigned_color = c;
                break;
            }
        }

        if (assigned_color != -1) {
            assignment_[node] = {assigned_color, false, -1};
            if (assigned_color + 1 > max_phys_used_) {
                max_phys_used_ = assigned_color + 1;
            }
        } else {
            // Must spill
            assignment_[node] = {-1, true, spill_count_++};
        }
    }
}

bool RegisterAllocator::verify() const {
    for (const auto& pair : adj_list_) {
        int u = pair.first;
        auto it_u = assignment_.find(u);
        if (it_u == assignment_.end() || it_u->second.is_spilled) continue;

        for (int v : pair.second) {
            auto it_v = assignment_.find(v);
            if (it_v == assignment_.end() || it_v->second.is_spilled) continue;

            if (it_u->second.phys_reg == it_v->second.phys_reg) {
                return false; // Interference violation!
            }
        }
    }
    return true;
}

void RegisterAllocator::dump(std::ostream& out) const {
    out << "Register Allocation Report for @" << fn_->name() << ":\n";
    out << "  Register Budget (K): " << k_budget_ << "\n";
    out << "  Physical Registers Used: " << max_phys_used_ << " / " << k_budget_ << "\n";
    out << "  Spill Count: " << spill_count_ << "\n\n";

    out << "--- Interference Graph ---\n";
    std::map<int, std::unordered_set<int>> sorted_adj(adj_list_.begin(), adj_list_.end());
    for (const auto& pair : sorted_adj) {
        out << "  %v" << pair.first << " (degree " << pair.second.size() << ") conflicts with: { ";
        std::vector<int> n_sorted(pair.second.begin(), pair.second.end());
        std::sort(n_sorted.begin(), n_sorted.end());
        for (int n : n_sorted) out << "%v" << n << " ";
        out << "}\n";
    }
    out << "\n";

    out << "--- Register Assignments ---\n";
    std::map<int, RegisterAssignment> sorted_assign(assignment_.begin(), assignment_.end());
    for (const auto& pair : sorted_assign) {
        out << "  %v" << pair.first << " -> ";
        if (pair.second.is_spilled) {
            out << "[SPILLED: stack_slot_" << pair.second.spill_slot << "]\n";
        } else {
            out << "R" << pair.second.phys_reg << "\n";
        }
    }
    out << "\n";

    out << "  Verification: " << (verify() ? "PASSED (no conflicts)" : "FAILED (conflict detected!)") << "\n";
}

} // namespace minigpu
