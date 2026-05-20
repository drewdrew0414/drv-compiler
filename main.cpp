#include "compiler/src/compiler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>

// ── Exit codes ───────────────────────────────────────────────────────────────
// 0  success
// 1  compile error  (lex / parse / analyze / codegen)
// 2  backend error  (clang/g++ failed)
// 3  I/O error      (file not found / read-write failure)
// 4  internal error (unexpected exception / OOM)
static constexpr int EXIT_COMPILE  = 1;
static constexpr int EXIT_BACKEND  = 2;
static constexpr int EXIT_IO       = 3;
static constexpr int EXIT_INTERNAL = 4;

// ── ANSI color helpers ───────────────────────────────────────────────────────
struct Colors {
    const char* bold    = "";
    const char* red     = "";
    const char* yellow  = "";
    const char* cyan    = "";
    const char* reset   = "";
};
static Colors makeColors(bool use) {
    Colors c;
    if (use) {
        c.bold   = "\033[1m";
        c.red    = "\033[1;31m";
        c.yellow = "\033[1;33m";
        c.cyan   = "\033[1;36m";
        c.reset  = "\033[0m";
    }
    return c;
}

// ── Source cache ─────────────────────────────────────────────────────────────
// Loads and caches source files so each is read at most once.
class SourceCache {
    std::unordered_map<std::string, std::vector<std::string>> data_;
public:
    const std::string* getLine(const std::string& path, int line) {
        auto it = data_.find(path);
        if (it == data_.end()) {
            std::ifstream f(path);
            auto& lines = data_[path];
            if (f) {
                std::string l;
                while (std::getline(f, l)) lines.push_back(l);
            }
            it = data_.find(path);
        }
        auto& v = it->second;
        if (line < 1 || line > (int)v.size()) return nullptr;
        return &v[line - 1];
    }
};

// ── Diagnostic string parser ──────────────────────────────────────────────────
// Parses "file:line:col: level: message" (Unix paths, no ':' in filename).
struct Diag {
    std::string file, level, message;
    int line{0}, col{0};
    bool located{false};
};

static Diag parseDiag(const std::string& s) {
    Diag d;
    d.message = s;
    // Scan for the first ":digits:digits: word: " pattern
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != ':') continue;
        size_t j = i + 1;
        if (j >= s.size() || !std::isdigit((unsigned char)s[j])) continue;
        size_t ln0 = j;
        while (j < s.size() && std::isdigit((unsigned char)s[j])) ++j;
        int ln = std::stoi(s.substr(ln0, j - ln0));
        if (j >= s.size() || s[j] != ':') continue;
        ++j;
        size_t cn0 = j;
        while (j < s.size() && std::isdigit((unsigned char)s[j])) ++j;
        if (cn0 == j) continue;
        int cn = std::stoi(s.substr(cn0, j - cn0));
        if (j + 1 >= s.size() || s[j] != ':' || s[j+1] != ' ') continue;
        j += 2;
        size_t lv0 = j;
        while (j < s.size() && std::isalpha((unsigned char)s[j])) ++j;
        std::string lv = s.substr(lv0, j - lv0);
        if (lv != "error" && lv != "warning" && lv != "note") continue;
        if (j + 1 >= s.size() || s[j] != ':' || s[j+1] != ' ') continue;
        d.file    = s.substr(0, i);
        d.line    = ln;
        d.col     = cn;
        d.level   = lv;
        d.message = s.substr(j + 2);
        d.located = true;
        return d;
    }
    return d;
}

