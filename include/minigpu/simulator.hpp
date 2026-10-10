#pragma once

#include "minigpu/ir.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>

namespace minigpu {

struct Dim3 {
    int x{1};
    int y{1};
    int z{1};

    Dim3(int x_ = 1, int y_ = 1, int z_ = 1) : x(x_), y(y_), z(z_) {}
};

struct SimulatedBuffer {
    std::vector<uint8_t> data;
    uint64_t base_address{0};

    SimulatedBuffer() = default;
    explicit SimulatedBuffer(size_t size_bytes, uint64_t addr = 0x10000000)
        : data(size_bytes, 0), base_address(addr) {}
};

class IRSimulator {
public:
    IRSimulator() = default;

    /// Launch a kernel function on a simulated GPU grid
    bool launch(IRFunction& fn,
                Dim3 grid_dim,
                Dim3 block_dim,
                const std::vector<void*>& host_ptrs,
                const std::vector<size_t>& param_sizes,
                const std::vector<bool>& is_ptr);

private:
    struct ThreadContext {
        int tid_x{0};
        int tid_y{0};
        int tid_z{0};
        int bid_x{0};
        int bid_y{0};
        int bid_z{0};
        Dim3 bdim;
        Dim3 gdim;

        // Virtual registers value store
        std::unordered_map<int, int64_t> reg_int;
        std::unordered_map<int, double> reg_float;
        std::unordered_map<int, bool> reg_bool;
        std::unordered_map<int, uint64_t> reg_ptr;

        // Local alloca memory
        std::vector<uint8_t> local_memory;
    };

    bool execute_thread(IRFunction& fn, ThreadContext& ctx);

    // Global memory simulator
    std::unordered_map<uint64_t, void*> address_map_;
};

} // namespace minigpu
