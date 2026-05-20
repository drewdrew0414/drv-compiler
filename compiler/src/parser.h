#pragma once
#include "ast.h"
#include "token.h"
#include <vector>
#include <string>

namespace drv {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens, std::string filename = "<stdin>");

    Program parse();

    const std::vector<std::string>& errors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    std::vector<Token> toks_;
    std::string        file_;
    size_t             pos_{0};
    std::vector<std::string> errors_;
    // Positions where GtGt was mutated to Gt for nested generic splitting.
    // restorePos() undoes mutations at positions >= the saved pos so probes
    // that backtrack leave the token stream in its original state.
    std::vector<size_t> gt_splits_;
    int  expr_depth_{0};  // recursion depth counter
    bool panic_{false};   // set on fatal parse error; makes all expr parsers short-circuit
    // Each parseExpr() call recurses through ~16 intermediate parse functions
    // (parsePipe → … → parsePrimary) before calling parseExpr() again.
    // Each such chain consumes ~2 KiB of stack.
    // 100 × 16 × 2 KiB = 3.2 MiB — well within the 8 MiB default stack.
    // Once the limit fires, panic_ is set so every subsequent parseExpr()
    // returns nullptr immediately without touching the token stream, ensuring
    // clean unwinding without null-deref or additional recursion.
    static constexpr int kMaxExprDepth = 100;

    // ── Annotation scope stack ────────────────────────────────────────────────
    // Each entry holds annots active at that block depth.
    // Pushed on '{', popped on '}'.  Parser attaches active scope to nodes.
    struct AnnotScopeFrame {
        std::vector<std::string> annots; // annotations declared on this scope
        int depth{0};
    };
    std::vector<AnnotScopeFrame> annot_scope_stack_;
    int scope_depth_{0};

    void pushAnnotScope(std::vector<std::string> a) {
        annot_scope_stack_.push_back({std::move(a), scope_depth_});
    }
    void popAnnotScope() {
        while (!annot_scope_stack_.empty() &&
               annot_scope_stack_.back().depth >= scope_depth_)
            annot_scope_stack_.pop_back();
    }
    // Returns all active annotations from enclosing scopes
    std::vector<std::string> activeAnnotations() const {
        std::vector<std::string> result;
        for (auto& f : annot_scope_stack_)
            for (auto& a : f.annots) result.push_back(a);
        return result;
    }

    // ── Token navigation ──────────────────────────────────────────────────────
    const Token& peek(int offset = 0) const noexcept;
    const Token& current()            const noexcept { return peek(); }
    const Token& advance()            noexcept;
    bool         check(TK k)          const noexcept { return peek().kind == k; }
    bool         match(TK k)          noexcept;
    bool         match2(TK a, TK b)   noexcept;
    const Token& expect(TK k, const std::string& msg);
    void         skipSemi();
    void         sync();   // error recovery: skip to next ;
    // Restore pos and undo any GtGt→Gt mutations made at positions >= saved.
    void         restorePos(size_t saved);

    void error(const Token& t, const std::string& msg);
    void error(const std::string& msg) { error(peek(), msg); }

    // ── Annotation collection ─────────────────────────────────────────────────
    std::vector<std::string> parseAnnotations();

    // ── Top-level ─────────────────────────────────────────────────────────────
    StmtPtr parseTopLevel();
    StmtPtr parseModuleDecl();
    StmtPtr parseUseDecl();
    StmtPtr parseExternDecl();
    StmtPtr parseDimDecl();
    StmtPtr parseClassDecl(std::vector<std::string> modifiers);
    StmtPtr parseTraitDecl();
    StmtPtr parseImplDecl();
    StmtPtr parseFuncDecl(std::vector<std::string> modifiers, bool is_async = false);

    // ── Statements ────────────────────────────────────────────────────────────
    StmtPtr parseStmt();
    StmtPtr parseVarDecl(std::vector<std::string> modifiers = {});
    StmtPtr parseIfStmt();
    StmtPtr parseForStmt(bool is_simd, bool is_parallel, std::vector<std::string> reductions);
    StmtPtr parseWhileStmt();
    StmtPtr parseSwitchStmt();
    StmtPtr parseMatchStmt();
    StmtPtr parseTryCatchStmt();
    StmtPtr parseRegionStmt();
    StmtPtr parseSpawnStmt();
    StmtPtr parseStaticIfStmt();
    StmtPtr parseExistsStmt();
    StmtList parseBlock(std::vector<std::string> scope_annots = {});  // { stmts }

    // ── Type parsing ──────────────────────────────────────────────────────────
    TypeRef parseTypeRef();
    Param   parseParam();
    std::vector<Param> parseParamList();

    // ── Expressions ───────────────────────────────────────────────────────────
    ExprPtr parseExpr();
    ExprPtr parsePipe();
    ExprPtr parseAssign();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseRelational();
    ExprPtr parseBitOr();
    ExprPtr parseBitXor();
    ExprPtr parseBitAnd();
    ExprPtr parseShift();
    ExprPtr parseAddSub();
    ExprPtr parseMulDiv();
    ExprPtr parsePower();
    ExprPtr parseUnary();
    ExprPtr parsePostfix();
    ExprPtr parsePrimary();
    ExprPtr parseLambda();
    ExprList parseArgList();
};

} // namespace drv
