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

// ── i18n ─────────────────────────────────────────────────────────────────────
static std::string g_lang = "en";  // set from --lang option

struct LangStrings {
    const char* error;
    const char* warning;
    const char* note;
    const char* generated_errors;
    const char* generated_warnings;
    const char* build_failed;
    const char* build_ok;
};

static const LangStrings LANG_EN = {
    "error", "warning", "note",
    "error(s) generated.", "warning(s) generated.",
    "build failed.", "build succeeded:"
};
static const LangStrings LANG_KO = {
    "오류", "경고", "알림",
    "개 오류 발생.", "개 경고 발생.",
    "빌드 실패.", "빌드 성공:"
};
static const LangStrings LANG_JA = {
    "エラー", "警告", "注意",
    "個のエラーが発生しました。", "個の警告が発生しました。",
    "ビルド失敗。", "ビルド成功:"
};

static const LangStrings& getLang() {
    if (g_lang == "ko") return LANG_KO;
    if (g_lang == "ja") return LANG_JA;
    return LANG_EN;
}

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
    // Scan for "file:line:col: level: msg"  OR  "file:line: level: msg"
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != ':') continue;
        size_t j = i + 1;
        if (j >= s.size() || !std::isdigit((unsigned char)s[j])) continue;
        size_t ln0 = j;
        while (j < s.size() && std::isdigit((unsigned char)s[j])) ++j;
        int ln = std::stoi(s.substr(ln0, j - ln0));
        if (j >= s.size() || s[j] != ':') continue;
        ++j;

        int cn = 1;
        // Optional column number
        if (j < s.size() && std::isdigit((unsigned char)s[j])) {
            size_t cn0 = j;
            while (j < s.size() && std::isdigit((unsigned char)s[j])) ++j;
            cn = std::stoi(s.substr(cn0, j - cn0));
            if (j >= s.size() || s[j] != ':') continue;
            ++j;
        }

        if (j >= s.size() || s[j] != ' ') continue;
        ++j;
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

    // "file:line:col: error: message" — with i18n level label
    const auto& L = getLang();
    const char* lv_label = (d.level == "error")   ? L.error :
                           (d.level == "warning")  ? L.warning : L.note;
    std::cerr << C.bold << d.file << ":" << d.line << ":" << d.col << ": "
              << lvl_color << lv_label << ": "
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

// ── dri.drpm manifest parser ─────────────────────────────────────────────────
// Supports a simple subset of TOML:
//   key = "value"
//   key = ["a", "b", "c"]
//   key = true/false
//   [section]
struct DrpmManifest {
    std::string name;
    std::string version;
    std::string main_file  = "src/main.dri";
    std::string output;
    bool        release{false};
    bool        debug_build{false};
    int         opt_level{1};
    std::vector<std::string> link_libs;
    std::vector<std::string> search_dirs;
    std::vector<std::string> defines;
};

static std::string drpmTrim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::vector<std::string> drpmParseArray(const std::string& s) {
    // Parses ["a", "b", "c"] → {"a","b","c"}
    std::vector<std::string> out;
    size_t i = s.find('[');
    if (i == std::string::npos) {
        // single unquoted value
        auto t = drpmTrim(s);
        if (!t.empty() && t[0]=='"') t = t.substr(1, t.size()-2);
        if (!t.empty()) out.push_back(t);
        return out;
    }
    ++i;
    while (i < s.size()) {
        size_t q = s.find('"', i);
        if (q == std::string::npos) break;
        size_t q2 = s.find('"', q+1);
        if (q2 == std::string::npos) break;
        out.push_back(s.substr(q+1, q2-q-1));
        i = q2 + 1;
    }
    return out;
}

