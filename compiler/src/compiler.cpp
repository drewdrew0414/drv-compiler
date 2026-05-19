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
#include <unordered_set>
#include <algorithm>
#include <cctype>

namespace drv {

namespace fs = std::filesystem;

Compiler::Compiler(CompileOptions opts) : opts_(std::move(opts)) {}

// ── Security helpers ──────────────────────────────────────────────────────────

// Validate a target triple: only allow [A-Za-z0-9._-] so it is safe to
// embed in shell command flags like --target=<triple>.
static bool isValidTargetTriple(const std::string& t) {
    if (t.empty()) return false;
    return std::all_of(t.begin(), t.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '-' || c == '_' || c == '.';
    });
}

// Validate a module name (simple identifier: [A-Za-z0-9_] only).
// Rejects path-traversal sequences (../), absolute paths, etc.
static bool isValidModuleName(const std::string& name) {
    if (name.empty()) return false;
    return std::all_of(name.begin(), name.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '_';
    });
}

// Validate a -D<define> value: allow [A-Za-z0-9_=.+-@] only so
// metacharacters cannot escape the shell context.
static bool isValidDefine(const std::string& d) {
    if (d.empty()) return false;
    return std::all_of(d.begin(), d.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '_' || c == '=' || c == '.'
                               || c == '+' || c == '-' || c == '@';
    });
}

// ─────────────────────────────────────────────────────────────────────────────

// Maximum source file size: 50 MiB.  Prevents OOM from maliciously large
// files, and keeps parse times within reasonable bounds.
static constexpr std::streamoff kMaxSourceBytes = 50LL * 1024 * 1024;

