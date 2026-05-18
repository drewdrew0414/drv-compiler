#pragma once
#include "ast.h"
#include <string>
#include <unordered_set>
#include <vector>

namespace drv {

// ── Diagnostic severity ───────────────────────────────────────────────────────
enum class DiagKind { Error, Warning, Note };

struct AnalyzerDiag {
    DiagKind    kind;
    std::string file;
    int         line{0};
    std::string msg;

    bool isError()   const { return kind == DiagKind::Error; }
    bool isWarning() const { return kind == DiagKind::Warning; }

    std::string format() const {
        std::string prefix = file + ":" + std::to_string(line) + ": ";
        switch (kind) {
            case DiagKind::Error:   return prefix + "error: "   + msg;
            case DiagKind::Warning: return prefix + "warning: " + msg;
            default:                return prefix + "note: "    + msg;
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Static Analyzer
// Passes:
//   1. Thread-safety check  — detects unsynchronized shared mutations in parallel for
//   2. Aliasing verifier    — validates @noalias annotations at call sites
//   3. @packed inheritance  — error if @packed class uses extends
//   4. atomic<string> check — warns and prepares string atomic rewrite marker
// ─────────────────────────────────────────────────────────────────────────────
class Analyzer {
public:
    explicit Analyzer(std::string filename = "<unknown>") : file_(std::move(filename)) {}

    // Run all passes on the program; returns true if no errors
    bool analyze(const Program& prog);

    const std::vector<AnalyzerDiag>& diags()    const { return diags_; }
    bool hasErrors()   const;
    bool hasWarnings() const;

    // Convenience: collect just errors / warnings as strings
    std::vector<std::string> errors()   const;
    std::vector<std::string> warnings() const;

private:
    std::string              file_;
    std::vector<AnalyzerDiag> diags_;

    void emitError  (int line, const std::string& msg);
    void emitWarning(int line, const std::string& msg);
    void emitNote   (int line, const std::string& msg);

    // ── Pass 1: Thread-safety ─────────────────────────────────────────────────
    // Tracks variables declared outside a parallel-for body that are written
    // inside without atomic/reduction protection.
    struct ParallelScope {
        std::unordered_set<std::string> outer_vars;   // vars visible from outside
        std::unordered_set<std::string> reductions;   // reduction variables (safe)
        bool is_parallel{false};
    };

    void analyzeStmt      (const Stmt& s, ParallelScope& scope);
    void analyzeParallelFor(const ForRangeStmt& s);
    void analyzeParallelForEach(const ForEachStmt& s);
    void collectOuterVars (const Program& prog, std::unordered_set<std::string>& out);
    void checkWriteInParallel(const Stmt& s, const ParallelScope& scope, bool in_body);

    // ── Pass 2: Aliasing verifier ─────────────────────────────────────────────
    // At each call site, if callee has @noalias params, verify no two args
    // refer to the same variable.
    struct FuncInfo {
        std::string name;
        std::vector<std::string> noalias_params;  // param names with @noalias
        int line{0};
    };
    std::vector<FuncInfo> func_registry_;

    void collectFuncInfo   (const Program& prog);
    void checkAliasingCall (const CallExpr& call, int line);
    void analyzeExprAlias  (const Expr& e, int line);
    void analyzeStmtAlias  (const Stmt& s);

    // ── Pass 3: @packed inheritance check ────────────────────────────────────
    void checkPackedInheritance(const ClassDecl& cls);

    // ── Pass 4: atomic<string> ────────────────────────────────────────────────
    void checkAtomicString(const VarDecl& v);

    // ── Helpers ───────────────────────────────────────────────────────────────
    static std::string exprVarName(const Expr& e);  // returns "" if not a simple var
    static bool        hasAnnotation(const std::vector<std::string>& annots,
                                     const std::string& name);
};

} // namespace drv