static bool parseDrpm(const std::string& path, DrpmManifest& m, std::string& err) {
    std::ifstream f(path);
    if (!f) { err = "cannot open: " + path; return false; }
    std::string section;
    std::string line;
    int lineno = 0;
    while (std::getline(f, line)) {
        ++lineno;
        // strip comment
        auto ci = line.find('#');
        if (ci != std::string::npos) line = line.substr(0, ci);
        line = drpmTrim(line);
        if (line.empty()) continue;
        if (line[0] == '[') {
            auto e = line.find(']');
            section = (e != std::string::npos) ? line.substr(1, e-1) : "";
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = drpmTrim(line.substr(0, eq));
        std::string val = drpmTrim(line.substr(eq+1));
        // strip surrounding quotes from string values
        auto strVal = [&]() -> std::string {
            if (val.size() >= 2 && val.front()=='"' && val.back()=='"')
                return val.substr(1, val.size()-2);
            return val;
        };
        auto boolVal = [&]() -> bool {
            return (val == "true" || val == "1" || val == "yes");
        };
        auto intVal = [&]() -> int {
            try { return std::stoi(val); } catch(...) { return 1; }
        };
        if      (key=="name")        m.name        = strVal();
        else if (key=="version")     m.version     = strVal();
        else if (key=="main")        m.main_file   = strVal();
        else if (key=="output")      m.output      = strVal();
        else if (key=="release")     m.release     = boolVal();
        else if (key=="debug")       m.debug_build = boolVal();
        else if (key=="opt")         m.opt_level   = intVal();
        else if (key=="link")        m.link_libs   = drpmParseArray(val);
        else if (key=="search_dirs") m.search_dirs = drpmParseArray(val);
        else if (key=="defines")     m.defines     = drpmParseArray(val);
        // [build] section overrides
        else if (section=="build" && key=="release")  m.release     = boolVal();
        else if (section=="build" && key=="debug")    m.debug_build = boolVal();
        else if (section=="build" && key=="opt")      m.opt_level   = intVal();
        else if (section=="build" && key=="link")     m.link_libs   = drpmParseArray(val);
        else if (section=="build" && key=="search_dirs") m.search_dirs = drpmParseArray(val);
        else if (section=="build" && key=="defines")  m.defines     = drpmParseArray(val);
        else if (section=="build" && key=="output")   m.output      = strVal();
    }
    if (m.name.empty()) { err = "manifest missing 'name' field"; return false; }
    if (m.main_file.empty()) { err = "manifest missing 'main' field"; return false; }
    return true;
}

static int runBuild(int argc, char* argv[], const Colors& C) {
    namespace fs = std::filesystem;

    // Look for dri.drpm in current directory
    std::string manifest_path = "dri.drpm";
    bool release_override = false, debug_override = false;
    std::string output_override;

    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if      (a == "--release") release_override = true;
        else if (a == "--debug")   debug_override   = true;
        else if (a == "--output" && i+1 < argc) output_override = argv[++i];
        else if (a == "--manifest" && i+1 < argc) manifest_path = argv[++i];
    }

    if (!fs::exists(manifest_path)) {
        std::cerr << C.red << "dri: " << C.reset
                  << "no dri.drpm found in current directory\n"
                  << "  Create one with:\n\n"
                  << "    name = \"myproject\"\n"
                  << "    version = \"0.1.0\"\n"
                  << "    main = \"src/main.dri\"\n\n";
        return 1;
    }

    DrpmManifest manifest;
    std::string parse_err;
    if (!parseDrpm(manifest_path, manifest, parse_err)) {
        std::cerr << C.red << "dri: dri.drpm parse error: " << C.reset << parse_err << "\n";
        return 1;
    }

    if (release_override) manifest.release     = true;
    if (debug_override)   manifest.debug_build = true;
    if (!output_override.empty()) manifest.output = output_override;
    if (manifest.output.empty()) manifest.output = manifest.name;

    // Resolve paths relative to the manifest file's directory
    fs::path manifest_dir = fs::path(manifest_path).parent_path();
    auto resolve = [&](const std::string& p) -> std::string {
        fs::path fp(p);
        if (fp.is_absolute()) return p;
        return (manifest_dir / fp).string();
    };
    manifest.main_file = resolve(manifest.main_file);
    if (!manifest.output.empty() && !fs::path(manifest.output).is_absolute())
        manifest.output = (manifest_dir / manifest.output).string();
    for (auto& d : manifest.search_dirs) d = resolve(d);

    std::cerr << C.cyan << "dri build" << C.reset
              << "  " << manifest.name << " v" << manifest.version
              << "  [" << (manifest.release ? "release" : manifest.debug_build ? "debug" : "dev") << "]\n"
              << "  main: " << manifest.main_file
              << "  →  " << manifest.output << "\n";

    if (!fs::exists(manifest.main_file)) {
        std::cerr << C.red << "dri: main file not found: " << C.reset << manifest.main_file << "\n";
        return 1;
    }

    drv::CompileOptions opts;
    opts.input_file   = manifest.main_file;
    opts.output_exe   = manifest.output;
    opts.release      = manifest.release;
    opts.debug        = manifest.debug_build;
    opts.opt_level    = manifest.opt_level;
    opts.link_libs    = manifest.link_libs;
    opts.search_dirs  = manifest.search_dirs;
    opts.defines      = manifest.defines;
    if (manifest.release) { opts.opt_level = 3; opts.native = true; opts.lto = true; }

    SourceCache src_cache;
    drv::Compiler compiler(opts);
    auto result = compiler.compile();

    for (auto& w : result.warnings)
        printDiag(w, src_cache, C, /*is_warning=*/true);
    if (!result.success) {
        for (auto& e : result.errors)
            printDiag(e, src_cache, C);
        if (!result.backend_output.empty())
            std::cerr << result.backend_output << "\n";
        std::cerr << C.red << "build failed.\n" << C.reset;
        return 1;
    }
    std::cerr << C.cyan << "build succeeded: " << C.reset << manifest.output << "\n";
    return 0;
}

