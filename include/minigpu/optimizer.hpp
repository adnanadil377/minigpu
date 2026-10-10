#pragma once

#include "minigpu/ir.hpp"
#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace minigpu {

class Pass {
public:
    virtual ~Pass() = default;
    virtual std::string name() const = 0;
    virtual bool run_on_function(IRFunction& fn) = 0;
};

class ConstantFoldingPass : public Pass {
public:
    std::string name() const override { return "ConstantFolding"; }
    bool run_on_function(IRFunction& fn) override;
};

class ConstantPropagationPass : public Pass {
public:
    std::string name() const override { return "ConstantPropagation"; }
    bool run_on_function(IRFunction& fn) override;
};

class AlgebraicSimplificationPass : public Pass {
public:
    std::string name() const override { return "AlgebraicSimplification"; }
    bool run_on_function(IRFunction& fn) override;
};

class CommonSubexpressionEliminationPass : public Pass {
public:
    std::string name() const override { return "CommonSubexpressionElimination"; }
    bool run_on_function(IRFunction& fn) override;
};

class DeadCodeEliminationPass : public Pass {
public:
    std::string name() const override { return "DeadCodeElimination"; }
    bool run_on_function(IRFunction& fn) override;
};

class Mem2RegPass : public Pass {
public:
    std::string name() const override { return "Mem2Reg"; }
    bool run_on_function(IRFunction& fn) override;
};

class PassManager {
public:
    PassManager() = default;

    void add_pass(std::unique_ptr<Pass> pass) {
        passes_.push_back(std::move(pass));
    }

    void add_default_passes();

    void set_dump_passes(bool dump) { dump_passes_ = dump; }
    void run(IRModule& module, std::ostream& out = std::cout);

private:
    std::vector<std::unique_ptr<Pass>> passes_;
    bool dump_passes_{false};
};

} // namespace minigpu