std::string Compiler::readSource() {
    std::ifstream f(opts_.input_file, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file: " + opts_.input_file);
    f.seekg(0, std::ios::end);
    auto sz = f.tellg();
    f.seekg(0, std::ios::beg);
    if (sz > kMaxSourceBytes)
        throw std::runtime_error("Source file too large (>" +
                                 std::to_string(kMaxSourceBytes / (1024*1024)) +
                                 " MiB): " + opts_.input_file);
    std::string buf;
    if (sz > 0) buf.resize(static_cast<size_t>(sz));
    if (sz > 0) f.read(buf.data(), sz);
    else        buf.assign(std::istreambuf_iterator<char>(f), {});
    if (f.bad()) throw std::runtime_error("I/O error while reading: " + opts_.input_file);
    return buf;
}

void Compiler::writeCpp(const std::string& src) {
    std::string path = opts_.output_cpp.empty()
        ? (fs::temp_directory_path() / "__drv_out__.cpp").string()
        : opts_.output_cpp;
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f.write(src.data(), static_cast<std::streamsize>(src.size()));
    if (!f) throw std::runtime_error("Write failed: " + path);
}

bool Compiler::invokeBackend(const std::string& cpp_path) {
    if (opts_.output_exe.empty()) return true;

#ifdef _WIN32
    const std::string null_dev = "NUL";
#else
    const std::string null_dev = "/dev/null";
#endif

    // Shell-quote a single token.  Escapes embedded backslashes and double
    // quotes so the string is safe to embed in a POSIX shell command line.
    auto q = [](const std::string& s) {
        std::string out;
        out.reserve(s.size() + 2);
        out.push_back('"');
        for (char c : s) {
            if (c == '\\' || c == '"') out.push_back('\\');
            out.push_back(c);
        }
        out.push_back('"');
        return out;
    };

    // Probe whether a *known-safe* compiler binary exists by running
    // `binary --version`.  'name' must already be a safe identifier
    // (caller is responsible — only hardcoded names or validated triples
    // should reach here).
    auto probe = [&](const std::string& name) -> bool {
        return std::system((q(name) + " --version >" + null_dev + " 2>&1").c_str()) == 0;
    };

    std::string cxx;
    std::string flags = "-std=c++20 ";
    bool is_cross = !opts_.target_triple.empty();

    // ── Cross-compilation mode ─────────────────────────────────────────────
    if (!opts_.cross_cxx.empty()) {
        // User-provided compiler binary; used verbatim (it is their own binary).
        // q() ensures spaces/special chars in the path don't break the shell cmd.
        cxx = opts_.cross_cxx;
    } else if (is_cross) {
        // Try clang++ with --target= (most portable cross-compile approach)
        if (probe("clang++")) {
            cxx = "clang++";
            // target_triple was already validated in compile() → safe to embed
            flags += "--target=" + opts_.target_triple + " ";
            if (!opts_.sysroot.empty())
                flags += "--sysroot=" + q(opts_.sysroot) + " ";
            const std::string& t = opts_.target_triple;
            if      (t.find("aarch64")  != std::string::npos) flags += "-march=armv8-a ";
            else if (t.find("armv7")    != std::string::npos) flags += "-march=armv7-a -mfpu=neon ";
            else if (t.find("riscv64")  != std::string::npos) flags += "-march=rv64gc -mabi=lp64d ";
            else if (t.find("wasm32")   != std::string::npos) flags += "--sysroot=/dev/null ";
            if (t.find("linux") != std::string::npos && opts_.native)
                flags += "-mavx512f -mavx512bw -mavx512dq -mavx512vl ";
        } else {
            // GCC cross-compiler (e.g. aarch64-linux-gnu-g++)
            // target_triple validated → gcc_cross is a safe identifier
            std::string gcc_cross = opts_.target_triple + "-g++";
            if (probe(gcc_cross)) {
                cxx = gcc_cross;
                if (!opts_.sysroot.empty())
                    flags += "--sysroot=" + q(opts_.sysroot) + " ";
            } else {
                std::cerr << "dri: cross-compiler not found for target '"
                          << opts_.target_triple << "'\n"
                          << "  Tried: clang++ --target=..., " << gcc_cross << "\n"
                          << "  Install: sudo apt-get install gcc-"
                          << opts_.target_triple << "\n"
                          << "  Or use --cross-cxx to specify compiler path\n";
                return false;
            }
        }
    } else {
        // ── Native compilation ─────────────────────────────────────────────
        if      (probe("clang++")) cxx = "clang++";
        else if (probe("g++"))     cxx = "g++";
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
    for (auto& d : opts_.defines)  flags += q("-D" + d) + " ";

#ifndef _WIN32
    // ── OpenMP availability probe ─────────────────────────────────────────
    // Apple's Xcode clang++ does not ship the OpenMP runtime by default,
    // so we compile a minimal test program before committing to -fopenmp.
    // If the probe fails we fall back to single-threaded execution and print
    // a hint about installing libomp (brew install libomp).
    {
        namespace fs = std::filesystem;
        const std::string omp_src = (fs::temp_directory_path()
                                     / "__drv_omp_probe__.cpp").string();
        const std::string omp_out = (fs::temp_directory_path()
                                     / "__drv_omp_probe__").string();
        bool omp_ok = false;
        {
            std::ofstream tf(omp_src);
            if (tf) tf << "#include <omp.h>\nint main(){return omp_get_max_threads();}\n";
        }
        if (std::filesystem::exists(omp_src)) {
            std::string omp_cmd = q(cxx) + " -fopenmp " + q(omp_src)
                                  + " -o " + q(omp_out)
                                  + " >" + null_dev + " 2>&1";
            omp_ok = (std::system(omp_cmd.c_str()) == 0);
            std::remove(omp_src.c_str());
            std::remove(omp_out.c_str());
        }

        if (omp_ok) {
            flags += "-fopenmp ";
        } else {
            std::cerr << "dri: warning: OpenMP not available with " << cxx
                      << " — parallel for loops will run single-threaded.\n"
                      << "  macOS: brew install libomp  "
                         "(then re-run with the Homebrew clang: "
                         "brew install llvm)\n"
                      << "  Linux: sudo apt-get install libomp-dev\n";
        }
    }
#endif

    // q(cxx) prevents a user-supplied cross_cxx path containing spaces or
    // special characters from splitting into multiple shell tokens.
    std::string cmd = q(cxx) + " " + flags + " " + q(cpp_path) +
                      " -o " + q(opts_.output_exe) +
#ifndef _WIN32
                      " -lm "
#endif
                      " 2>&1";
    int ret = std::system(cmd.c_str());
    if (ret != 0 && is_cross) {
        std::cerr << "dri: cross-compilation failed for target '"
                  << opts_.target_triple << "'\n"
                  << "     Check that target sysroot is installed.\n";
    }
    return ret == 0;
}

CompileResult Compiler::compile() {
    CompileResult result;

    try {
        // 0. Validate options
        if (opts_.input_file.empty()) {
            result.errors.push_back("dri: no input file");
            return result;
        }
        if (opts_.opt_level < 0 || opts_.opt_level > 3) {
            result.errors.push_back("dri: --opt level must be 0-3 (got " +
                                    std::to_string(opts_.opt_level) + ")");
            return result;
        }
        if (opts_.debug && opts_.release) {
            result.errors.push_back("dri: --debug and --release are mutually exclusive");
            return result;
        }
        // Validate --target triple: only [A-Za-z0-9._-] allowed so it cannot
        // inject shell metacharacters into the backend command string.
        if (!opts_.target_triple.empty() && !isValidTargetTriple(opts_.target_triple)) {
            result.errors.push_back("dri: --target contains invalid characters: '" +
                                    opts_.target_triple + "'");
            return result;
        }
        // Validate -D defines: reject values with shell metacharacters.
        for (auto& d : opts_.defines) {
            if (!isValidDefine(d)) {
                result.errors.push_back("dri: -D define contains unsafe characters: '" +
                                        d + "'");
                return result;
            }
        }

        // Single source of truth for the .cpp output path
        const std::string cpp_path = opts_.output_cpp.empty()
            ? (fs::temp_directory_path() / "__drv_out__.cpp").string()
            : opts_.output_cpp;

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
        //     - Each `use foo;` resolves once against {<input_dir>, "."} search dirs.
        //     - Modules are loaded in declaration order; duplicates suppressed.
        //     - Lexer/parser diagnostics from modules are surfaced into the result
        //       so failures are visible instead of silently ignored.
        {
            fs::path input_dir = fs::path(opts_.input_file).parent_path();
            std::vector<fs::path> include_dirs = { input_dir, fs::path(".") };
            std::unordered_set<std::string> loaded;
            std::vector<StmtPtr> prepended;
            bool module_failed = false;

            for (auto& s : program.stmts) {
                auto* ud = dynamic_cast<const UseDecl*>(s.get());
                if (!ud) continue;
                if (!loaded.insert(ud->module).second) continue;

                // Guard against path-traversal: module names must be simple
                // identifiers (alphanumeric + underscore).  Reject anything
                // that could resolve outside the project directory.
                if (!isValidModuleName(ud->module)) {
                    result.errors.push_back("dri: module name contains illegal "
                        "characters (path traversal attempt?): '" +
                        ud->module + "'");
                    module_failed = true;
                    continue;
                }

                fs::path resolved;
                for (auto& dir : include_dirs) {
                    fs::path cand = dir / (ud->module + ".dri");
                    std::error_code ec;
                    if (fs::exists(cand, ec)) { resolved = cand; break; }
                }
                if (resolved.empty()) continue; // not found → defer to codegen/link

                std::ifstream mf(resolved, std::ios::binary);
                if (!mf) {
                    result.errors.push_back(resolved.string() + ": error: cannot open module");
                    module_failed = true;
                    continue;
                }
                std::string msrc{std::istreambuf_iterator<char>(mf), {}};

                Lexer ml(msrc, resolved.string());
                auto mtoks = ml.tokenize();
                if (ml.hasErrors()) {
                    for (auto& e : ml.errors()) result.errors.push_back(e);
                    module_failed = true;
                    continue;
                }
                Parser mp(std::move(mtoks), resolved.string());
                auto mprog = mp.parse();
                if (mp.hasErrors()) {
                    for (auto& e : mp.errors()) result.errors.push_back(e);
                    module_failed = true;
                    continue;
                }
                // Move module decls into the prepend buffer (preserves
                // declaration order across multiple modules)
                for (auto& ms : mprog.stmts)
                    prepended.push_back(std::move(ms));
            }

            if (module_failed) return result;

            if (!prepended.empty()) {
                program.stmts.insert(
                    program.stmts.begin(),
                    std::make_move_iterator(prepended.begin()),
                    std::make_move_iterator(prepended.end()));
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

        // 3d. Incremental build: reuse cached .cpp when source hasn't changed
        if (opts_.incremental) {
            std::string cache_root = opts_.cache_dir.empty()
                ? fs::path(opts_.input_file).parent_path().string()
                : opts_.cache_dir;
            IncrementalCache cache(cache_root);
            if (cache.isUpToDate(opts_.input_file, cpp_path)) {
                // .cpp is up to date — skip transpile, just compile if needed
                if (!opts_.output_exe.empty()) {
                    bool ok = invokeBackend(cpp_path);
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
        {
            std::ofstream f(cpp_path, std::ios::binary);
            if (!f) throw std::runtime_error("Cannot write: " + cpp_path);
            f.write(result.cpp_source.data(),
                    static_cast<std::streamsize>(result.cpp_source.size()));
            if (!f) throw std::runtime_error("Write failed: " + cpp_path);
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
