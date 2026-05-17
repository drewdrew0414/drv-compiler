#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <filesystem>

namespace drv {

namespace fs = std::filesystem;

Compiler::Compiler(CompileOptions opts) : opts_(std::move(opts)) {}

std::string Compiler::readSource() {
    std::ifstream f(opts_.input_file);
    if (!f) throw std::runtime_error("Cannot open file: " + opts_.input_file);
    return {std::istreambuf_iterator<char>(f), {}};
}

void Compiler::writeCpp(const std::string& src) {
    std::string path = opts_.output_cpp.empty()
        ? (fs::temp_directory_path() / "__drv_out__.cpp").string()
        : opts_.output_cpp;
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f << src;
}

bool Compiler::invokeBackend(const std::string& cpp_path) {
    if (opts_.output_exe.empty()) return true;

    // Detect compiler — prefer clang++, fall back to g++
#ifdef _WIN32
    std::string null_dev = "NUL";
#else
    std::string null_dev = "/dev/null";
#endif
    std::string cxx = "clang++";
    if (std::system(("clang++ --version >" + null_dev + " 2>&1").c_str()) != 0) {
        if (std::system(("g++ --version >" + null_dev + " 2>&1").c_str()) == 0)
            cxx = "g++";
        else {
            return false; // no compiler found
        }
    }

    // Flags
    std::string flags = "-std=c++20 ";
    if (opts_.debug)        flags += "-O0 -g -DDEBUG ";
    else if (opts_.release) flags += "-O3 -DNDEBUG ";
    else                    flags += "-O" + std::to_string(opts_.opt_level) + " ";

    if (opts_.native) flags += "-march=native ";
    if (opts_.lto)    flags += "-flto ";

    for (auto& d : opts_.defines) flags += "-D" + d + " ";

    // OpenMP — optional, skip silently if unavailable
#ifndef _WIN32
    flags += "-fopenmp ";
#endif

    // Quote paths for spaces
    auto q = [](const std::string& s) { return "\"" + s + "\""; };
    std::string cmd = cxx + " " + flags + " " + q(cpp_path) +
                      " -o " + q(opts_.output_exe) +
#ifndef _WIN32
                      " -lm"
#endif
                      " 2>&1";
    int ret = std::system(cmd.c_str());
    return ret == 0;
}

CompileResult Compiler::compile() {
    CompileResult result;

    try {
        // 1. Read source
        std::string src = readSource();

        // 2. Lex
        Lexer lexer(src, opts_.input_file);
        auto tokens = lexer.tokenize();
        if (lexer.hasErrors()) {
            result.errors = lexer.errors();
            return result;
        }

        // 3. Parse
        Parser parser(std::move(tokens), opts_.input_file);
        auto program = parser.parse();
        if (parser.hasErrors()) {
            result.errors = parser.errors();
            return result;
        }

        if (opts_.check_only) {
            result.success = true;
            return result;
        }

        // 4. Code generation
        CodegenOptions cg_opts;
        cg_opts.source_file = opts_.input_file;
        cg_opts.debug_build = opts_.debug;
        cg_opts.release_build = opts_.release;
        cg_opts.opt_level = opts_.opt_level;
        cg_opts.emit_line_directives = true;

        Codegen cg(cg_opts);
        result.cpp_source = cg.emit(program);
        result.warnings = cg.warnings();

        if (!cg.errors().empty()) {
            result.errors = cg.errors();
            return result;
        }

        // 5. Write C++ output
        std::string cpp_path = opts_.output_cpp.empty()
            ? (std::filesystem::temp_directory_path() / "__drv_out__.cpp").string()
            : opts_.output_cpp;

        {
            std::ofstream f(cpp_path);
            if (!f) throw std::runtime_error("Cannot write: " + cpp_path);
            f << result.cpp_source;
        }

        // 6. Invoke backend compiler (if --exe)
        if (!opts_.output_exe.empty()) {
            bool ok = invokeBackend(cpp_path);
            if (!ok) {
                result.errors.push_back("backend compilation failed");
                return result;
            }
        }

        result.success = true;

    } catch (const std::exception& ex) {
        result.errors.push_back(std::string("fatal: ") + ex.what());
    }

    return result;
}

} // namespace drv