static int runInit(int argc, char* argv[], const Colors& C) {
    namespace fs = std::filesystem;
    std::string name = (argc >= 3) ? argv[2] : "myproject";

    if (fs::exists("dri.drpm")) {
        std::cerr << C.yellow << "dri init: " << C.reset
                  << "dri.drpm already exists — not overwriting\n";
        return 1;
    }

    // Create directory structure
    fs::create_directories("src");
    fs::create_directories("tests");

    // dri.drpm
    {
        std::ofstream f("dri.drpm");
        f << "name = \"" << name << "\"\n"
          << "version = \"0.1.0\"\n"
          << "main = \"src/main.dri\"\n"
          << "output = \"bin/" << name << "\"\n\n"
          << "[build]\n"
          << "release = false\n"
          << "link = []\n";
    }

    // src/main.dri
    {
        std::ofstream f("src/main.dri");
        f << "module " << name << ";\n\n"
          << "print(\"Hello from " << name << "!\");\n";
    }

    // tests/hello.dri
    {
        std::ofstream f("tests/hello.dri");
        f << "# " << name << " basic test\n"
          << "module test_hello;\n"
          << "print(\"test ok\");\n";
    }

    // .gitignore
    {
        std::ofstream f(".gitignore");
        f << "bin/\n.dri_cache/\n*.cpp\n";
    }

    fs::create_directories("bin");

    std::cerr << C.cyan << "dri init" << C.reset << "  " << name << "\n"
              << "  created: dri.drpm  src/main.dri  tests/hello.dri  .gitignore\n"
              << "  run with: dri build && ./bin/" << name << "\n";
    return 0;
}

static int runTest(int argc, char* argv[], const Colors& C) {
    namespace fs = std::filesystem;

    // Find test files
    std::string test_dir = "tests";
    if (argc >= 3) test_dir = argv[2];

    std::string dri_bin = argv[0];  // path to this compiler
    if (!fs::exists(dri_bin)) dri_bin = "./cmake-build-debug/dri";

    if (!fs::exists(test_dir)) {
        std::cerr << C.red << "dri test: " << C.reset
                  << "no tests/ directory found\n";
        return 1;
    }

    int pass = 0, fail = 0;
    std::vector<std::string> failed;

    for (auto& entry : fs::directory_iterator(test_dir)) {
        if (entry.path().extension() != ".dri") continue;
        std::string src = entry.path().string();
        std::string name = entry.path().stem().string();

        // Check for expected output file
        fs::path exp_path = entry.path().parent_path() / (name + ".expected");

        std::string tmp_out = "/tmp/dri_test_" + name + ".out";
        std::string cmd = dri_bin + " \"" + src + "\" > \"" + tmp_out + "\" 2>/dev/null";
        int rc = std::system(cmd.c_str());

        if (rc != 0) {
            std::cerr << "  FAIL: " << name << " (exit " << rc << ")\n";
            ++fail;
            failed.push_back(name);
            continue;
        }

        if (fs::exists(exp_path)) {
            // Compare with expected
            std::string cmp_cmd = "diff -q \"" + exp_path.string() + "\" \"" + tmp_out + "\" >/dev/null 2>&1";
            if (std::system(cmp_cmd.c_str()) == 0) {
                std::cerr << "  PASS: " << name << "\n";
                ++pass;
            } else {
                std::cerr << "  FAIL: " << name << " (output mismatch)\n";
                ++fail;
                failed.push_back(name);
            }
        } else {
            // No expected file: pass if exit code 0
            std::cerr << "  PASS: " << name << " (exit-code only)\n";
            ++pass;
        }
        fs::remove(tmp_out);
    }

    std::cerr << "\n  PASS: " << pass << "   FAIL: " << fail << "\n";
    if (!failed.empty()) {
        std::cerr << "  Failed: ";
        for (auto& f : failed) std::cerr << f << " ";
        std::cerr << "\n";
    }
    return fail == 0 ? 0 : 1;
}

