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
    StmtList parseBlock();  // { stmts }

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
