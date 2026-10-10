#pragma once

#include "minigpu/ir.hpp"
#include "minigpu/lexer.hpp"
#include <vector>

namespace minigpu {

class IRVerifier {
public:
    IRVerifier() = default;

    bool verify_module(const IRModule& module);
    bool verify_function(const IRFunction& fn);

    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }
    bool has_errors() const { return has_errors_; }

private:
    void add_error(const SourceLocation& loc, const std::string& msg);

    std::vector<Diagnostic> diagnostics_;
    bool has_errors_{false};
};

} // namespace minigpu
