#pragma once
#include "ast.h"
#include "codegen.h"
#include <string>
#include <vector>

namespace drv {

struct CompileOptions {
    std::string  input_file;
    std::string  output_exe;       // --exe
    std::string  output_cpp;       // --cpp
    bool         check_only{false}; // --check
    bool         debug{false};      // --debug
    bool         release{false};    // --release
    int          opt_level{1};      // --opt N
    bool         native{false};     // --native
    bool         lto{false};        // --lto
    std::string  trace_file;        // --trace
    std::vector<std::string> defines; // -DFOO
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
