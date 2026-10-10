#include "minigpu/lexer.hpp"
#include "minigpu/parser.hpp"
#include "minigpu/semantic_analysis.hpp"
#include "minigpu/ir_builder.hpp"
#include "minigpu/ir_verifier.hpp"
#include "minigpu/cfg.hpp"
#include "minigpu/optimizer.hpp"
#include "minigpu/liveness.hpp"
#include "minigpu/regalloc.hpp"
#include "minigpu/codegen.hpp"
#include "minigpu/simulator.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

void print_help(const char* prog) {
    std::cout << "MiniGPU - Educational GPU Compiler\n"
              << "Usage: " << prog << " [options] <input.mgpu>\n\n"
              << "Pipeline Stages & Emissions:\n"
              << "  --emit-tokens         Tokenize source and print token stream\n"
              << "  --emit-ast            Parse source and print typed AST\n"
              << "  --emit-ir             Lower to three-address IR and print\n"
              << "  --emit-cfg            Print control-flow graph and basic blocks\n"
              << "  --emit-liveness       Compute and print dataflow liveness intervals\n"
              << "  --emit-regalloc       Run graph-coloring register allocation and print\n"
              << "\nOptimization Options:\n"
              << "  --no-opt              Disable all optimization passes\n"
              << "  --dump-passes         Print IR before and after each optimization pass\n"
              << "\nBackend & Code Generation:\n"
              << "  --target <arch>       Set PTX target architecture (default: sm_75)\n"
              << "                        Supported: sm_70, sm_75, sm_80, sm_86, sm_90\n"
              << "  --register-budget <N> Set physical register budget for coloring (default: 32)\n"
              << "  -o, --output <file>   Write generated PTX output to file\n"
              << "\nVerification & Execution:\n"
              << "  --verify-ir           Run strict IR verifier checks\n"
              << "  --run-cpu             Simulate execution on CPU grid with sample buffers\n"
              << "\nGeneral:\n"
              << "  -h, --help            Show this help message\n"
              << "  -v, --version         Show compiler version\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    std::string input_file;
    std::string output_file;
    bool emit_tokens = false;
    bool emit_ast = false;
    bool emit_ir = false;
    bool emit_cfg = false;
    bool emit_liveness = false;
    bool emit_regalloc = false;
    bool no_opt = false;
    bool dump_passes = false;
    bool verify_ir = false;
    bool run_cpu = false;
    int register_budget = 32;
    minigpu::TargetArch target_arch = minigpu::TargetArch::SM_75;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "MiniGPU Compiler v0.4.0 (Educational GPU Compiler)\n";
            return 0;
        } else if (arg == "--emit-tokens") {
            emit_tokens = true;
        } else if (arg == "--emit-ast") {
            emit_ast = true;
        } else if (arg == "--emit-ir") {
            emit_ir = true;
        } else if (arg == "--emit-cfg") {
            emit_cfg = true;
        } else if (arg == "--emit-liveness") {
            emit_liveness = true;
        } else if (arg == "--emit-regalloc") {
            emit_regalloc = true;
        } else if (arg == "--no-opt") {
            no_opt = true;
        } else if (arg == "--dump-passes") {
            dump_passes = true;
        } else if (arg == "--verify-ir") {
            verify_ir = true;
        } else if (arg == "--run-cpu") {
            run_cpu = true;
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "--target") {
            if (i + 1 < argc) {
                target_arch = minigpu::parse_target_arch(argv[++i]);
            } else {
                std::cerr << "Error: --target requires architecture argument\n";
                return 1;
            }
        } else if (arg == "--register-budget") {
            if (i + 1 < argc) {
                register_budget = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --register-budget requires an integer\n";
                return 1;
            }
        } else if (arg[0] == '-') {
            std::cerr << "Error: unrecognized option '" << arg << "'\n";
            print_help(argv[0]);
            return 1;
        } else {
            input_file = arg;
        }
    }

    if (input_file.empty()) {
        std::cerr << "Error: no input file provided\n";
        return 1;
    }

    // Read source
    std::ifstream file(input_file);
    if (!file.is_open()) {
        std::cerr << "Error: could not open file '" << input_file << "'\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // 1. Lexing
    minigpu::Lexer lexer(source, input_file);
    auto tokens = lexer.tokenize();

    if (lexer.has_errors()) {
        for (const auto& diag : lexer.diagnostics()) {
            std::cerr << diag.to_string() << "\n";
        }
        return 1;
    }

    if (emit_tokens) {
        for (const auto& tok : tokens) {
            std::cout << tok.to_string() << "\n";
        }
        return 0;
    }

    // 2. Parsing
    minigpu::Parser parser(tokens);
    auto ast = parser.parse_program();

    if (parser.has_errors()) {
        for (const auto& diag : parser.diagnostics()) {
            std::cerr << diag.to_string() << "\n";
        }
        return 1;
    }

    // 3. Semantic Analysis
    minigpu::SemanticAnalyzer sema;
    if (!sema.analyze(*ast)) {
        for (const auto& diag : sema.diagnostics()) {
            std::cerr << diag.to_string() << "\n";
        }
        return 1;
    }

    if (emit_ast) {
        minigpu::ASTPrinter printer(std::cout);
        printer.print(*ast);
        return 0;
    }

    // 4. IR Generation
    minigpu::IRGen irgen;
    auto ir_module = irgen.emit_module(*ast);

    // Strict IR verification if requested
    if (verify_ir) {
        minigpu::IRVerifier verifier;
        if (!verifier.verify_module(*ir_module)) {
            std::cerr << "IR Verification Failed:\n";
            for (const auto& diag : verifier.diagnostics()) {
                std::cerr << "  " << diag.to_string() << "\n";
            }
            return 1;
        }
    }

    // 5. Optimization Passes
    if (!no_opt) {
        minigpu::PassManager pm;
        pm.add_default_passes();
        pm.set_dump_passes(dump_passes);
        pm.run(*ir_module, std::cout);
    }

    if (emit_ir) {
        minigpu::IRPrinter::print(*ir_module, std::cout);
        return 0;
    }

    if (emit_cfg) {
        for (auto& fn : ir_module->functions()) {
            minigpu::CFG cfg(fn.get());
            cfg.dump_text(std::cout);
            std::cout << "\n";
        }
        return 0;
    }

    if (emit_liveness) {
        for (auto& fn : ir_module->functions()) {
            minigpu::LivenessAnalysis liveness(fn.get());
            liveness.run();
            liveness.dump(std::cout);
            std::cout << "\n";
        }
        return 0;
    }

    if (emit_regalloc) {
        for (auto& fn : ir_module->functions()) {
            minigpu::RegisterAllocator ra(fn.get(), register_budget);
            ra.allocate();
            ra.dump(std::cout);
            std::cout << "\n";
        }
        return 0;
    }

    // 6. PTX Code Generation
    minigpu::PTXCodeGen codegen(target_arch);
    codegen.set_register_budget(register_budget);
    std::string ptx_code = codegen.generate(*ir_module);

    if (run_cpu) {
        std::cout << "Simulating execution on CPU grid for kernel in '" << input_file << "'...\n";
        if (ir_module->functions().empty()) {
            std::cerr << "Error: no function found in module\n";
            return 1;
        }
        auto& fn = ir_module->functions()[0];

        // Prepare dummy buffers for execution
        const int N = 16;
        std::vector<float> A(N, 1.0f);
        std::vector<float> B(N, 2.0f);
        std::vector<float> C(N, 0.0f);
        int n_param = N;

        std::vector<void*> host_ptrs = {A.data(), B.data(), C.data(), &n_param};
        std::vector<size_t> sizes = {N * sizeof(float), N * sizeof(float), N * sizeof(float), sizeof(int)};
        std::vector<bool> is_ptr = {true, true, true, false};

        minigpu::IRSimulator sim;
        bool ok = sim.launch(*fn, minigpu::Dim3(1, 1, 1), minigpu::Dim3(N, 1, 1), host_ptrs, sizes, is_ptr);
        if (ok) {
            std::cout << "Kernel execution completed successfully!\nSample outputs: ";
            for (int i = 0; i < std::min(5, N); ++i) {
                std::cout << "C[" << i << "]=" << C[i] << " ";
            }
            std::cout << "\n";
        } else {
            std::cerr << "Kernel simulation failed!\n";
            return 1;
        }
    }

    if (!output_file.empty()) {
        std::ofstream out(output_file);
        if (!out.is_open()) {
            std::cerr << "Error: could not open output file '" << output_file << "'\n";
            return 1;
        }
        out << ptx_code;
        std::cout << "Successfully generated PTX: " << output_file << "\n";
    } else if (!run_cpu) {
        std::cout << ptx_code;
    }

    return 0;
}
