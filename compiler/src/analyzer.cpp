#include "analyzer.h"
#include <algorithm>
#include <sstream>

namespace drv {

// ── helpers ───────────────────────────────────────────────────────────────────
void Analyzer::emitError  (int l, const std::string& m) { diags_.push_back({DiagKind::Error,   file_, l, m}); }
void Analyzer::emitWarning(int l, const std::string& m) { diags_.push_back({DiagKind::Warning, file_, l, m}); }
void Analyzer::emitNote   (int l, const std::string& m) { diags_.push_back({DiagKind::Note,    file_, l, m}); }

bool Analyzer::hasErrors()   const {
    return std::any_of(diags_.begin(), diags_.end(), [](auto& d){ return d.isError(); });
}
bool Analyzer::hasWarnings() const {
    return std::any_of(diags_.begin(), diags_.end(), [](auto& d){ return d.isWarning(); });
}
std::vector<std::string> Analyzer::errors() const {
    std::vector<std::string> r;
    for (auto& d : diags_) if (d.isError())   r.push_back(d.format());
    return r;
}
std::vector<std::string> Analyzer::warnings() const {
    std::vector<std::string> r;
    for (auto& d : diags_) if (d.isWarning()) r.push_back(d.format());
    return r;
}

bool Analyzer::hasAnnotation(const std::vector<std::string>& annots,
                              const std::string& name) {
    return std::any_of(annots.begin(), annots.end(),
                       [&](const std::string& a){ return a.rfind(name,0)==0; });
}

std::string Analyzer::exprVarName(const Expr& e) {
    if (auto* id = dynamic_cast<const IdentExpr*>(&e)) return id->name;
    return "";
}

// ─────────────────────────────────────────────────────────────────────────────
// Main entry
// ─────────────────────────────────────────────────────────────────────────────
bool Analyzer::analyze(const Program& prog) {
    // Pass 2 setup: collect function signatures with @noalias params
    collectFuncInfo(prog);

    // Walk all top-level statements
    for (auto& s : prog.stmts) {
        if (!s) continue;

        // Pass 3: @packed inheritance
        if (auto* cls = dynamic_cast<const ClassDecl*>(s.get()))
            checkPackedInheritance(*cls);

        // Pass 4: atomic<string>
        if (auto* vd = dynamic_cast<const VarDecl*>(s.get()))
            checkAtomicString(*vd);

        // Pass 1: thread safety (top-level parallel for)
        if (auto* fr = dynamic_cast<const ForRangeStmt*>(s.get()))
            if (fr->is_parallel) analyzeParallelFor(*fr);
        if (auto* fe = dynamic_cast<const ForEachStmt*>(s.get()))
            if (fe->is_parallel) analyzeParallelForEach(*fe);

        // Pass 2: aliasing at statement level
        analyzeStmtAlias(*s);
    }

    return !hasErrors();
}

// ─────────────────────────────────────────────────────────────────────────────
// Pass 1: Thread-safety
// ─────────────────────────────────────────────────────────────────────────────

// Collect all variable names declared at the given scope level
void Analyzer::collectOuterVars(const Program& prog,
                                 std::unordered_set<std::string>& out) {
    for (auto& s : prog.stmts) {
        if (auto* vd = dynamic_cast<const VarDecl*>(s.get()))
            out.insert(vd->name);
        if (auto* fd = dynamic_cast<const FuncDecl*>(s.get()))
            out.insert(fd->name);
    }
}

// Recursively scan stmt for assignments to variables in the outer scope
// without atomic/fence protection
void Analyzer::checkWriteInParallel(const Stmt& s,
                                     const ParallelScope& scope,
                                     bool in_body) {
    if (!in_body) return;

    // Check ExprStmt containing an assignment
    if (auto* es = dynamic_cast<const ExprStmt*>(&s)) {
        if (auto* bin = dynamic_cast<const BinaryExpr*>(es->expr.get())) {
            static const std::vector<std::string> ASSIGN_OPS = {
                "=", "+=", "-=", "*=", "/=", "%="
            };
            bool is_assign = std::any_of(ASSIGN_OPS.begin(), ASSIGN_OPS.end(),
                                         [&](const std::string& op){ return bin->op == op; });
            if (is_assign) {
                std::string var = exprVarName(*bin->left);
                if (!var.empty() &&
                    scope.outer_vars.count(var) &&
                    !scope.reductions.count(var)) {
                    emitError(0,
                        "thread-unsafe: variable '" + var +
                        "' is modified inside parallel for without atomic or reduction qualifier; "
                        "add reduction(+:" + var + ") or declare as atomic<T>");
                }
            }
        }
    }

    // VarDecl inside body is safe (local to thread)

    // Recurse into sub-blocks
    auto recurse = [&](const StmtList& stmts) {
        for (auto& child : stmts)
            if (child) checkWriteInParallel(*child, scope, true);
    };

    if (auto* ifs = dynamic_cast<const IfStmt*>(&s)) {
        recurse(ifs->then_body);
        recurse(ifs->else_body);
    }
    if (auto* wh = dynamic_cast<const WhileStmt*>(&s))  recurse(wh->body);
    if (auto* fr = dynamic_cast<const ForRangeStmt*>(&s)) recurse(fr->body);
    if (auto* fe = dynamic_cast<const ForEachStmt*>(&s))  recurse(fe->body);
}

void Analyzer::analyzeParallelFor(const ForRangeStmt& s) {
    if (!s.is_parallel) return;

    ParallelScope scope;
    scope.is_parallel = true;
    // reduction variables are safe — format is "op:varname" e.g. "+:total"
    for (auto& rv : s.reductions) {
        auto colon = rv.find(':');
        if (colon != std::string::npos)
            scope.reductions.insert(rv.substr(colon+1));
        else
            scope.reductions.insert(rv);
    }

    // We don't have full program context here; heuristic: flag any assignment
    // to a name that looks like a captured outer variable (not the loop var).
    // The loop variable itself is safe.
    scope.outer_vars.insert("__all__"); // sentinel: we'll check all non-loop vars

    for (auto& st : s.body) {
        if (!st) continue;
        // Simplified: scan ExprStmt assignments
        if (auto* es = dynamic_cast<const ExprStmt*>(st.get())) {
            if (auto* bin = dynamic_cast<const BinaryExpr*>(es->expr.get())) {
                static const std::vector<std::string> ASSIGN_OPS = {"=","+=","-=","*=","/=","%="};
                bool is_assign = std::any_of(ASSIGN_OPS.begin(), ASSIGN_OPS.end(),
                    [&](const std::string& op){ return bin->op == op; });
                if (is_assign) {
                    std::string var = exprVarName(*bin->left);
                    if (!var.empty() && var != s.var &&
                        !scope.reductions.count(var)) {
                        // Warn: shared variable modified without synchronization
                        emitWarning(0,
                            "possible thread-unsafe write to '" + var +
                            "' inside parallel for — add reduction(op:" + var +
                            ") or atomic<T>, or use sys.sync.fence_full()");
                    }
                }
            }
        }
    }
}

void Analyzer::analyzeParallelForEach(const ForEachStmt& s) {
    if (!s.is_parallel) return;
    for (auto& st : s.body) {
        if (!st) continue;
        if (auto* es = dynamic_cast<const ExprStmt*>(st.get())) {
            if (auto* bin = dynamic_cast<const BinaryExpr*>(es->expr.get())) {
                static const std::vector<std::string> ASSIGN_OPS = {"=","+=","-=","*=","/=","%="};
                bool is_assign = std::any_of(ASSIGN_OPS.begin(), ASSIGN_OPS.end(),
                    [&](const std::string& op){ return bin->op == op; });
                if (is_assign) {
                    std::string var = exprVarName(*bin->left);
                    if (!var.empty() && var != s.var) {
                        emitWarning(0,
                            "possible thread-unsafe write to '" + var +
                            "' inside parallel for — use atomic<T> or reduction");
                    }
                }
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Pass 2: Aliasing verifier
// ─────────────────────────────────────────────────────────────────────────────

void Analyzer::collectFuncInfo(const Program& prog) {
    for (auto& s : prog.stmts) {
        auto* fd = dynamic_cast<const FuncDecl*>(s.get());
        if (!fd) continue;
        FuncInfo info;
        info.name = fd->name;
        info.line = 0; // line info not tracked in AST currently
        // @noalias declared on function-level annotations applies to all params
        bool fn_noalias = hasAnnotation(fd->annotations, "@noalias");
        for (auto& p : fd->params) {
            if (fn_noalias) {
                info.noalias_params.push_back(p.name);
            }
        }
        if (!info.noalias_params.empty())
            func_registry_.push_back(std::move(info));
    }
}

void Analyzer::checkAliasingCall(const CallExpr& call, int line) {
    // Find the callee in our registry
    std::string callee_name;
    if (auto* id = dynamic_cast<const IdentExpr*>(call.callee.get()))
        callee_name = id->name;
    if (callee_name.empty()) return;

    for (auto& fi : func_registry_) {
        if (fi.name != callee_name) continue;
        // Check if any two arguments have the same variable name
        std::vector<std::string> arg_vars;
        for (auto& a : call.args)
            arg_vars.push_back(a ? exprVarName(*a) : "");

        for (size_t i = 0; i < arg_vars.size(); ++i) {
            if (arg_vars[i].empty()) continue;
            for (size_t j = i+1; j < arg_vars.size(); ++j) {
                if (!arg_vars[j].empty() && arg_vars[i] == arg_vars[j]) {
                    emitError(line,
                        "aliasing violation: '" + arg_vars[i] +
                        "' passed twice to @noalias function '" + callee_name +
                        "' — arguments at positions " + std::to_string(i) +
                        " and " + std::to_string(j) + " must not alias");
                }
            }
        }
    }
}

void Analyzer::analyzeExprAlias(const Expr& e, int line) {
    if (auto* call = dynamic_cast<const CallExpr*>(&e))
        checkAliasingCall(*call, line);
    // Recurse into sub-expressions via BinaryExpr, etc. (simplified)
}

void Analyzer::analyzeStmtAlias(const Stmt& s) {
    if (auto* es = dynamic_cast<const ExprStmt*>(&s))
        if (es->expr) analyzeExprAlias(*es->expr, 0);
    if (auto* vd = dynamic_cast<const VarDecl*>(&s))
        if (vd->init) analyzeExprAlias(*vd->init, 0);
    // Recurse
    auto recurse = [&](const StmtList& stmts) {
        for (auto& st : stmts) if (st) analyzeStmtAlias(*st);
    };
    if (auto* ifs = dynamic_cast<const IfStmt*>(&s)) {
        recurse(ifs->then_body); recurse(ifs->else_body);
    }
    if (auto* wh  = dynamic_cast<const WhileStmt*>(&s))     recurse(wh->body);
    if (auto* fr  = dynamic_cast<const ForRangeStmt*>(&s))  recurse(fr->body);
    if (auto* fe  = dynamic_cast<const ForEachStmt*>(&s))   recurse(fe->body);
    if (auto* fd  = dynamic_cast<const FuncDecl*>(&s))      recurse(fd->body);
    if (auto* cls = dynamic_cast<const ClassDecl*>(&s))     recurse(cls->methods);
}

// ─────────────────────────────────────────────────────────────────────────────
// Pass 3: @packed + inheritance check
// ─────────────────────────────────────────────────────────────────────────────
void Analyzer::checkPackedInheritance(const ClassDecl& cls) {
    if (hasAnnotation(cls.annotations, "@packed") && !cls.base.empty()) {
        emitError(0,
            "class '" + cls.name + "' cannot use @packed with inheritance (extends '" +
            cls.base + "') — @packed removes vtable padding guarantees; "
            "use composition instead");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Pass 4: atomic<string> detection
// ─────────────────────────────────────────────────────────────────────────────
void Analyzer::checkAtomicString(const VarDecl& v) {
    if (v.type.name == "atomic" &&
        !v.type.args.empty() &&
        (v.type.args[0].name == "String" || v.type.args[0].name == "string")) {
        emitWarning(0,
            "atomic<String> for variable '" + v.name +
            "' is rewritten internally as atomic<char*> pointer wrapper — "
            "string contents are NOT atomically updated, only the pointer; "
            "consider using a mutex for full string atomicity");
    }
}

} // namespace drv
