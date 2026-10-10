#include "minigpu/simulator.hpp"
#include <cstring>
#include <iostream>

namespace minigpu {

bool IRSimulator::launch(IRFunction& fn,
                         Dim3 grid_dim,
                         Dim3 block_dim,
                         const std::vector<void*>& host_ptrs,
                         const std::vector<size_t>& param_sizes,
                         const std::vector<bool>& is_ptr) {
    (void)param_sizes;
    address_map_.clear();
    uint64_t base_gpu_addr = 0x10000000ULL;

    std::vector<uint64_t> gpu_addresses(host_ptrs.size(), 0);

    for (size_t i = 0; i < host_ptrs.size(); ++i) {
        if (is_ptr[i]) {
            uint64_t addr = base_gpu_addr + (i * 0x10000000ULL);
            address_map_[addr] = host_ptrs[i];
            gpu_addresses[i] = addr;
        }
    }

    // Iterate over grid blocks and threads
    for (int bz = 0; bz < grid_dim.z; ++bz) {
        for (int by = 0; by < grid_dim.y; ++by) {
            for (int bx = 0; bx < grid_dim.x; ++bx) {
                for (int tz = 0; tz < block_dim.z; ++tz) {
                    for (int ty = 0; ty < block_dim.y; ++ty) {
                        for (int tx = 0; tx < block_dim.x; ++tx) {
                            ThreadContext ctx;
                            ctx.tid_x = tx;
                            ctx.tid_y = ty;
                            ctx.tid_z = tz;
                            ctx.bid_x = bx;
                            ctx.bid_y = by;
                            ctx.bid_z = bz;
                            ctx.bdim = block_dim;
                            ctx.gdim = grid_dim;

                            // Set parameter values
                            for (size_t p_idx = 0; p_idx < fn.params().size(); ++p_idx) {
                                if (p_idx >= host_ptrs.size()) break;
                                const auto& p = fn.params()[p_idx];
                                if (is_ptr[p_idx]) {
                                    ctx.reg_ptr[p_idx + 10000] = gpu_addresses[p_idx];
                                } else if (p.type()->is_float()) {
                                    float f_val = *reinterpret_cast<float*>(host_ptrs[p_idx]);
                                    ctx.reg_float[p_idx + 10000] = f_val;
                                } else {
                                    int i_val = *reinterpret_cast<int*>(host_ptrs[p_idx]);
                                    ctx.reg_int[p_idx + 10000] = i_val;
                                }
                            }

                            if (!execute_thread(fn, ctx)) {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool IRSimulator::execute_thread(IRFunction& fn, ThreadContext& ctx) {
    IRBasicBlock* curr_bb = fn.entry_block();
    if (!curr_bb) return false;

    // Local stack pointer
    uint64_t local_sp = 0x8000000000000000ULL;
    std::unordered_map<int, uint64_t> alloca_map;

    auto resolve_ptr = [&](uint64_t addr) -> void* {
        if ((addr & 0x8000000000000000ULL) != 0) {
            size_t offset = static_cast<size_t>(addr & 0x7FFFFFFFFFFFFFFFULL);
            if (offset + 8 > ctx.local_memory.size()) {
                ctx.local_memory.resize(offset + 64, 0);
            }
            return &ctx.local_memory[offset];
        }
        // Global pointer: find matching base
        for (const auto& pair : address_map_) {
            uint64_t base = pair.first;
            if (addr >= base && addr < base + 0x10000000ULL) {
                uint64_t offset = addr - base;
                return reinterpret_cast<char*>(pair.second) + offset;
            }
        }
        return nullptr;
    };

    auto get_int_val = [&](const IRValue& v) -> int64_t {
        if (v.kind() == IRValueKind::ConstantInt) return v.int_val();
        if (v.is_param()) {
            for (size_t i = 0; i < fn.params().size(); ++i) {
                if (fn.params()[i].name() == v.name()) {
                    return ctx.reg_int[i + 10000];
                }
            }
        }
        if (v.is_reg()) return ctx.reg_int[v.reg_id()];
        return 0;
    };

    auto get_float_val = [&](const IRValue& v) -> double {
        if (v.kind() == IRValueKind::ConstantFloat) return v.float_val();
        if (v.is_param()) {
            for (size_t i = 0; i < fn.params().size(); ++i) {
                if (fn.params()[i].name() == v.name()) return ctx.reg_float[i + 10000];
            }
        }
        if (v.is_reg()) return ctx.reg_float[v.reg_id()];
        return 0.0;
    };

    auto get_bool_val = [&](const IRValue& v) -> bool {
        if (v.kind() == IRValueKind::ConstantBool) return v.bool_val();
        if (v.is_reg()) return ctx.reg_bool[v.reg_id()];
        return false;
    };

    auto get_ptr_val = [&](const IRValue& v) -> uint64_t {
        if (v.is_param()) {
            for (size_t i = 0; i < fn.params().size(); ++i) {
                if (fn.params()[i].name() == v.name()) return ctx.reg_ptr[i + 10000];
            }
        }
        if (v.is_reg()) {
            if (alloca_map.count(v.reg_id())) return alloca_map[v.reg_id()];
            return ctx.reg_ptr[v.reg_id()];
        }
        return 0;
    };

    int cycle_limit = 1000000;
    int cycles = 0;

    while (curr_bb && cycles++ < cycle_limit) {
        IRBasicBlock* next_bb = nullptr;

        for (const auto& inst : curr_bb->instructions()) {
            switch (inst->opcode()) {
                case IROpcode::Alloca: {
                    uint64_t addr = local_sp | ctx.local_memory.size();
                    ctx.local_memory.resize(ctx.local_memory.size() + 16, 0);
                    alloca_map[inst->dest().reg_id()] = addr;
                    ctx.reg_ptr[inst->dest().reg_id()] = addr;
                    break;
                }
                case IROpcode::SpecialReg: {
                    const std::string& sr = inst->special_reg();
                    int64_t val = 0;
                    if (sr == "thread_id_x") val = ctx.tid_x;
                    else if (sr == "thread_id_y") val = ctx.tid_y;
                    else if (sr == "thread_id_z") val = ctx.tid_z;
                    else if (sr == "block_id_x") val = ctx.bid_x;
                    else if (sr == "block_id_y") val = ctx.bid_y;
                    else if (sr == "block_id_z") val = ctx.bid_z;
                    else if (sr == "block_dim_x") val = ctx.bdim.x;
                    else if (sr == "block_dim_y") val = ctx.bdim.y;
                    else if (sr == "grid_dim_x") val = ctx.gdim.x;
                    ctx.reg_int[inst->dest().reg_id()] = val;
                    break;
                }
                case IROpcode::Copy: {
                    const auto& op = inst->operands()[0];
                    if (inst->dest().type()->is_float()) {
                        ctx.reg_float[inst->dest().reg_id()] = get_float_val(op);
                    } else if (inst->dest().type()->is_bool()) {
                        ctx.reg_bool[inst->dest().reg_id()] = get_bool_val(op);
                    } else if (inst->dest().type()->is_pointer()) {
                        ctx.reg_ptr[inst->dest().reg_id()] = get_ptr_val(op);
                    } else {
                        ctx.reg_int[inst->dest().reg_id()] = get_int_val(op);
                    }
                    break;
                }
                case IROpcode::Add: {
                    ctx.reg_int[inst->dest().reg_id()] =
                        get_int_val(inst->operands()[0]) + get_int_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::Sub: {
                    ctx.reg_int[inst->dest().reg_id()] =
                        get_int_val(inst->operands()[0]) - get_int_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::Mul: {
                    ctx.reg_int[inst->dest().reg_id()] =
                        get_int_val(inst->operands()[0]) * get_int_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::SDiv: {
                    int64_t d = get_int_val(inst->operands()[1]);
                    ctx.reg_int[inst->dest().reg_id()] = (d != 0) ? (get_int_val(inst->operands()[0]) / d) : 0;
                    break;
                }
                case IROpcode::SRem: {
                    int64_t d = get_int_val(inst->operands()[1]);
                    ctx.reg_int[inst->dest().reg_id()] = (d != 0) ? (get_int_val(inst->operands()[0]) % d) : 0;
                    break;
                }
                case IROpcode::FAdd: {
                    ctx.reg_float[inst->dest().reg_id()] =
                        get_float_val(inst->operands()[0]) + get_float_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::FSub: {
                    ctx.reg_float[inst->dest().reg_id()] =
                        get_float_val(inst->operands()[0]) - get_float_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::FMul: {
                    ctx.reg_float[inst->dest().reg_id()] =
                        get_float_val(inst->operands()[0]) * get_float_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::FDiv: {
                    double d = get_float_val(inst->operands()[1]);
                    ctx.reg_float[inst->dest().reg_id()] = (d != 0.0) ? (get_float_val(inst->operands()[0]) / d) : 0.0;
                    break;
                }
                case IROpcode::ICmpSLT: {
                    ctx.reg_bool[inst->dest().reg_id()] =
                        get_int_val(inst->operands()[0]) < get_int_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::ICmpSLE: {
                    ctx.reg_bool[inst->dest().reg_id()] =
                        get_int_val(inst->operands()[0]) <= get_int_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::ICmpSGT: {
                    ctx.reg_bool[inst->dest().reg_id()] =
                        get_int_val(inst->operands()[0]) > get_int_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::ICmpSGE: {
                    ctx.reg_bool[inst->dest().reg_id()] =
                        get_int_val(inst->operands()[0]) >= get_int_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::ICmpEQ: {
                    ctx.reg_bool[inst->dest().reg_id()] =
                        get_int_val(inst->operands()[0]) == get_int_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::ICmpNE: {
                    ctx.reg_bool[inst->dest().reg_id()] =
                        get_int_val(inst->operands()[0]) != get_int_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::FCmpOLT: {
                    ctx.reg_bool[inst->dest().reg_id()] =
                        get_float_val(inst->operands()[0]) < get_float_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::FCmpOLE: {
                    ctx.reg_bool[inst->dest().reg_id()] =
                        get_float_val(inst->operands()[0]) <= get_float_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::FCmpOGT: {
                    ctx.reg_bool[inst->dest().reg_id()] =
                        get_float_val(inst->operands()[0]) > get_float_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::FCmpOGE: {
                    ctx.reg_bool[inst->dest().reg_id()] =
                        get_float_val(inst->operands()[0]) >= get_float_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::FCmpEQ: {
                    ctx.reg_bool[inst->dest().reg_id()] =
                        get_float_val(inst->operands()[0]) == get_float_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::FCmpNE: {
                    ctx.reg_bool[inst->dest().reg_id()] =
                        get_float_val(inst->operands()[0]) != get_float_val(inst->operands()[1]);
                    break;
                }
                case IROpcode::And: {
                    if (inst->dest().type()->is_bool()) {
                        ctx.reg_bool[inst->dest().reg_id()] =
                            get_bool_val(inst->operands()[0]) && get_bool_val(inst->operands()[1]);
                    } else {
                        ctx.reg_int[inst->dest().reg_id()] =
                            get_int_val(inst->operands()[0]) & get_int_val(inst->operands()[1]);
                    }
                    break;
                }
                case IROpcode::Or: {
                    if (inst->dest().type()->is_bool()) {
                        ctx.reg_bool[inst->dest().reg_id()] =
                            get_bool_val(inst->operands()[0]) || get_bool_val(inst->operands()[1]);
                    } else {
                        ctx.reg_int[inst->dest().reg_id()] =
                            get_int_val(inst->operands()[0]) | get_int_val(inst->operands()[1]);
                    }
                    break;
                }
                case IROpcode::Xor: {
                    if (inst->dest().type()->is_bool()) {
                        ctx.reg_bool[inst->dest().reg_id()] =
                            get_bool_val(inst->operands()[0]) ^ get_bool_val(inst->operands()[1]);
                    } else {
                        ctx.reg_int[inst->dest().reg_id()] =
                            get_int_val(inst->operands()[0]) ^ get_int_val(inst->operands()[1]);
                    }
                    break;
                }
                case IROpcode::SIToFP: {
                    ctx.reg_float[inst->dest().reg_id()] = static_cast<double>(get_int_val(inst->operands()[0]));
                    break;
                }
                case IROpcode::FPToSI: {
                    ctx.reg_int[inst->dest().reg_id()] = static_cast<int64_t>(get_float_val(inst->operands()[0]));
                    break;
                }
                case IROpcode::GetElementPtr: {
                    uint64_t base = get_ptr_val(inst->operands()[0]);
                    int64_t idx = get_int_val(inst->operands()[1]);
                    uint64_t elem_size = 4;
                    ctx.reg_ptr[inst->dest().reg_id()] = base + (idx * elem_size);
                    break;
                }
                case IROpcode::Load: {
                    uint64_t addr = get_ptr_val(inst->operands()[0]);
                    void* p = resolve_ptr(addr);
                    if (p) {
                        if (inst->dest().type()->is_float()) {
                            float val = *reinterpret_cast<float*>(p);
                            ctx.reg_float[inst->dest().reg_id()] = val;
                        } else if (inst->dest().type()->is_pointer()) {
                            uint64_t val = *reinterpret_cast<uint64_t*>(p);
                            ctx.reg_ptr[inst->dest().reg_id()] = val;
                        } else {
                            int32_t val = *reinterpret_cast<int32_t*>(p);
                            ctx.reg_int[inst->dest().reg_id()] = val;
                        }
                    }
                    break;
                }
                case IROpcode::Store: {
                    uint64_t addr = get_ptr_val(inst->operands()[1]);
                    void* p = resolve_ptr(addr);
                    if (p) {
                        const auto& val = inst->operands()[0];
                        if (val.type()->is_float()) {
                            float f = static_cast<float>(get_float_val(val));
                            *reinterpret_cast<float*>(p) = f;
                        } else if (val.type()->is_pointer()) {
                            uint64_t ptr_val = get_ptr_val(val);
                            *reinterpret_cast<uint64_t*>(p) = ptr_val;
                        } else {
                            int32_t i = static_cast<int32_t>(get_int_val(val));
                            *reinterpret_cast<int32_t*>(p) = i;
                        }
                    }
                    break;
                }
                case IROpcode::Branch: {
                    next_bb = fn.find_block(inst->true_label());
                    break;
                }
                case IROpcode::BranchCond: {
                    bool cond = get_bool_val(inst->operands()[0]);
                    next_bb = fn.find_block(cond ? inst->true_label() : inst->false_label());
                    break;
                }
                case IROpcode::Return: {
                    return true;
                }
                default: break;
            }
        }

        curr_bb = next_bb;
    }

    return true;
}

} // namespace minigpu