static void printUsage(const char* prog) {
    std::cerr
        << "dri compiler — transpiles .dri source to C++20\n\n"
        << "Usage:\n"
        << "  " << prog << " <input.dri>               compile and run (script mode)\n"
        << "  " << prog << " <input.dri> [options]\n"
        << "  " << prog << " build                     build from dri.drpm manifest\n"
        << "  " << prog << " init [name]               scaffold a new project\n"
        << "  " << prog << " test [dir]                run tests in tests/ directory\n\n"
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
        << "  --strict          Treat all warnings as errors\n"
        << "  --link <libs>     Link system libraries  (comma-separated, e.g. m,pthread)\n"
        << "  --lang <ko|en|ja> Diagnostic message language (default: en)\n"
        << "  --bench-json <f>  Write @bench timings to a JSON file at program exit\n"
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
        std::cout << "dri compiler v0.1.1 (spec: 2026-05, C++20 backend)\n";
        return 0;
    }

    const Colors C_early = makeColors(isatty(STDERR_FILENO) != 0);
    if (std::strcmp(argv[1], "build") == 0) return runBuild(argc, argv, C_early);
    if (std::strcmp(argv[1], "init")  == 0) return runInit (argc, argv, C_early);
    if (std::strcmp(argv[1], "test")  == 0) return runTest (argc, argv, C_early);

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
        } else if (arg == "--strict") {
            opts.strict = true;
        } else if (arg == "--bench-json" && i+1 < argc) {
            opts.bench_json_file = argv[++i];
        } else if (arg == "--lang" && i+1 < argc) {
            opts.lang = argv[++i];
            if (opts.lang != "ko" && opts.lang != "en" && opts.lang != "ja") {
                std::cerr << "dri: --lang must be ko, en, or ja (got '" << opts.lang << "')\n";
                return 1;
            }
            g_lang = opts.lang;
        } else if (arg == "--link" && i+1 < argc) {
            // Accept comma-separated list: --link m,pthread  or  --link m --link pthread
            std::string libs = argv[++i];
            std::string cur;
            for (char c : libs) {
                if (c == ',') { if (!cur.empty()) { opts.link_libs.push_back(cur); cur.clear(); } }
                else cur += c;
            }
            if (!cur.empty()) opts.link_libs.push_back(cur);
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
    // In --strict mode promote warnings to errors before checking result.errors.
    if (opts.strict && !result.warnings.empty()) {
        for (auto& w : result.warnings) {
            // Re-print as error
            std::string as_err = w;
            // Replace " warning:" with " error:" for the strict-mode display
            auto pos = as_err.find(" warning:");
            if (pos != std::string::npos) as_err.replace(pos, 9, " error:");
            printDiag(as_err, src_cache, C, /*is_warning=*/false);
        }
        std::cerr << C.red << result.warnings.size() << " warning(s) promoted to error(s) by --strict\n" << C.reset;
        // Show warning count as normal path
        if (result.success) {
            // Fail the build
            std::cerr << C.red << result.warnings.size() << " error(s) generated (--strict).\n" << C.reset;
            return EXIT_COMPILE;
        }
    } else {
        for (auto& w : result.warnings)
            printDiag(w, src_cache, C, /*is_warning=*/true);
    }

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
        const auto& LS = getLang();
        std::cerr << C.bold << C.red << nerr << " " << LS.generated_errors;
        if (nwarn > 0) std::cerr << C.reset << C.bold << "  " << nwarn << " " << LS.generated_warnings;
        std::cerr << C.reset << '\n';

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
                  << " " << getLang().generated_warnings << C.reset << '\n';
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
