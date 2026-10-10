#pragma once

#include "minigpu/types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <iostream>

namespace minigpu {

enum class IROpcode {
    // Arithmetic Integer
    Add,
    Sub,
    Mul,
    SDiv,
    SRem,

    // Arithmetic Float
    FAdd,
    FSub,
    FMul,
    FDiv,
    FMA,

    // Comparisons Integer
    ICmpEQ,
    ICmpNE,
    ICmpSLT,
    ICmpSLE,
    ICmpSGT,
    ICmpSGE,

    // Comparisons Float
    FCmpEQ,
    FCmpNE,
    FCmpOLT,
    FCmpOLE,
    FCmpOGT,
    FCmpOGE,

    // Logical / Bitwise
    And,
    Or,
    Xor,

    // Conversion
    SIToFP,
    FPToSI,

    // Memory operations
    Alloca,
    Load,
    Store,
    GetElementPtr,

    // GPU Built-ins
    SpecialReg,

    // Data movement
    Copy,
    ParamLoad,

    // Control Flow (Terminators)
    Branch,
    BranchCond,
    Return
};

const char* ir_opcode_to_string(IROpcode op);

enum class IRValueKind {
    None,
    Register,
    ConstantInt,
    ConstantFloat,
    ConstantBool,
    Parameter
};

class IRValue {
public:
    IRValue() : kind_(IRValueKind::None), type_(Type::getVoid()) {}

    static IRValue make_reg(int id, std::shared_ptr<Type> type, std::string debug_name = "") {
        IRValue v;
        v.kind_ = IRValueKind::Register;
        v.id_ = id;
        v.type_ = std::move(type);
        v.str_val_ = std::move(debug_name);
        return v;
    }

    static IRValue make_int_const(int64_t val) {
        IRValue v;
        v.kind_ = IRValueKind::ConstantInt;
        v.int_val_ = val;
        v.type_ = Type::getInt();
        return v;
    }

    static IRValue make_float_const(double val) {
        IRValue v;
        v.kind_ = IRValueKind::ConstantFloat;
        v.float_val_ = val;
        v.type_ = Type::getFloat();
        return v;
    }

    static IRValue make_bool_const(bool val) {
        IRValue v;
        v.kind_ = IRValueKind::ConstantBool;
        v.bool_val_ = val;
        v.type_ = Type::getBool();
        return v;
    }

    static IRValue make_param(std::string name, std::shared_ptr<Type> type) {
        IRValue v;
        v.kind_ = IRValueKind::Parameter;
        v.str_val_ = std::move(name);
        v.type_ = std::move(type);
        return v;
    }

    IRValueKind kind() const { return kind_; }
    const std::shared_ptr<Type>& type() const { return type_; }
    int reg_id() const { return id_; }
    int64_t int_val() const { return int_val_; }
    double float_val() const { return float_val_; }
    bool bool_val() const { return bool_val_; }
    const std::string& name() const { return str_val_; }

    bool is_reg() const { return kind_ == IRValueKind::Register; }
    bool is_const() const {
        return kind_ == IRValueKind::ConstantInt ||
               kind_ == IRValueKind::ConstantFloat ||
               kind_ == IRValueKind::ConstantBool;
    }
    bool is_param() const { return kind_ == IRValueKind::Parameter; }

    std::string to_string() const;
    bool equals(const IRValue& other) const;

private:
    IRValueKind kind_{IRValueKind::None};
    std::shared_ptr<Type> type_;
    int id_{-1};
    int64_t int_val_{0};
    double float_val_{0.0};
    bool bool_val_{false};
    std::string str_val_;
};

inline bool operator==(const IRValue& lhs, const IRValue& rhs) {
    return lhs.equals(rhs);
}

class IRBasicBlock;

class IRInstruction {
public:
    IRInstruction(IROpcode op, IRValue dest, std::vector<IRValue> ops, SourceLocation loc = {})
        : opcode_(op), dest_(std::move(dest)), operands_(std::move(ops)), loc_(std::move(loc)) {}

    IROpcode opcode() const { return opcode_; }
    void set_opcode(IROpcode op) { opcode_ = op; }

    const IRValue& dest() const { return dest_; }
    void set_dest(IRValue dest) { dest_ = std::move(dest); }
    bool has_dest() const { return dest_.kind() != IRValueKind::None; }

    const std::vector<IRValue>& operands() const { return operands_; }
    std::vector<IRValue>& operands() { return operands_; }
    void set_operand(size_t idx, IRValue val) { operands_[idx] = std::move(val); }

    const std::string& true_label() const { return true_label_; }
    void set_true_label(std::string l) { true_label_ = std::move(l); }

    const std::string& false_label() const { return false_label_; }
    void set_false_label(std::string l) { false_label_ = std::move(l); }

    const std::string& special_reg() const { return special_reg_; }
    void set_special_reg(std::string r) { special_reg_ = std::move(r); }

