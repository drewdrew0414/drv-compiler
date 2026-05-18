#pragma once
#include "ast.h"
#include "codegen.h"
#include <string>
#include <vector>

namespace drv {

struct CompileOptions {
    std::string  input_file;
    std::string  output_exe;        // --exe
    std::string  output_cpp;        // --cpp
    bool         check_only{false}; // --check
    bool         debug{false};      // --debug
    bool         release{false};    // --release
    int          opt_level{1};      // --opt N
    bool         native{false};     // --native
    bool         lto{false};        // --lto
    std::string  trace_file;        // --trace
    std::vector<std::string> defines; // -DFOO
    // ── Static analysis / incremental ──────────────────────────────────────
    bool         no_analyze{false};   // --no-analyze
    bool         incremental{false};  // --incremental
    std::string  cache_dir;           // --cache-dir <path>
    // ── Cross-compilation ────────────────────────────────────────────────
    std::string  target_triple;       // --target x86_64-linux-gnu
    std::string  sysroot;             // --sysroot /path/to/sysroot
    std::string  cross_cxx;           // --cross-cxx (override compiler binary)
    // ── Source mapping ───────────────────────────────────────────────────
    std::string  source_map_file;     // --source-map <file.json>
};

struct CompileResult {
    bool        success{false};
    std::string cpp_source;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

class Compiler {
public:
    explicit Compiler(CompileOptions opts);
    CompileResult compile();

private:
    CompileOptions opts_;

    std::string readSource();
    void        writeCpp(const std::string& src);
    bool        invokeBackend(const std::string& cpp_path);
};

} // namespace drv