// ── Backend output filter ─────────────────────────────────────────────────────
// Replace C++ internal names with dri-facing equivalents so backend errors
// are readable by users who don't know the generated C++ types.
static std::string filterBackendLine(const std::string& line) {
    struct Sub { const char* from; const char* to; };
    // Order matters: more specific patterns must come before general ones.
    static const Sub subs[] = {
        {"std::basic_string<char>",    "String"},
        {"basic_string<char>",         "String"},
        {"std::string",                "String"},
        {"int32_t",                    "int"},
        {"int64_t",                    "long"},
        {"std::vector<int32_t>",       "list<int>"},
        {"std::vector<int64_t>",       "list<long>"},
        {"std::vector<double>",        "list<double>"},
        {"std::vector<float>",         "list<float>"},
        {"std::vector<String>",        "list<String>"},
        {"std::vector<std::string>",   "list<String>"},
        {"std::vector",                "list"},
        {"std::unordered_map",         "Map"},
        {"std::unique_ptr",            "Own"},
        {"std::shared_ptr",            "Ref"},
        {"std::optional",              "Option"},
        {"DrvResult",                  "Result"},
        {"__drv_print",                "print"},
        {"__drv_lst_filter",           "lst.filter"},
        {"__drv_lst_map",              "lst.map"},
        {"__drv_lst_reduce",           "lst.reduce"},
        {"__drv_entry__",              "<module-init>"},
        {"__drv_to_str",               "<to_string>"},
    };
    std::string r = line;
    for (auto& sub : subs) {
        std::string f = sub.from, t = sub.to;
        for (size_t p = 0; (p = r.find(f, p)) != std::string::npos; ) {
            r.replace(p, f.size(), t);
            p += t.size();
        }
    }

    // Remove redundant "(aka '...')" clauses produced by clang when both
    // the display name and alias are the same after our substitutions
    // e.g. "'int' (aka 'int')" → "'int'"
    for (size_t p = 0; p < r.size(); ) {
        auto ak = r.find(" (aka '", p);
        if (ak == std::string::npos) break;
        auto close = r.find("')", ak + 7);
        if (close == std::string::npos) break;
        std::string inner = r.substr(ak + 7, close - (ak + 7));
        // Find the quoted type just before "(aka ...)"
        size_t q1 = r.rfind("'", ak - 1);
        if (q1 != std::string::npos) {
            size_t q0 = r.rfind("'", q1 - 1);
            if (q0 != std::string::npos) {
                std::string outer = r.substr(q0 + 1, q1 - q0 - 1);
                if (outer == inner) {
                    r.erase(ak, close - ak + 2); // remove " (aka 'X')"
                    continue;
                }
            }
        }
        p = close + 2;
    }
    return r;
}

// ── Diagnostic printer ────────────────────────────────────────────────────────
// Prints one diagnostic with source context (source line + caret underline).
static void printDiag(const std::string& msg, SourceCache& cache,
                      const Colors& C, bool is_warning = false) {
    Diag d = parseDiag(msg);
    const char* lvl_color = is_warning ? C.yellow : C.red;

    if (!d.located) {
        std::cerr << lvl_color << msg << C.reset << '\n';
        return;
    }

    // "file:line:col: error: message"
    std::cerr << C.bold << d.file << ":" << d.line << ":" << d.col << ": "
              << lvl_color << d.level << ": "
              << C.reset << C.bold << d.message << C.reset << '\n';

    // Source context
    auto* src = cache.getLine(d.file, d.line);
    if (src) {
        std::string lno = std::to_string(d.line);
        std::string pad(lno.size(), ' ');

        std::cerr << "  " << pad << " |\n";
        std::cerr << "  " << lno << " | " << *src << '\n';

        // Caret — account for tabs (expand to 4 spaces each)
        std::string prefix;
        int target = std::max(0, d.col - 1);
        for (int k = 0; k < (int)src->size() && k < target; ++k)
            prefix += ((*src)[k] == '\t') ? "    " : " ";
        std::cerr << "  " << pad << " | " << lvl_color << prefix << "^" << C.reset << '\n';
    }
}

