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
    // Pass 6 setup: collect functions returning Option/Result
    collectResultFuncs(prog);

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

        // Pass 5: move-after-use (inside function bodies)
        if (auto* fd = dynamic_cast<const FuncDecl*>(s.get())) {
            std::unordered_map<std::string,int> moved;
            checkMoveInBody(fd->body, moved);
        }

        // Pass 6: unchecked Option/Result in function bodies
        if (auto* fd = dynamic_cast<const FuncDecl*>(s.get()))
            checkUncheckedResult(fd->body);
    }

    // Pass 5 at top level: detect move-after-use in module-level code
    {
        std::unordered_map<std::string,int> moved;
        checkMoveInBody(prog.stmts, moved);
    }
    // Pass 6 at top level: unchecked Option/Result in module-level code
    checkUncheckedResult(prog.stmts);

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

// ─────────────────────────────────────────────────────────────────────────────
// Pass 5: move-after-use detection
// ─────────────────────────────────────────────────────────────────────────────

void Analyzer::checkMoveExpr(const Expr& e, int line,
                              std::unordered_map<std::string,int>& moved) {
    // If this expression reads a moved variable, error
    if (auto* id = dynamic_cast<const IdentExpr*>(&e)) {
        auto it = moved.find(id->name);
        if (it != moved.end()) {
            emitError(line > 0 ? line : it->second,
                "use-after-move: variable '" + id->name +
                "' was moved at line " + std::to_string(it->second) +
                " and cannot be used again (assign a new value first)");
        }
        return;
    }
    // Detect move(x) → mark x as moved
    if (auto* mv = dynamic_cast<const MoveExpr*>(&e)) {
        if (auto* id = dynamic_cast<const IdentExpr*>(mv->operand.get())) {
            moved[id->name] = line;
        }
        return; // don't recurse into the moved operand
    }
    // Recurse
    if (auto* bin = dynamic_cast<const BinaryExpr*>(&e)) {
        // Left side of assignment: being written, not read → clear moved state
        if (bin->op == "=" || bin->op == "+=" || bin->op == "-=" ||
            bin->op == "*=" || bin->op == "/=" || bin->op == "%=") {
            if (auto* id = dynamic_cast<const IdentExpr*>(bin->left.get()))
                moved.erase(id->name);  // re-assignment clears moved state
            if (bin->right) checkMoveExpr(*bin->right, line, moved);
            return;
        }
        if (bin->left)  checkMoveExpr(*bin->left,  line, moved);
        if (bin->right) checkMoveExpr(*bin->right, line, moved);
    }
    if (auto* un = dynamic_cast<const UnaryExpr*>(&e))
        if (un->operand) checkMoveExpr(*un->operand, line, moved);
    if (auto* call = dynamic_cast<const CallExpr*>(&e)) {
        if (call->callee) checkMoveExpr(*call->callee, line, moved);
        for (auto& a : call->args) if (a) checkMoveExpr(*a, line, moved);
    }
    if (auto* idx = dynamic_cast<const IndexExpr*>(&e)) {
        if (idx->object) checkMoveExpr(*idx->object, line, moved);
        if (idx->index)  checkMoveExpr(*idx->index,  line, moved);
    }
    if (auto* mem = dynamic_cast<const MemberExpr*>(&e))
        if (mem->object) checkMoveExpr(*mem->object, line, moved);
}

void Analyzer::checkMoveInBody(const StmtList& stmts,
                                std::unordered_map<std::string,int>& moved) {
    for (auto& s : stmts) {
        if (!s) continue;
        int line = s->line;

        if (auto* vd = dynamic_cast<const VarDecl*>(s.get())) {
            // VarDecl with initializer: check the init expr, then clear moved for the new var
            if (vd->init) checkMoveExpr(*vd->init, line, moved);
            moved.erase(vd->name);  // new binding is always valid
        } else if (auto* es = dynamic_cast<const ExprStmt*>(s.get())) {
            if (es->expr) checkMoveExpr(*es->expr, line, moved);
        } else if (auto* ret = dynamic_cast<const ReturnStmt*>(s.get())) {
            if (ret->value) checkMoveExpr(*ret->value, line, moved);
        } else if (auto* ifs = dynamic_cast<const IfStmt*>(s.get())) {
            if (ifs->cond) checkMoveExpr(*ifs->cond, line, moved);
            // Branch-scoped: take a copy for each branch
            auto moved_then = moved, moved_else = moved;
            checkMoveInBody(ifs->then_body, moved_then);
            checkMoveInBody(ifs->else_body, moved_else);
            // After if: union of moved from both branches
            for (auto& [v,l] : moved_then) moved[v] = l;
            for (auto& [v,l] : moved_else) moved[v] = l;
        } else if (auto* wh = dynamic_cast<const WhileStmt*>(s.get())) {
            if (wh->cond) checkMoveExpr(*wh->cond, line, moved);
            checkMoveInBody(wh->body, moved);
        } else if (auto* fr = dynamic_cast<const ForRangeStmt*>(s.get())) {
            checkMoveInBody(fr->body, moved);
        } else if (auto* fe = dynamic_cast<const ForEachStmt*>(s.get())) {
            checkMoveInBody(fe->body, moved);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Pass 6: unchecked Option<T> / Result<T> return values
// ─────────────────────────────────────────────────────────────────────────────

void Analyzer::collectResultFuncs(const Program& prog) {
    for (auto& s : prog.stmts) {
        auto* fd = dynamic_cast<const FuncDecl*>(s.get());
        if (!fd) continue;
        const std::string& rt = fd->ret_type.name;
        if (rt == "Option" || rt == "Result" || rt == "Some" || rt == "Ok" || rt == "Err")
            result_returning_funcs_.insert(fd->name);
    }
}

void Analyzer::checkUncheckedResult(const StmtList& stmts) {
    for (auto& s : stmts) {
        if (!s) continue;
        // An ExprStmt whose expression is a bare function call that returns Option/Result
        if (auto* es = dynamic_cast<const ExprStmt*>(s.get())) {
            if (auto* call = dynamic_cast<const CallExpr*>(es->expr.get())) {
                std::string callee;
                if (auto* id = dynamic_cast<const IdentExpr*>(call->callee.get()))
                    callee = id->name;
                if (!callee.empty() && result_returning_funcs_.count(callee)) {
                    emitWarning(s->line,
                        "unchecked result: return value of '" + callee +
                        "' (Option/Result) is discarded — handle with match or unwrap");
                }
            }
        }
        // Recurse
        auto recurse = [&](const StmtList& body) { checkUncheckedResult(body); };
        if (auto* ifs = dynamic_cast<const IfStmt*>(s.get())) {
            recurse(ifs->then_body); recurse(ifs->else_body);
        }
        if (auto* wh = dynamic_cast<const WhileStmt*>(s.get())) recurse(wh->body);
        if (auto* fr = dynamic_cast<const ForRangeStmt*>(s.get())) recurse(fr->body);
        if (auto* fe = dynamic_cast<const ForEachStmt*>(s.get())) recurse(fe->body);
        if (auto* fd = dynamic_cast<const FuncDecl*>(s.get())) recurse(fd->body);
    }
}

} // namespace drv
