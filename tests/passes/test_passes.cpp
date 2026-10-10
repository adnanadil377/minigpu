#include "minigpu/ir.hpp"
#include "minigpu/optimizer.hpp"
#include <cassert>
#include <iostream>

using namespace minigpu;

void test_constant_folding() {
    auto fn = std::make_unique<IRFunction>("test_fold", Type::getVoid());
    auto bb = fn->create_block("entry");

    auto r0 = fn->create_register(Type::getInt(), "r0");
    bb->add_instruction(std::make_unique<IRInstruction>(
        IROpcode::Add, r0, std::vector<IRValue>{IRValue::make_int_const(10), IRValue::make_int_const(32)}));
    bb->add_instruction(std::make_unique<IRInstruction>(
        IROpcode::Return, IRValue(), std::vector<IRValue>{r0}));

    minigpu::ConstantFoldingPass fold;
    bool changed = fold.run_on_function(*fn);
    assert(changed);
    assert(bb->instructions()[0]->opcode() == IROpcode::Copy);
    assert(bb->instructions()[0]->operands()[0].int_val() == 42);
    std::cout << "[PASS] test_constant_folding (10 + 32 -> 42)\n";
}

void test_algebraic_simplification() {
    auto fn = std::make_unique<IRFunction>("test_alg", Type::getVoid());
    auto bb = fn->create_block("entry");

    auto x = fn->create_register(Type::getInt(), "x");
    auto r1 = fn->create_register(Type::getInt(), "r1");
    // x + 0
    bb->add_instruction(std::make_unique<IRInstruction>(
        IROpcode::Add, r1, std::vector<IRValue>{x, IRValue::make_int_const(0)}));
    bb->add_instruction(std::make_unique<IRInstruction>(
        IROpcode::Return, IRValue(), std::vector<IRValue>{r1}));

    minigpu::AlgebraicSimplificationPass alg;
    bool changed = alg.run_on_function(*fn);
    assert(changed);
    assert(bb->instructions()[0]->opcode() == IROpcode::Copy);
    assert(bb->instructions()[0]->operands()[0].equals(x));
    std::cout << "[PASS] test_algebraic_simplification (x + 0 -> x)\n";
}

void test_dead_code_elimination() {
    auto fn = std::make_unique<IRFunction>("test_dce", Type::getVoid());
    auto bb = fn->create_block("entry");

    auto dead_reg = fn->create_register(Type::getInt(), "dead");
    bb->add_instruction(std::make_unique<IRInstruction>(
        IROpcode::Add, dead_reg, std::vector<IRValue>{IRValue::make_int_const(1), IRValue::make_int_const(2)}));
    bb->add_instruction(std::make_unique<IRInstruction>(
        IROpcode::Return, IRValue(), std::vector<IRValue>{}));

    minigpu::DeadCodeEliminationPass dce;
    bool changed = dce.run_on_function(*fn);
    assert(changed);
    assert(bb->instructions().size() == 1);
    assert(bb->instructions()[0]->opcode() == IROpcode::Return);
    std::cout << "[PASS] test_dead_code_elimination\n";
}

int main() {
    std::cout << "--- Running Optimization Pass Tests ---\n";
    test_constant_folding();
    test_algebraic_simplification();
    test_dead_code_elimination();
    std::cout << "All Optimization Pass tests passed!\n";
    return 0;
}
