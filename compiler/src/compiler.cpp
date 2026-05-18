#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "analyzer.h"
#include "incremental.h"

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

#ifdef _WIN32
    const std::string null_dev = "NUL";
#else
    const std::string null_dev = "/dev/null";
#endif
    auto probe = [&](const std::string& cmd) -> bool {
        return std::system((cmd + " --version >" + null_dev + " 2>&1").c_str()) == 0;
    };
    auto q = [](const std::string& s) { return "\"" + s + "\""; };

    std::string cxx;
    std::string flags = "-std=c++20 ";
    bool is_cross = !opts_.target_triple.empty();

    // ── Cross-compilation mode ─────────────────────────────────────────────
    if (!opts_.cross_cxx.empty()) {
        // User explicitly specified cross compiler
        cxx = opts_.cross_cxx;
    } else if (is_cross) {
        // Try clang++ with --target= (most portable cross-compile approach)
        if (probe("clang++")) {
            cxx = "clang++";
            flags += "--target=" + opts_.target_triple + " ";
            // Sysroot for finding system headers/libs of target OS
            if (!opts_.sysroot.empty())
                flags += "--sysroot=" + q(opts_.sysroot) + " ";
            // Architecture-specific flags based on triple
            const std::string& t = opts_.target_triple;
            if (t.find("aarch64")  != std::string::npos) flags += "-march=armv8-a ";
            else if (t.find("armv7") != std::string::npos) flags += "-march=armv7-a -mfpu=neon ";
            else if (t.find("riscv64") != std::string::npos) flags += "-march=rv64gc -mabi=lp64d ";
            else if (t.find("wasm32") != std::string::npos) flags += "--sysroot=/dev/null ";
            // AVX-512 preset for HPC Linux targets
            if (t.find("linux") != std::string::npos && opts_.native)
                flags += "-mavx512f -mavx512bw -mavx512dq -mavx512vl ";
        } else {
            // Try GCC cross-compiler binary (e.g. aarch64-linux-gnu-g++)
            std::string gcc_cross = opts_.target_triple + "-g++";
            if (probe(gcc_cross)) {
                cxx = gcc_cross;
                if (!opts_.sysroot.empty()) flags += "--sysroot=" + q(opts_.sysroot) + " ";
            } else {
                std::cerr << "dri: cross-compiler not found for target '" << opts_.target_triple
                          << "'\n  Tried: clang++ --target=..., " << gcc_cross << "\n"
                          << "  Install: sudo apt-get install gcc-" << opts_.target_triple << "\n"
                          << "  Or use --cross-cxx to specify compiler path\n";
                return false;
            }
        }
    } else {
        // ── Native compilation ─────────────────────────────────────────────
        if (probe("clang++")) cxx = "clang++";
        else if (probe("g++")) cxx = "g++";
        else {
            std::cerr << "dri: no C++ compiler found (tried clang++, g++)\n";
            return false;
        }
    }

    // ── Build flags ────────────────────────────────────────────────────────
    if (opts_.debug)        flags += "-O0 -g -DDEBUG ";
    else if (opts_.release) flags += "-O3 -DNDEBUG ";
    else                    flags += "-O" + std::to_string(opts_.opt_level) + " ";

    if (opts_.native && !is_cross) flags += "-march=native ";
    if (opts_.lto)                 flags += "-flto ";
    for (auto& d : opts_.defines)  flags += "-D" + d + " ";

    // OpenMP — skip on Windows (needs separate runtime DLL)
#ifndef _WIN32
    flags += "-fopenmp ";
#endif

    std::string cmd = cxx + " " + flags + " " + q(cpp_path) +
                      " -o " + q(opts_.output_exe) +
#ifndef _WIN32
                      " -lm "
#endif
                      " 2>&1";
    int ret = std::system(cmd.c_str());
    if (ret != 0 && is_cross) {
        std::cerr << "dri: cross-compilation failed for target '" << opts_.target_triple << "'\n"
                  << "     Check that target sysroot is installed.\n";
    }
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

        // 3b. Module resolution: load referenced .dri files
        {
            namespace fs = std::filesystem;
            fs::path input_dir = fs::path(opts_.input_file).parent_path();
            std::vector<std::string> include_dirs = { input_dir.string(), "." };
            for (auto& s : program.stmts) {
                auto* ud = dynamic_cast<const UseDecl*>(s.get());
                if (!ud) continue;
                for (auto& dir : include_dirs) {
                    fs::path mod_path = fs::path(dir) / (ud->module + ".dri");
                    if (!fs::exists(mod_path)) continue;
                    std::ifstream mf(mod_path);
                    if (!mf) continue;
                    std::string msrc{std::istreambuf_iterator<char>(mf), {}};
                    Lexer ml(msrc, mod_path.string());
                    auto mtoks = ml.tokenize();
                    if (ml.hasErrors()) break;
                    Parser mp(std::move(mtoks), mod_path.string());
                    auto mprog = mp.parse();
                    if (mp.hasErrors()) break;
                    // Inject module declarations before current program
                    for (auto& ms : mprog.stmts)
                        program.stmts.insert(program.stmts.begin(), std::move(const_cast<StmtPtr&>(ms)));
                    break;
                }
            }
        }

        // 3c. Static analysis (thread-safety + aliasing + @packed checks)
        if (!opts_.no_analyze) {
            Analyzer analyzer(opts_.input_file);
            bool ok = analyzer.analyze(program);
            // Collect warnings regardless
            for (auto& w : analyzer.warnings())
                result.warnings.push_back(w);
            if (!ok) {
                for (auto& e : analyzer.errors())
                    result.errors.push_back(e);
                return result;
            }
        }

        if (opts_.check_only) {
            result.success = true;
            return result;
        }

        // 3d. Incremental build: determine cpp output path early so we can check cache
        std::string cpp_path_early = opts_.output_cpp.empty()
            ? (fs::temp_directory_path() / "__drv_out__.cpp").string()
            : opts_.output_cpp;

        if (opts_.incremental) {
            std::string cache_root = opts_.cache_dir.empty()
                ? fs::path(opts_.input_file).parent_path().string()
                : opts_.cache_dir;
            IncrementalCache cache(cache_root);
            if (cache.isUpToDate(opts_.input_file, cpp_path_early)) {
                // .cpp is up to date — skip transpile, just compile if needed
                if (!opts_.output_exe.empty()) {
                    bool ok = invokeBackend(cpp_path_early);
                    if (!ok) { result.errors.push_back("backend compilation failed"); return result; }
                }
                result.success = true;
                return result;
            }
        }

        // 4. Code generation
        CodegenOptions cg_opts;
        cg_opts.source_file = opts_.input_file;
        cg_opts.debug_build = opts_.debug;
        cg_opts.release_build = opts_.release;
        cg_opts.opt_level = opts_.opt_level;
        cg_opts.emit_line_directives = true;
        cg_opts.trace_file = opts_.trace_file;
        cg_opts.source_map_file = opts_.source_map_file;
        cg_opts.target_triple = opts_.target_triple;

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

        // 7. Update incremental cache on success
        if (opts_.incremental) {
            std::string cache_root = opts_.cache_dir.empty()
                ? fs::path(opts_.input_file).parent_path().string()
                : opts_.cache_dir;
            IncrementalCache cache(cache_root);
            cache.updateEntry(opts_.input_file);
        }

        result.success = true;

    } catch (const std::exception& ex) {
        result.errors.push_back(std::string("fatal: ") + ex.what());
    }

    return result;
}

} // namespace drv
