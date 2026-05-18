#include "compiler/src/compiler.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

static void printUsage(const char* prog) {
    std::cerr
        << "dri compiler — transpiles .dri source to C++17\n\n"
        << "Usage:\n"
        << "  " << prog << " <input.dri> [options]\n\n"
        << "Options:\n"
        << "  --exe <file>     Build executable\n"
        << "  --cpp <file>     Output C++ source only\n"
        << "  --check          Type check only, no code generation\n"
        << "  --debug          Debug build (-O0, bounds checks)\n"
        << "  --release        Release build (-O3, -march=native, LTO)\n"
        << "  --opt <0-3>      Manual optimization level\n"
        << "  --native         Enable -march=native\n"
        << "  --lto            Enable Link-Time Optimization\n"
        << "  --trace <file>   Export parallel loop trace (Chrome DevTools)\n"
        << "  -D<FLAG>         Define compile-time flag for static_if\n"
        << "  --version        Show compiler version\n"
        << "  --help           Show this help\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    if (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0) {
        printUsage(argv[0]); return 0;
    }
    if (std::strcmp(argv[1], "--version") == 0) {
        std::cout << "dri compiler v0.1.0 (spec: 2026-05, C++20 backend)\n";
        return 0;
    }

    drv::CompileOptions opts;
    opts.input_file = argv[1];

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--exe" && i+1 < argc) {
            opts.output_exe = argv[++i];
        } else if (arg == "--cpp" && i+1 < argc) {
            opts.output_cpp = argv[++i];
        } else if (arg == "--check") {
            opts.check_only = true;
        } else if (arg == "--debug") {
            opts.debug = true;
        } else if (arg == "--release") {
            opts.release = true;
            opts.opt_level = 3;
            opts.native = true;
            opts.lto = true;
        } else if (arg == "--opt" && i+1 < argc) {
            opts.opt_level = std::stoi(argv[++i]);
        } else if (arg == "--native") {
            opts.native = true;
        } else if (arg == "--lto") {
            opts.lto = true;
        } else if (arg == "--trace" && i+1 < argc) {
            opts.trace_file = argv[++i];
        } else if (arg.size() > 2 && arg[0]=='-' && arg[1]=='D') {
            opts.defines.push_back(arg.substr(2));
        } else {
            std::cerr << "dri: unknown option '" << arg << "'\n";
            return 1;
        }
    }

    if (opts.input_file.empty()) {
        std::cerr << "dri: no input file\n";
        return 1;
    }

    drv::Compiler compiler(opts);
    auto result = compiler.compile();

    // Print errors
    for (auto& e : result.errors)   std::cerr << e << '\n';
    for (auto& w : result.warnings) std::cerr << "warning: " << w << '\n';

    if (!result.success) return 1;

    if (opts.check_only)
        std::cerr << "dri: " << opts.input_file << " — OK\n";

    return 0;
}
