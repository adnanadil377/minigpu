#pragma once

#include "minigpu/ir.hpp"
#include "minigpu/target.hpp"
#include "minigpu/regalloc.hpp"
#include <string>
#include <iostream>
#include <memory>

namespace minigpu {

class PTXCodeGen {
public:
    explicit PTXCodeGen(TargetArch arch = TargetArch::SM_75);

    void set_arch(TargetArch arch) { arch_ = arch; }
    void set_register_budget(int budget) { reg_budget_ = budget; }

    std::string generate(IRModule& module);

private:
    void generate_function(IRFunction& fn, std::ostream& out);

    TargetArch arch_{TargetArch::SM_75};
    int reg_budget_{32};
};

} // namespace minigpu
