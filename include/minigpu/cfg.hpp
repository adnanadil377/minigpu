#pragma once

#include "minigpu/ir.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <iosfwd>

namespace minigpu {

struct BlockDataFlowInfo {
    std::unordered_set<int> use;     // Registers used before defined in block
    std::unordered_set<int> def;     // Registers defined in block
    std::unordered_set<int> live_in;  // Registers live at entry
    std::unordered_set<int> live_out; // Registers live at exit
};

class CFG {
public:
    explicit CFG(IRFunction* fn);

    void build();
    void remove_unreachable_blocks();
    void compute_block_dataflow();

    IRFunction* function() const { return fn_; }
    const std::unordered_set<std::string>& reachable_blocks() const { return reachable_; }
    const BlockDataFlowInfo& get_dataflow_info(const std::string& bb_label) const;

    void dump_text(std::ostream& out) const;
    void dump_dot(std::ostream& out) const;

private:
    void compute_reachability();

    IRFunction* fn_;
    std::unordered_set<std::string> reachable_;
    std::unordered_map<std::string, BlockDataFlowInfo> dataflow_info_;
};

} // namespace minigpu
