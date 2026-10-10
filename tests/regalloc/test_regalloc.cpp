#include "minigpu/ir.hpp"
#include "minigpu/liveness.hpp"
#include "minigpu/regalloc.hpp"
#include <cassert>
#include <iostream>

using namespace minigpu;

void test_liveness_and_coloring() {
    auto fn = std::make_unique<IRFunction>("test_ra", Type::getVoid());
    auto bb = fn->create_block("entry");

    // %v0 = 10
    // %v1 = 20
    // %v2 = %v0 + %v1
    // ret %v2
    auto v0 = fn->create_register(Type::getInt(), "v0");
    auto v1 = fn->create_register(Type::getInt(), "v1");
    auto v2 = fn->create_register(Type::getInt(), "v2");

    bb->add_instruction(std::make_unique<IRInstruction>(
        IROpcode::Copy, v0, std::vector<IRValue>{IRValue::make_int_const(10)}));
    bb->add_instruction(std::make_unique<IRInstruction>(
        IROpcode::Copy, v1, std::vector<IRValue>{IRValue::make_int_const(20)}));
    bb->add_instruction(std::make_unique<IRInstruction>(
        IROpcode::Add, v2, std::vector<IRValue>{v0, v1}));
    bb->add_instruction(std::make_unique<IRInstruction>(
        IROpcode::Return, IRValue(), std::vector<IRValue>{v2}));

    // Liveness
    minigpu::LivenessAnalysis liveness(fn.get());
    liveness.run();
    assert(liveness.intervals().size() == 3);

    // Allocation with budget 4
    minigpu::RegisterAllocator ra(fn.get(), 4);
    ra.allocate();
    assert(ra.verify());
    assert(ra.num_spills() == 0);
    std::cout << "[PASS] test_liveness_and_coloring (budget 4, 0 spills)\n";
}

void test_spilling_under_register_pressure() {
    auto fn = std::make_unique<IRFunction>("test_spill", Type::getVoid());
    auto bb = fn->create_block("entry");

    // 4 mutually interfering variables
    auto v0 = fn->create_register(Type::getInt(), "v0");
    auto v1 = fn->create_register(Type::getInt(), "v1");
    auto v2 = fn->create_register(Type::getInt(), "v2");
    auto v3 = fn->create_register(Type::getInt(), "v3");

    bb->add_instruction(std::make_unique<IRInstruction>(IROpcode::Copy, v0, std::vector<IRValue>{IRValue::make_int_const(1)}));
    bb->add_instruction(std::make_unique<IRInstruction>(IROpcode::Copy, v1, std::vector<IRValue>{IRValue::make_int_const(2)}));
    bb->add_instruction(std::make_unique<IRInstruction>(IROpcode::Copy, v2, std::vector<IRValue>{IRValue::make_int_const(3)}));
    bb->add_instruction(std::make_unique<IRInstruction>(IROpcode::Copy, v3, std::vector<IRValue>{IRValue::make_int_const(4)}));

    // All used simultaneously in a sink
    auto sum = fn->create_register(Type::getInt(), "sum");
    bb->add_instruction(std::make_unique<IRInstruction>(IROpcode::Add, sum, std::vector<IRValue>{v0, v1}));
    bb->add_instruction(std::make_unique<IRInstruction>(IROpcode::Add, sum, std::vector<IRValue>{sum, v2}));
    bb->add_instruction(std::make_unique<IRInstruction>(IROpcode::Add, sum, std::vector<IRValue>{sum, v3}));
    bb->add_instruction(std::make_unique<IRInstruction>(IROpcode::Return, IRValue(), std::vector<IRValue>{sum}));

    // With register budget K = 2, spilling MUST occur
    minigpu::RegisterAllocator ra_spill(fn.get(), 2);
    ra_spill.allocate();
    assert(ra_spill.num_spills() > 0);
    assert(ra_spill.verify());
    std::cout << "[PASS] test_spilling_under_register_pressure (budget 2, spills detected and verified)\n";
}

int main() {
    std::cout << "--- Running Register Allocator Tests ---\n";
    test_liveness_and_coloring();
    test_spilling_under_register_pressure();
    std::cout << "All Register Allocator tests passed!\n";
    return 0;
}