    const SourceLocation& loc() const { return loc_; }

    bool is_terminator() const {
        return opcode_ == IROpcode::Branch ||
               opcode_ == IROpcode::BranchCond ||
               opcode_ == IROpcode::Return;
    }

    std::string to_string() const;

private:
    IROpcode opcode_;
    IRValue dest_;
    std::vector<IRValue> operands_;
    std::string true_label_;
    std::string false_label_;
    std::string special_reg_;
    SourceLocation loc_;
};

class IRBasicBlock {
public:
    explicit IRBasicBlock(std::string label) : label_(std::move(label)) {}

    const std::string& label() const { return label_; }
    void set_label(std::string l) { label_ = std::move(l); }

    const std::vector<std::unique_ptr<IRInstruction>>& instructions() const { return instructions_; }
    std::vector<std::unique_ptr<IRInstruction>>& instructions() { return instructions_; }

    void add_instruction(std::unique_ptr<IRInstruction> inst) {
        instructions_.push_back(std::move(inst));
    }

    bool is_terminated() const {
        if (instructions_.empty()) return false;
        return instructions_.back()->is_terminator();
    }

    IRInstruction* get_terminator() {
        if (is_terminated()) return instructions_.back().get();
        return nullptr;
    }

    const std::vector<IRBasicBlock*>& predecessors() const { return preds_; }
    std::vector<IRBasicBlock*>& predecessors() { return preds_; }
    const std::vector<IRBasicBlock*>& successors() const { return succs_; }
    std::vector<IRBasicBlock*>& successors() { return succs_; }

    void add_predecessor(IRBasicBlock* b) { preds_.push_back(b); }
    void add_successor(IRBasicBlock* b) { succs_.push_back(b); }
    void clear_cfg_edges() { preds_.clear(); succs_.clear(); }

private:
    std::string label_;
    std::vector<std::unique_ptr<IRInstruction>> instructions_;
    std::vector<IRBasicBlock*> preds_;
    std::vector<IRBasicBlock*> succs_;
};

class IRFunction {
public:
    IRFunction(std::string name, std::shared_ptr<Type> ret_type)
        : name_(std::move(name)), ret_type_(std::move(ret_type)) {}

    const std::string& name() const { return name_; }
    const std::shared_ptr<Type>& return_type() const { return ret_type_; }

    const std::vector<IRValue>& params() const { return params_; }
    std::vector<IRValue>& params() { return params_; }
    void add_param(IRValue p) { params_.push_back(std::move(p)); }

    const std::vector<std::unique_ptr<IRBasicBlock>>& blocks() const { return blocks_; }
    std::vector<std::unique_ptr<IRBasicBlock>>& blocks() { return blocks_; }

    IRBasicBlock* entry_block() {
        return blocks_.empty() ? nullptr : blocks_.front().get();
    }

    IRBasicBlock* create_block(const std::string& prefix = "bb") {
        std::string label = prefix + "_" + std::to_string(block_counter_++);
        blocks_.push_back(std::make_unique<IRBasicBlock>(label));
        return blocks_.back().get();
    }

    IRBasicBlock* create_block_with_label(std::string label) {
        blocks_.push_back(std::make_unique<IRBasicBlock>(std::move(label)));
        return blocks_.back().get();
    }

    IRValue create_register(std::shared_ptr<Type> type, const std::string& debug_hint = "") {
        return IRValue::make_reg(reg_counter_++, std::move(type), debug_hint);
    }

    int next_reg_id() const { return reg_counter_; }

    IRBasicBlock* find_block(const std::string& label) {
        for (auto& b : blocks_) {
            if (b->label() == label) return b.get();
        }
        return nullptr;
    }

private:
    std::string name_;
    std::shared_ptr<Type> ret_type_;
    std::vector<IRValue> params_;
    std::vector<std::unique_ptr<IRBasicBlock>> blocks_;
    int reg_counter_{0};
    int block_counter_{0};
};

class IRModule {
public:
    explicit IRModule(std::string name = "module") : name_(std::move(name)) {}

    const std::string& name() const { return name_; }
    const std::vector<std::unique_ptr<IRFunction>>& functions() const { return functions_; }
    std::vector<std::unique_ptr<IRFunction>>& functions() { return functions_; }

    void add_function(std::unique_ptr<IRFunction> fn) {
        functions_.push_back(std::move(fn));
    }

private:
    std::string name_;
    std::vector<std::unique_ptr<IRFunction>> functions_;
};

/// IR Pretty Printer
class IRPrinter {
public:
    static void print(const IRModule& module, std::ostream& out);
    static void print(const IRFunction& fn, std::ostream& out);
    static void print(const IRBasicBlock& bb, std::ostream& out);
    static void print(const IRInstruction& inst, std::ostream& out);
};

} // namespace minigpu
