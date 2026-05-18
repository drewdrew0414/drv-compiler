#pragma once
#include "ast.h"
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace drv {

struct CodegenOptions {
    bool   emit_line_directives{true};  // #line N "file.dri"
    bool   debug_build{false};
    bool   release_build{false};
    int    opt_level{1};
    std::string source_file;
    std::string trace_file;             // --trace output (Chrome DevTools JSON)
};

class Codegen {
public:
    explicit Codegen(CodegenOptions opts = {});

    std::string emit(const Program& prog);
    std::string emitExpr(const Expr& e);

    const std::vector<std::string>& errors()   const { return errors_; }
    const std::vector<std::string>& warnings() const { return warnings_; }
    std::string getOutput() const { return out_.str(); }

private:
    CodegenOptions opts_;
    std::ostringstream out_;
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;
    int indent_{0};

    // impl-for pre-processing: class_name → list of ImplDecl*
    std::unordered_map<std::string, std::vector<const ImplDecl*>> impls_by_class_;
    // smart pointer variable tracking: var name → "own" or "ref"
    std::unordered_map<std::string, std::string> smart_ptr_vars_;
    // @trace state
    bool tracing_{false};
    std::string tracing_func_;

    // ── emit helpers ──────────────────────────────────────────────────────────
    void write(const std::string& s)  { out_ << s; }
    void writeln(const std::string& s = "") { out_ << s << '\n'; }
    void writeIndent() { for (int i=0;i<indent_;++i) out_ << "    "; }
    void writei(const std::string& s) { writeIndent(); write(s); }
    void writeil(const std::string& s){ writeIndent(); writeln(s); }
    static std::string toForwardSlash(std::string s) {
        for (auto& c : s) if (c == '\\') c = '/';
        return s;
    }
    void lineDir(int line) {
        if (opts_.emit_line_directives && !opts_.source_file.empty())
            writeln("#line " + std::to_string(line) + " \"" + toForwardSlash(opts_.source_file) + "\"");
    }

    // ── type mapping ──────────────────────────────────────────────────────────
    std::string mapType(const TypeRef& tr);
    std::string mapBuiltinType(const std::string& name);

    // ── statement emitters ────────────────────────────────────────────────────
    void emitStmt(const Stmt& s);
    void emitVarDecl(const VarDecl& s);
    void emitFuncDecl(const FuncDecl& s);
    void emitClassDecl(const ClassDecl& s);
    void emitTraitDecl(const TraitDecl& s);
    void emitImplDecl(const ImplDecl& s);
    void emitDimDecl(const DimDecl& s);
    void emitModuleDecl(const ModuleDecl& s);
    void emitUseDecl(const UseDecl& s);
    void emitExternDecl(const ExternDecl& s);
    void emitIfStmt(const IfStmt& s);
    void emitForRange(const ForRangeStmt& s);
    void emitForEach(const ForEachStmt& s);
    void emitForCStyle(const ForCStyleStmt& s);
    void emitWhileStmt(const WhileStmt& s);
    void emitSwitchStmt(const SwitchStmt& s);
    void emitMatchStmt(const MatchStmt& s);
    void emitTryCatch(const TryCatchStmt& s);
    void emitRegion(const RegionStmt& s);
    void emitSpawn(const SpawnStmt& s);
    void emitStaticIf(const StaticIfStmt& s);
    void emitBlock(const StmtList& stmts);

    // ── expression emitters ───────────────────────────────────────────────────
    std::string emitBinary(const BinaryExpr& e);
    std::string emitUnary(const UnaryExpr& e);
    std::string emitCall(const CallExpr& e);
    std::string emitMember(const MemberExpr& e);
    std::string emitIndex(const IndexExpr& e);
    std::string emitLambda(const LambdaExpr& e);
    std::string emitNew(const NewExpr& e);
    std::string emitPipe(const PipeExpr& e);

    // ── annotation emitters ───────────────────────────────────────────────────
    void emitFuncAnnotations(const std::vector<std::string>& annots);
    void emitVarAnnotations(const std::vector<std::string>& annots, std::string& prefix);

    // ── built-in mapping ──────────────────────────────────────────────────────
    std::string mapBuiltinCall(const std::string& name, const ExprList& args);
    std::string mapBuiltinNamespace(const std::string& ns, const std::string& fn,
                                    const ExprList& args);
};

} // namespace drv