static void printUsage(const char* prog) {
    std::cerr
        << "dri compiler — transpiles .dri source to C++20\n\n"
        << "Usage:\n"
        << "  " << prog << " <input.dri>               compile and run (script mode)\n"
        << "  " << prog << " <input.dri> [options]\n\n"
        << "Options:\n"
        << "  --exe <file>      Build executable\n"
        << "  --cpp <file>      Output C++ source only\n"
        << "  --check           Type-check only, no code generation\n"
        << "  --debug           Debug build (-O0, -g, bounds checks)\n"
        << "  --release         Release build (-O3, -march=native, LTO)\n"
        << "  --opt <0-3>       Manual optimisation level (default 1)\n"
        << "  --native          Enable -march=native\n"
        << "  --lto             Enable Link-Time Optimisation\n"
        << "  --trace <file>    Export parallel-loop trace (Chrome DevTools JSON)\n"
        << "  --incremental     Skip unchanged files (uses .dri_cache/)\n"
        << "  --cache-dir <d>   Set incremental cache directory\n"
        << "  --no-analyze      Disable static analysis (thread-safety, aliasing)\n"
        << "  --target <triple> Cross-compile target  (e.g. x86_64-linux-gnu)\n"
        << "  --sysroot <path>  Sysroot for cross-compilation\n"
        << "  --cross-cxx <cc>  Explicit cross-compiler binary\n"
        << "  --source-map <f>  Emit source-map JSON  (cpp_line → dri_line)\n"
        << "  -D<FLAG>          Define compile-time flag for static_if\n"
        << "  --version         Show compiler version\n"
        << "  --help            Show this help\n\n"
        << "Exit codes:\n"
        << "  0  success\n"
        << "  1  compile error  (lex/parse/analyze/codegen)\n"
        << "  2  backend error  (clang/g++ failed)\n"
        << "  3  I/O error      (file not found or read/write failure)\n"
        << "  4  internal error (unexpected exception)\n";
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
            try {
                opts.opt_level = std::stoi(argv[++i]);
            } catch (const std::exception&) {
                std::cerr << "dri: --opt requires an integer 0-3 (got '"
                          << argv[i] << "')\n";
                return 1;
            }
            if (opts.opt_level < 0 || opts.opt_level > 3) {
                std::cerr << "dri: --opt must be 0-3 (got " << opts.opt_level << ")\n";
                return 1;
            }
        } else if (arg == "--native") {
            opts.native = true;
        } else if (arg == "--lto") {
            opts.lto = true;
        } else if (arg == "--trace" && i+1 < argc) {
            opts.trace_file = argv[++i];
        } else if (arg == "--incremental") {
            opts.incremental = true;
        } else if (arg == "--cache-dir" && i+1 < argc) {
            opts.cache_dir = argv[++i];
        } else if (arg == "--no-analyze") {
            opts.no_analyze = true;
        } else if (arg == "--target" && i+1 < argc) {
            opts.target_triple = argv[++i];
        } else if (arg == "--sysroot" && i+1 < argc) {
            opts.sysroot = argv[++i];
        } else if (arg == "--cross-cxx" && i+1 < argc) {
            opts.cross_cxx = argv[++i];
        } else if (arg == "--source-map" && i+1 < argc) {
            opts.source_map_file = argv[++i];
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

    // ── Script mode: no --exe / --cpp / --check → compile and run immediately ─
    bool run_mode = opts.output_exe.empty() && opts.output_cpp.empty() && !opts.check_only;
    std::string tmp_exe;
    if (run_mode) {
        tmp_exe = std::filesystem::temp_directory_path().string()
                + "/dri_run_" + std::to_string(::getpid());
        opts.output_exe = tmp_exe;
    }

    const Colors C = makeColors(isatty(STDERR_FILENO) != 0);
    SourceCache src_cache;

    drv::Compiler compiler(opts);
    auto result = compiler.compile();

    // ── Warnings ─────────────────────────────────────────────────────────────
    for (auto& w : result.warnings)
        printDiag(w, src_cache, C, /*is_warning=*/true);

    // ── Errors ───────────────────────────────────────────────────────────────
    if (!result.errors.empty()) {
        for (auto& e : result.errors)
            printDiag(e, src_cache, C);

        // Backend output — show filtered, human-readable lines
        if (!result.backend_output.empty()) {
            std::cerr << C.cyan << "── C++ backend output ──" << C.reset << '\n';

            // Count backend errors so we can cap noisy output
            int backend_err_count = 0;
            constexpr int kMaxBackendErrors = 8;
            bool truncated = false;

            std::istringstream bss(result.backend_output);
            std::string bline;
            while (std::getline(bss, bline)) {
                if (bline.empty()) continue;

                // Skip lines that only reference the internal temp .cpp file
                // and have no .dri context — they're not useful to the user.
                bool mentions_dri = bline.find(".dri") != std::string::npos;
                bool mentions_tmp = bline.find("__drv_out__") != std::string::npos
                                 || bline.find("__drv_backend") != std::string::npos;
                if (mentions_tmp && !mentions_dri) continue;

                // Suppress verbose "note: candidate template ignored / not viable"
                // lines — they come from standard library headers and confuse users.
                if (bline.find("note: candidate") != std::string::npos)  continue;
                if (bline.find("note: implicit deduction") != std::string::npos) continue;
                if (bline.find("_LIBCPP_") != std::string::npos)         continue;
                if (bline.find("_HIDE_FROM_ABI") != std::string::npos)   continue;
                // Suppress SDK header paths (not useful to dri users)
                if (bline.find("/CommandLineTools/SDKs/") != std::string::npos) continue;
                if (bline.find("/usr/include/c++/") != std::string::npos)       continue;

                // Cap total backend error lines shown
                if (bline.find(": error:") != std::string::npos ||
                    bline.find("error generated") != std::string::npos) {
                    ++backend_err_count;
                    if (backend_err_count > kMaxBackendErrors) {
                        truncated = true;
                        continue;
                    }
                }

                // Translate internal C++ names to dri names and print.
                // Don't add our own source context here — clang's output
                // already contains its own source lines (the "N | code" blocks).
                // Adding ours would show the same line twice.
                std::string filtered = filterBackendLine(bline);

                // Highlight the top-level error/warning line; print others as-is
                Diag bd = parseDiag(filtered);
                if (bd.located && (bd.level == "error" || bd.level == "warning")) {
                    const char* lc = bd.level == "error" ? C.red : C.yellow;
                    std::cerr << "  " << C.bold << bd.file << ":" << bd.line
                              << ":" << bd.col << ": " << lc << bd.level
                              << ": " << C.reset << bd.message << '\n';
                } else {
                    std::cerr << "  " << filtered << '\n';
                }
            }
            if (truncated)
                std::cerr << "  " << C.yellow << "... ("
                          << (backend_err_count - kMaxBackendErrors)
                          << " more backend error(s) suppressed; "
                             "use --cpp to inspect generated C++)"
                          << C.reset << '\n';
        }

        // ── Summary ──────────────────────────────────────────────────────────
        int nerr  = (int)result.errors.size();
        int nwarn = (int)result.warnings.size();
        std::cerr << C.bold << C.red << nerr << " error(s)";
        if (nwarn > 0) std::cerr << C.reset << C.bold << ", " << nwarn << " warning(s)";
        std::cerr << " generated." << C.reset << '\n';

        // Clean up temp exe if it was partially created
        if (run_mode && std::filesystem::exists(tmp_exe))
            std::filesystem::remove(tmp_exe);

        // Map error kind to exit code
        switch (result.error_kind) {
            case drv::ErrorKind::Backend:  return EXIT_BACKEND;
            case drv::ErrorKind::IO:       return EXIT_IO;
            case drv::ErrorKind::Internal: return EXIT_INTERNAL;
            default:                       return EXIT_COMPILE;
        }
    }

    // ── Success (warnings only) ───────────────────────────────────────────────
    if (!result.warnings.empty()) {
        std::cerr << C.bold << result.warnings.size()
                  << " warning(s) generated." << C.reset << '\n';
    }

    if (opts.check_only) {
        std::cerr << C.cyan << opts.input_file << ": OK" << C.reset << '\n';
        return 0;
    }

    if (run_mode) {
        int rc = std::system(tmp_exe.c_str());
        std::filesystem::remove(tmp_exe);
        return rc;
    }

    return 0;
}
