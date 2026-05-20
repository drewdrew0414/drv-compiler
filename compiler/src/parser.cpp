#include "parser.h"
#include "lexer.h"
#include <stdexcept>
#include <sstream>
#include <cassert>

namespace drv {

static const Token EOF_TOK{TK::Eof, "", 0, 0};

Parser::Parser(std::vector<Token> tokens, std::string filename)
    : toks_(std::move(tokens)), file_(std::move(filename)) {}

// ── navigation ───────────────────────────────────────────────────────────────
const Token& Parser::peek(int offset) const noexcept {
    size_t idx = pos_ + offset;
    return idx < toks_.size() ? toks_[idx] : EOF_TOK;
}

const Token& Parser::advance() noexcept {
    if (pos_ < toks_.size()) return toks_[pos_++];
    return EOF_TOK;
}

bool Parser::match(TK k) noexcept {
    if (check(k)) { advance(); return true; }
    return false;
}

bool Parser::match2(TK a, TK b) noexcept {
    if (check(a) && peek(1).kind == b) { advance(); advance(); return true; }
    return false;
}

const Token& Parser::expect(TK k, const std::string& msg) {
    if (!check(k)) {
        // Show the actual token value; for EOF use a readable placeholder.
        std::string got = peek().kind == TK::Eof
            ? "<end of file>"
            : (peek().value.empty() ? std::string(tk_name(peek().kind)) : peek().value);
        error(msg + " (got '" + got + "')");
    }
    return advance();
}

void Parser::skipSemi() {
    while (check(TK::Semi)) advance();
}

// Convert a TypeRef back to its source-language string (e.g. "BST_Node<T>")
static std::string typeRefToStr(const TypeRef& tr) {
    std::string s = tr.name;
    if (!tr.args.empty()) {
        s += "<";
        for (size_t i = 0; i < tr.args.size(); ++i) {
            if (i) s += ", ";
            s += typeRefToStr(tr.args[i]);
        }
        s += ">";
    }
    if (tr.is_array) s += "[]";
    return s;
}

void Parser::restorePos(size_t saved) {
    // Undo GtGt→Gt mutations made at positions >= saved, then restore pos.
    while (!gt_splits_.empty() && gt_splits_.back() >= saved) {
        size_t p = gt_splits_.back(); gt_splits_.pop_back();
        toks_[p].kind  = TK::GtGt;
        toks_[p].value = ">>";
    }
    pos_ = saved;
}

void Parser::error(const Token& t, const std::string& msg) {
    errors_.push_back(file_ + ":" + std::to_string(t.line) + ":" +
                      std::to_string(t.col) + ": error: " + msg);
}

void Parser::sync() {
    while (!check(TK::Eof) && !check(TK::Semi) && !check(TK::RBrace))
        advance();
    if (check(TK::Semi)) advance();
}

// ── Scope annotations that apply to an entire block/function ─────────────────
// These are pushed onto annot_scope_stack_ rather than attached to a single node
static bool isScopeAnnotation(const std::string& name) {
    return name == "@fast_math"  || name == "@strict_math" ||
           name == "@noalias"    || name == "@threadsafe"  ||
           name == "@pure"       || name.rfind("@bench",0)==0 ||
           name.rfind("@trace",0)==0;
}

// ── annotations ──────────────────────────────────────────────────────────────
std::vector<std::string> Parser::parseAnnotations() {
    std::vector<std::string> annots;
    while (check(TK::At)) {
        std::string name = advance().value;  // e.g. "@align"
        // optional argument list: @align(64)
        if (check(TK::LParen)) {
            name += "(";
            advance();
            int depth = 1;
            while (!check(TK::Eof) && depth > 0) {
                if (check(TK::LParen)) ++depth;
                else if (check(TK::RParen)) { if (--depth == 0) break; }
                name += advance().value;
                if (depth > 0 && check(TK::Comma)) { name += ","; advance(); }
            }
            if (check(TK::RParen)) { advance(); name += ")"; }
        }
        annots.push_back(name);
    }
    return annots;
}

// ── type parsing ─────────────────────────────────────────────────────────────
TypeRef Parser::parseTypeRef() {
    TypeRef tr;
    // mut prefix
    if (match(TK::KwMut)) tr.is_mut = true;

    if (check(TK::Ident) || peek().kind >= TK::KwInt) {
        tr.name = advance().value;
    } else {
        error("expected type name");
        tr.name = "void";
    }

    // generic args: Foo<T, U>
    if (check(TK::Lt)) {
        advance();
        while (!check(TK::Gt) && !check(TK::GtGt) && !check(TK::Eof)) {
            tr.args.push_back(parseTypeRef());
            if (!match(TK::Comma)) break;
        }
        if (check(TK::GtGt)) {
            // Split '>>' for nested generic: Borrow<list<int>>.
            // Record in gt_splits_ so restorePos() can undo on backtrack.
            gt_splits_.push_back(pos_);
            toks_[pos_].kind  = TK::Gt;
            toks_[pos_].value = ">";
            // Do NOT advance — leave this Gt for the outer parseTypeRef.
        } else {
            expect(TK::Gt, "expected '>' after generic type args");
        }
    }

    // array suffix: int[]
    if (check(TK::LBracket) && peek(1).kind == TK::RBracket) {
        advance(); advance();
        tr.is_array = true;
    }

    return tr;
}

Param Parser::parseParam() {
    Param p;
    p.type = parseTypeRef();
    if (check(TK::Ident)) {
        p.name = advance().value;
    }
    if (match(TK::Eq)) {
        p.default_val = parseExpr();
    }
    return p;
}

std::vector<Param> Parser::parseParamList() {
    std::vector<Param> params;
    expect(TK::LParen, "expected '('");
    while (!check(TK::RParen) && !check(TK::Eof)) {
        params.push_back(parseParam());
        if (!match(TK::Comma)) break;
    }
    expect(TK::RParen, "expected ')'");
    return params;
}

// ── block ────────────────────────────────────────────────────────────────────
StmtList Parser::parseBlock(std::vector<std::string> scope_annots) {
    expect(TK::LBrace, "expected '{'");
    ++scope_depth_;
    // Push scope-level annotations (@fast_math, @noalias, etc.)
    if (!scope_annots.empty()) {
        std::vector<std::string> scope_only;
        for (auto& a : scope_annots)
            if (isScopeAnnotation(a)) scope_only.push_back(a);
        if (!scope_only.empty()) pushAnnotScope(std::move(scope_only));
    }
    StmtList stmts;
    while (true) {
        // Skip bare semicolons before re-evaluating the loop condition.
        // Without this, a trailing ';' after '};' lands parseStmt() on '}',
        // which causes an expression-parse failure instead of a clean exit.
        skipSemi();
        if (check(TK::RBrace) || check(TK::Eof)) break;
        try {
            stmts.push_back(parseStmt());
        } catch (...) { sync(); }
    }
    popAnnotScope();
    --scope_depth_;
    expect(TK::RBrace, "expected '}'");
    return stmts;
}

// ── top level ────────────────────────────────────────────────────────────────
Program Parser::parse() {
    Program prog;
    while (!check(TK::Eof)) {
        skipSemi();
        if (check(TK::Eof)) break;
        try {
            prog.stmts.push_back(parseTopLevel());
        } catch (...) { sync(); }
        skipSemi();
    }
    return prog;
}

StmtPtr Parser::parseTopLevel() {
    // @region must be detected before parseAnnotations() consumes @region token
    if (check(TK::At) && peek().value == "@region") {
        advance();
        return parseRegionStmt();
    }
    auto annots = parseAnnotations();

    // module / use
    if (check(TK::KwModule)) return parseModuleDecl();
    if (check(TK::KwUse))    return parseUseDecl();
    if (check(TK::KwExtern)) return parseExternDecl();
    if (check(TK::KwDimKw))  return parseDimDecl();
    if (check(TK::KwStaticIf)) return parseStaticIfStmt();
    if (check(TK::KwCompileEval)) {
        advance(); // consume compile_eval
        std::vector<std::string> ce_mods = {"compile_eval"};
        auto fn = parseFuncDecl(ce_mods, false);
        fn->annotations = annots;
        return fn;
    }

    // collect modifiers
    std::vector<std::string> mods;
    while (true) {
        TK k = peek().kind;
        if (k==TK::KwPublic||k==TK::KwPrivate||k==TK::KwStatic||
            k==TK::KwAbstract||k==TK::KwFinal||k==TK::KwSynchronized||
            k==TK::KwConst||k==TK::KwUnsigned||k==TK::KwMut) {
            mods.push_back(advance().value);
        } else break;
    }

    if (check(TK::KwClass))   return parseClassDecl(mods);
    if (check(TK::KwTrait))   return parseTraitDecl();
    if (check(TK::KwImpl))    return parseImplDecl();
    if (check(TK::KwAsync)) {
        advance();
        auto fn = parseFuncDecl(mods, true);
        fn->annotations = annots;
        return fn;
    }

    // function: RetType name<T>(params) { }
    // or variable: RetType name = ...;
    // heuristic: if next token after type is ident and then '(', it's a function
    auto stmt = parseStmt();
    stmt->annotations = annots;
    return stmt;
}

StmtPtr Parser::parseModuleDecl() {
    advance(); // module
    auto n = std::make_unique<ModuleDecl>();
    n->line = peek().line;
    n->name = expect(TK::Ident, "expected module name").value;
    expect(TK::Semi, "expected ';' after module declaration");
    return n;
}

StmtPtr Parser::parseUseDecl() {
    advance(); // use
    auto n = std::make_unique<UseDecl>();
    n->line = peek().line;
    n->module = expect(TK::Ident, "expected module name").value;
    while (check(TK::ColonColon)) {
        advance();
        n->symbol = expect(TK::Ident, "expected symbol name").value;
    }
    expect(TK::Semi, "expected ';'");
    return n;
}

StmtPtr Parser::parseExternDecl() {
    advance(); // extern
    auto n = std::make_unique<ExternDecl>();
    n->line = peek().line;
    if (check(TK::Ident) && peek().value == "unsafe_legacy") {
        n->unsafe_legacy = true; advance();
    }
    if (check(TK::LitString)) {
        n->abi = advance().value;
    }
    n->declarations = parseBlock();
    return n;
}

StmtPtr Parser::parseDimDecl() {
    advance(); // dim
    auto n = std::make_unique<DimDecl>();
    n->line = peek().line;
    n->name = expect(TK::Ident, "expected dimension name").value;
    expect(TK::Eq, "expected '='");
    n->symbol = expect(TK::LitString, "expected unit symbol string").value;
    expect(TK::Semi, "expected ';'");
    return n;
}

StmtPtr Parser::parseClassDecl(std::vector<std::string> mods) {
    advance(); // class
    auto n = std::make_unique<ClassDecl>();
    n->line = peek().line;
    n->modifiers = mods;
    n->name = expect(TK::Ident, "expected class name").value;
    // Generic type parameters: class Foo<T implements Bar, U> { ... }
    if (check(TK::Lt)) {
        advance();
        while (!check(TK::Gt) && !check(TK::GtGt) && !check(TK::Eof)) {
            n->type_params.push_back(advance().value);
            if (check(TK::KwIn) || (check(TK::Ident) && peek().value == "implements")) {
                advance();
                n->trait_bounds.push_back(advance().value);
                while (check(TK::Plus)) { advance(); n->trait_bounds.push_back(advance().value); }
            }
            if (!match(TK::Comma)) break;
        }
        if (check(TK::GtGt)) {
            gt_splits_.push_back(pos_);
            toks_[pos_].kind = TK::Gt; toks_[pos_].value = ">";
        } else {
            expect(TK::Gt, "expected '>'");
        }
    }
    if (match(TK::KwExtends)) {
        n->base = expect(TK::Ident, "expected base class name").value;
    }
    expect(TK::LBrace, "expected '{'");
    while (!check(TK::RBrace) && !check(TK::Eof)) {
        auto annots = parseAnnotations();
        // Don't collect modifiers here — let parseStmt() handle them
        // so they end up in FuncDecl::modifiers / VarDecl properly
        auto stmt = parseStmt();
        stmt->annotations = annots;
        // classify: FuncDecl → method, VarDecl → field (we store all as methods)
        n->methods.push_back(std::move(stmt));
        skipSemi();
    }
    expect(TK::RBrace, "expected '}'");
    return n;
}

StmtPtr Parser::parseTraitDecl() {
    advance(); // trait
    auto n = std::make_unique<TraitDecl>();
    n->line = peek().line;
    n->name = expect(TK::Ident, "expected trait name").value;
    n->body = parseBlock();
    return n;
}

StmtPtr Parser::parseImplDecl() {
    advance(); // impl
    auto n = std::make_unique<ImplDecl>();
    n->line = peek().line;
    n->trait_name = expect(TK::Ident, "expected trait name").value;
    if (check(TK::KwFor)) { advance(); }
    n->class_name = expect(TK::Ident, "expected class name").value;
    n->methods = parseBlock();
    return n;
}

StmtPtr Parser::parseFuncDecl(std::vector<std::string> mods, bool is_async) {
    auto n = std::make_unique<FuncDecl>();
    n->line = peek().line;
    n->modifiers = mods;
    n->is_async = is_async;
    n->ret_type  = parseTypeRef();

    // name and optional generic type params
    n->name = expect(TK::Ident, "expected function name").value;
    if (check(TK::Lt)) {
        advance();
        while (!check(TK::Gt) && !check(TK::Eof)) {
            n->type_params.push_back(advance().value);
            if (check(TK::KwIn) || (check(TK::Ident) && peek().value == "implements")) {
                advance();
                // collect trait bounds
                n->trait_bounds.push_back(advance().value);
                while (check(TK::Plus)) { advance(); n->trait_bounds.push_back(advance().value); }
            }
            if (!match(TK::Comma)) break;
        }
        expect(TK::Gt, "expected '>'");
    }

    n->params = parseParamList();
    if (check(TK::LBrace)) {
        n->body = parseBlock();
    } else {
        // abstract method (trait signature) — no body
        expect(TK::Semi, "expected ';' or '{' after function signature");
    }
    return n;
}

// ── statements ───────────────────────────────────────────────────────────────
StmtPtr Parser::parseStmt() {
    skipSemi();

    // @region name { ... } — must check BEFORE parseAnnotations() consumes @region
    if (check(TK::At) && peek().value == "@region") {
        advance(); // consume @region
        return parseRegionStmt();
    }

    auto annots = parseAnnotations();

    TK k = peek().kind;

    if (k == TK::KwIf)        { auto s = parseIfStmt();       s->annotations = annots; return s; }
    if (k == TK::KwWhile)     { auto s = parseWhileStmt();    s->annotations = annots; return s; }
    if (k == TK::KwWhere) {
        // where (cond) { ... } otherwise { ... }
        advance();
        auto n = std::make_unique<IfStmt>(); n->line = peek().line;
        expect(TK::LParen, "expected '('");
        n->cond = parseExpr();
        expect(TK::RParen, "expected ')'");
        n->then_body = parseBlock();
        if (check(TK::KwOtherwise)) { advance(); n->else_body = parseBlock(); }
        n->annotations = annots;
        return n;
    }
    if (k == TK::KwSwitch)    { auto s = parseSwitchStmt();   s->annotations = annots; return s; }
    if (k == TK::KwMatch)     { auto s = parseMatchStmt();    s->annotations = annots; return s; }
    if (k == TK::KwTry)       { auto s = parseTryCatchStmt(); s->annotations = annots; return s; }
    if (k == TK::KwSpawn)     { auto s = parseSpawnStmt();    s->annotations = annots; return s; }
    if (k == TK::KwStaticIf)  { auto s = parseStaticIfStmt(); s->annotations = annots; return s; }
    if (k == TK::KwExists)    { auto s = parseExistsStmt();   s->annotations = annots; return s; }
    // @region handled above before parseAnnotations()

    if (k == TK::KwReturn) {
        advance();
        auto s = std::make_unique<ReturnStmt>();
        s->annotations = annots;
        if (!check(TK::Semi)) s->value = parseExpr();
        expect(TK::Semi, "expected ';' after return");
        return s;
    }
    if (k == TK::KwBreak)    { advance(); auto s = std::make_unique<BreakStmt>();    s->annotations=annots; expect(TK::Semi,"expected ';'"); return s; }
    if (k == TK::KwContinue) { advance(); auto s = std::make_unique<ContinueStmt>(); s->annotations=annots; expect(TK::Semi,"expected ';'"); return s; }
    if (k == TK::KwThrow)    { advance(); auto s = std::make_unique<ThrowStmt>();   s->annotations=annots; s->value=parseExpr(); expect(TK::Semi,"expected ';'"); return s; }
    if (k == TK::KwPass)     { advance(); auto s = std::make_unique<PassStmt>();    s->annotations=annots; expect(TK::Semi,"expected ';'"); return s; }

    // parallel / simd for
    bool is_parallel = false, is_simd = false;
    std::vector<std::string> reductions;
    if (k == TK::KwParallel) { is_parallel = true; advance(); k = peek().kind; }
    if (k == TK::KwSimd)     { is_simd = true;     advance(); k = peek().kind; }
    if (k == TK::KwFor || (is_parallel || is_simd)) {
        if (k != TK::KwFor) { error("expected 'for'"); }
        else advance();
        // collect reductions
        while (check(TK::KwReduction) || (check(TK::Ident) && peek().value == "reduction")) {
            advance();
            expect(TK::LParen, "expected '(' after reduction");
            std::string spec;
            spec += advance().value; // operator
            expect(TK::Colon, "expected ':'");
            spec += ":" + advance().value; // variable
            expect(TK::RParen, "expected ')'");
            reductions.push_back(spec);
        }
        return parseForStmt(is_simd, is_parallel, reductions);
    }

    // lazy var ...
    bool is_lazy = false;
    if (k == TK::KwLazy) { is_lazy = true; advance(); k = peek().kind; }

    // collect modifiers for var/func
    std::vector<std::string> mods;
    while (true) {
        TK mk = peek().kind;
        if (mk==TK::KwPublic||mk==TK::KwPrivate||mk==TK::KwStatic||
            mk==TK::KwConst||mk==TK::KwUnsigned||mk==TK::KwMut||
            mk==TK::KwAbstract||mk==TK::KwFinal||mk==TK::KwOverride) {
            mods.push_back(advance().value);
        } else break;
    }

    bool is_async = false;
    if (check(TK::KwAsync)) { is_async = true; advance(); }

    // lookahead: is this a declaration (type + ident) or expression?
    // heuristic: if current token looks like a type and next is ident
    bool looksLikeDecl = false;
    {
        TK t0 = peek().kind;
        bool typeStart = (t0 >= TK::KwInt && t0 <= TK::KwDim) || t0 == TK::Ident || t0 == TK::KwVar || t0 == TK::KwRef;
        if (typeStart) {
            // save pos and try
            size_t saved = pos_;
            try {
                TypeRef tr = parseTypeRef();
                if (check(TK::Ident)) { looksLikeDecl = true; }
            } catch (...) {}
            restorePos(saved);
        }
    }

    if (looksLikeDecl) {
        // function or variable
        size_t saved = pos_;
        TypeRef tr = parseTypeRef();
        if (check(TK::Ident)) {
            std::string name = peek(1).kind == TK::LParen || peek(1).kind == TK::Lt
                ? "" : peek().value;
            bool isFunc = (peek(1).kind == TK::LParen || peek(1).kind == TK::Lt);
            if (isFunc) {
                restorePos(saved);
                auto fn = parseFuncDecl(mods, is_async);
                fn->annotations = annots;
                if (is_lazy) static_cast<FuncDecl*>(fn.get())->modifiers.push_back("lazy");
                return fn;
            } else {
                // variable decl
                auto vd = std::make_unique<VarDecl>();
                vd->line = peek().line;
                vd->annotations = annots;
                vd->type = tr;
                vd->name = advance().value;
                if (match(TK::Eq)) vd->init = parseExpr();
                expect(TK::Semi, "expected ';' after variable declaration");
                return vd;
            }
        }
        restorePos(saved);
    }

    // expression statement
    auto es = std::make_unique<ExprStmt>();
    es->annotations = annots;
    es->line = peek().line;
    es->expr = parseExpr();
    if (!check(TK::RBrace) && !check(TK::Eof))
        expect(TK::Semi, "expected ';' after expression");
    return es;
}

StmtPtr Parser::parseIfStmt() {
    advance(); // if
    auto n = std::make_unique<IfStmt>();
    n->line = peek().line;
    expect(TK::LParen, "expected '('");
    n->cond = parseExpr();
    expect(TK::RParen, "expected ')'");
    n->then_body = parseBlock();
    if (check(TK::KwElse)) {
        advance();
        if (check(TK::KwIf)) {
            StmtList nested;
            nested.push_back(parseIfStmt());
            n->else_body = std::move(nested);
        } else {
            n->else_body = parseBlock();
        }
    }
    return n;
}

StmtPtr Parser::parseForStmt(bool is_simd, bool is_parallel, std::vector<std::string> reds) {
    // for (var in/of expr) { }
    expect(TK::LParen, "expected '('");
    std::string var = expect(TK::Ident, "expected loop variable").value;

    if (check(TK::KwIn)) {
        advance();
        // range: expr..expr  or  expr
        auto from = parseExpr();
        if (check(TK::DotDot)) {
            advance();
            auto to = parseExpr();
            // Optional step: for (i in 0..N step TILE)
            ExprPtr step_expr;
            if (check(TK::Ident) && peek().value == "step") {
                advance(); // consume "step"
                step_expr = parseExpr();
            }
            expect(TK::RParen, "expected ')'");
            auto n = std::make_unique<ForRangeStmt>();
            n->line = from->line;
            n->var = var; n->from = std::move(from); n->to = std::move(to);
            n->step = std::move(step_expr);
            n->is_simd = is_simd; n->is_parallel = is_parallel;
            n->reductions = reds;
            n->body = parseBlock();
            return n;
        }
        // plain range with just from (error case)
        expect(TK::RParen, "expected ')'");
        auto n = std::make_unique<ForRangeStmt>();
        n->var = var; n->from = std::move(from); n->is_simd = is_simd; n->is_parallel = is_parallel;
        n->body = parseBlock();
        return n;
    }

    if (check(TK::KwOf)) {
        advance();
        auto coll = parseExpr();
        expect(TK::RParen, "expected ')'");
        auto n = std::make_unique<ForEachStmt>();
        n->line = coll->line;
        n->var = var; n->collection = std::move(coll);
        n->is_simd = is_simd; n->is_parallel = is_parallel;
        n->reductions = reds;
        n->body = parseBlock();
        return n;
    }

    // C-style: (TypeRef var = init; cond; step)
    // already consumed var as ident — parse C-style init
    // re-interpret: the ident was the type name
    // Build a simple ExprStmt for the init
    // This is a fallback — real C-style parsing needs more lookahead
    auto n = std::make_unique<ForCStyleStmt>();
    n->line = peek().line;
    // init expr
    auto initExpr = std::make_unique<ExprStmt>();
    initExpr->expr = parseExpr();
    n->init = std::move(initExpr);
    expect(TK::Semi, "expected ';'");
    n->cond = parseExpr();
    expect(TK::Semi, "expected ';'");
    n->step = parseExpr();
    expect(TK::RParen, "expected ')'");
    n->body = parseBlock();
    return n;
}

StmtPtr Parser::parseWhileStmt() {
    advance();
    auto n = std::make_unique<WhileStmt>();
    n->line = peek().line;
    expect(TK::LParen, "expected '('");
    n->cond = parseExpr();
    expect(TK::RParen, "expected ')'");
    n->body = parseBlock();
    return n;
}

StmtPtr Parser::parseSwitchStmt() {
    advance();
    auto n = std::make_unique<SwitchStmt>();
    n->line = peek().line;
    expect(TK::LParen, "expected '('");
    n->expr = parseExpr();
    expect(TK::RParen, "expected ')'");
    expect(TK::LBrace, "expected '{'");
    while (!check(TK::RBrace) && !check(TK::Eof)) {
        CaseArm arm;
        if (check(TK::KwCase)) {
            advance();
            if (check(TK::KwDefault)) { advance(); arm.is_default = true; }
            else { expect(TK::LParen,"expected '('"); arm.value = parseExpr(); expect(TK::RParen,"expected ')'"); }
        } else if (check(TK::KwDefault)) {
            advance(); arm.is_default = true;
        } else break;
        expect(TK::Arrow, "expected '->'");
        arm.body = parseBlock();
        expect(TK::Semi, "expected ';'");
        n->arms.push_back(std::move(arm));
    }
    expect(TK::RBrace, "expected '}'");
    return n;
}

StmtPtr Parser::parseMatchStmt() {
    advance();
    auto n = std::make_unique<MatchStmt>();
    n->line = peek().line;
    n->expr = parseExpr();
    expect(TK::LBrace, "expected '{'");
    while (!check(TK::RBrace) && !check(TK::Eof)) {
        MatchArm arm;
        arm.pattern = advance().value;
        if (check(TK::LParen)) {
            advance();
            arm.bind_var = advance().value;
            expect(TK::RParen, "expected ')'");
        }
        expect(TK::FatArrow, "expected '=>'");
        arm.body = parseBlock();
        expect(TK::Semi, "expected ';'");
        n->arms.push_back(std::move(arm));
    }
    expect(TK::RBrace, "expected '}'");
    return n;
}

StmtPtr Parser::parseTryCatchStmt() {
    advance();
    auto n = std::make_unique<TryCatchStmt>();
    n->line = peek().line;
    n->try_body = parseBlock();
    if (check(TK::KwCatch)) {
        advance();
        expect(TK::LParen, "expected '('");
        n->catch_var = expect(TK::Ident, "expected catch variable").value;
        expect(TK::RParen, "expected ')'");
        n->catch_body = parseBlock();
    }
    return n;
}

StmtPtr Parser::parseRegionStmt() {
    auto n = std::make_unique<RegionStmt>();
    n->line = peek().line;
    n->name = expect(TK::Ident, "expected region name").value;
    n->body = parseBlock();
    return n;
}

StmtPtr Parser::parseSpawnStmt() {
    advance();
    auto n = std::make_unique<SpawnStmt>();
    n->line = peek().line;
    n->body = parseBlock();
    return n;
}

StmtPtr Parser::parseStaticIfStmt() {
    advance();
    auto n = std::make_unique<StaticIfStmt>();
    n->line = peek().line;
    expect(TK::LParen, "expected '('");
    n->cond = parseExpr();
    expect(TK::RParen, "expected ')'");
    n->then_body = parseBlock();
    if (check(TK::KwElse)) {
        advance();
        n->else_body = parseBlock();
    }
    return n;
}

StmtPtr Parser::parseExistsStmt() {
    advance();
    auto n = std::make_unique<ExistsStmt>();
    n->line = peek().line;
    expect(TK::LParen, "expected '('");
    n->var = expect(TK::Ident, "expected variable name").value;
    expect(TK::RParen, "expected ')'");
    n->then_body = parseBlock();
    if (check(TK::KwElse)) { advance(); n->else_body = parseBlock(); }
    return n;
}

// ── expression parsing (recursive descent) ───────────────────────────────────
ExprPtr Parser::parseExpr() {
    // Short-circuit on panic so every recursive caller returns nullptr
    // immediately, enabling clean stack unwinding without null-derefs.
    if (panic_) return nullptr;

    if (++expr_depth_ > kMaxExprDepth) {
        --expr_depth_;
        // Set panic_ so all callers above us also return nullptr without
        // attempting any further parsing of the malformed token stream.
        panic_ = true;
        error("expression nested too deeply (limit " +
              std::to_string(kMaxExprDepth) + ")");
        return nullptr;
    }
    auto result = parsePipe();
    --expr_depth_;
    return result;
}

ExprPtr Parser::parsePipe() {
    auto lhs = parseAssign();
    while (check(TK::PipeGt)) {
        advance();
        auto rhs = parseAssign();
        auto n = std::make_unique<PipeExpr>();
        n->line = lhs->line;
        n->left = std::move(lhs);
        n->right = std::move(rhs);
        lhs = std::move(n);
    }
    return lhs;
}

ExprPtr Parser::parseAssign() {
    auto lhs = parseOr();
    static const std::vector<TK> ASSIGN_OPS = {TK::Eq,TK::PlusEq,TK::MinusEq,TK::StarEq,TK::SlashEq,TK::PercentEq};
    for (TK op : ASSIGN_OPS) {
        if (check(op)) {
            std::string opStr = advance().value;
            auto rhs = parseAssign();
            auto n = std::make_unique<BinaryExpr>();
            n->line = lhs->line; n->op = opStr;
            n->left = std::move(lhs); n->right = std::move(rhs);
            return n;
        }
    }
    return lhs;
}

#define BINARY_LEVEL(NAME, NEXT, ...)                                    \
ExprPtr Parser::NAME() {                                                 \
    auto lhs = NEXT();                                                   \
    static const std::vector<TK> OPS = {__VA_ARGS__};                  \
    for (bool going=true; going;) {                                      \
        going = false;                                                   \
        for (TK op : OPS) {                                              \
            if (check(op)) {                                             \
                std::string opStr = advance().value;                     \
                auto rhs = NEXT();                                       \
                auto n = std::make_unique<BinaryExpr>();                 \
                n->line = lhs->line; n->op = opStr;                     \
                n->left = std::move(lhs); n->right = std::move(rhs);    \
                lhs = std::move(n); going = true; break;                 \
            }                                                            \
        }                                                                \
    }                                                                    \
    return lhs;                                                          \
}

BINARY_LEVEL(parseOr,         parseAnd,       TK::KwOr, TK::PipePipe)
BINARY_LEVEL(parseAnd,        parseEquality,  TK::KwAnd, TK::AmpAmp)
BINARY_LEVEL(parseEquality,   parseRelational,TK::EqEq, TK::BangEq)
BINARY_LEVEL(parseRelational, parseBitOr,     TK::Lt, TK::Gt, TK::LtEq, TK::GtEq,
                                               TK::KwIn, TK::KwIs)
BINARY_LEVEL(parseBitOr,      parseBitXor,    TK::Pipe)
BINARY_LEVEL(parseBitXor,     parseBitAnd,    TK::Caret)
BINARY_LEVEL(parseBitAnd,     parseShift,     TK::Amp)
BINARY_LEVEL(parseShift,      parseAddSub,    TK::LtLt, TK::GtGt)
BINARY_LEVEL(parseAddSub,     parseMulDiv,    TK::Plus, TK::Minus)
BINARY_LEVEL(parseMulDiv,     parsePower,     TK::Star, TK::Slash, TK::Percent, TK::SlashSlash)

ExprPtr Parser::parsePower() {
    auto lhs = parseUnary();
    if (check(TK::StarStar)) {
        std::string op = advance().value;
        auto rhs = parsePower(); // right-assoc
        auto n = std::make_unique<BinaryExpr>();
        n->line = lhs->line; n->op = op;
        n->left = std::move(lhs); n->right = std::move(rhs);
        return n;
    }
    return lhs;
}

ExprPtr Parser::parseUnary() {
    if (check(TK::Minus)||check(TK::Bang)||check(TK::KwNot)||check(TK::Tilde)) {
        auto n = std::make_unique<UnaryExpr>();
        n->line = peek().line;
        n->op = advance().value;
        n->operand = parseUnary();
        return n;
    }
    if (check(TK::KwNot)) {
        auto n = std::make_unique<UnaryExpr>();
        n->line = peek().line; n->op = "not"; advance();
        n->operand = parseUnary(); return n;
    }
    if (check(TK::KwMove)) {
        advance();
        auto n = std::make_unique<MoveExpr>();
        n->line = peek().line;
        n->operand = parseUnary();
        return n;
    }
    return parsePostfix();
}

ExprPtr Parser::parsePostfix() {
    auto expr = parsePrimary();
    if (!expr) return nullptr;  // propagate panic / error without null-deref
    while (true) {
        if (check(TK::Dot)) {
            advance();
            auto n = std::make_unique<MemberExpr>();
            n->line = expr->line;
            n->object = std::move(expr);
            n->field = advance().value;
            if (check(TK::LParen)) {
                // method call → CallExpr(MemberExpr)
                auto call = std::make_unique<CallExpr>();
                call->line = n->line;
                // wrap member into callee
                call->callee = std::move(n);
                advance(); // (
                call->args = parseArgList();
                expr = std::move(call);
            } else {
                expr = std::move(n);
            }
        } else if (check(TK::LParen)) {
            advance();
            auto call = std::make_unique<CallExpr>();
            call->line = expr->line;
            call->callee = std::move(expr);
            call->args = parseArgList();
            expr = std::move(call);
        } else if (check(TK::LBracket)) {
            advance();
            auto idx = std::make_unique<IndexExpr>();
            idx->line = expr->line;
            idx->object = std::move(expr);
            idx->index = parseExpr();
            if (check(TK::Comma)) { advance(); idx->index2 = parseExpr(); }
            expect(TK::RBracket, "expected ']'");
            expr = std::move(idx);
        } else if (check(TK::KwAs)) {
            advance();
            auto n = std::make_unique<BinaryExpr>();
            n->line = expr->line; n->op = "as";
            n->left = std::move(expr);
            // type as rhs: store name in a string literal
            auto tr = parseTypeRef();
            auto tname = std::make_unique<StringLit>();
            tname->value = tr.name + (tr.is_array ? "[]" : "");
            n->right = std::move(tname);
            expr = std::move(n);
        } else if (check(TK::PlusPlus)||check(TK::MinusMinus)) {
            auto n = std::make_unique<UnaryExpr>();
            n->line = expr->line; n->op = advance().value; n->prefix = false;
            n->operand = std::move(expr);
            expr = std::move(n);
        } else {
            break;
        }
    }
    return expr;
}

ExprList Parser::parseArgList() {
    ExprList args;
    while (!check(TK::RParen) && !check(TK::Eof)) {
        args.push_back(parseExpr());
        if (!match(TK::Comma)) break;
    }
    expect(TK::RParen, "expected ')'");
    return args;
}

ExprPtr Parser::parseLambda() {
    // |params| -> expr  or  [caps] |params| -> expr  or  |params| { stmts }
    auto n = std::make_unique<LambdaExpr>();
    n->line = peek().line;

    // capture list: [copy x, ref y]
    if (check(TK::LBracket)) {
        advance();
        while (!check(TK::RBracket) && !check(TK::Eof)) {
            CaptureSpec cs;
            cs.mode = advance().value; // copy / ref
            cs.var  = advance().value;
            n->captures.push_back(cs);
            if (!match(TK::Comma)) break;
        }
        expect(TK::RBracket, "expected ']'");
    }

    expect(TK::Lambda, "expected '|'");
    while (!check(TK::Lambda) && !check(TK::Eof)) {
        LambdaParam lp;
        // optional type before name: type is keyword (KwInt etc.) or Ident followed by Ident
        TK t0 = peek().kind;
        bool isTypeKw = (t0 >= TK::KwInt && t0 <= TK::KwDim);
        bool isTypeIdent = (t0 == TK::Ident && peek(1).kind == TK::Ident);
        if (isTypeKw || isTypeIdent) {
            lp.type = advance().value;
            lp.name = advance().value;
        } else {
            lp.name = advance().value;
        }
        n->params.push_back(lp);
        if (!match(TK::Comma)) break;
    }
    expect(TK::Lambda, "expected '|'");

    if (check(TK::Arrow)) {
        advance();
        n->body_expr = parseExpr();
    } else if (check(TK::LBrace)) {
        n->body_stmts = parseBlock();
    }
    return n;
}

ExprPtr Parser::parsePrimary() {
    int line = peek().line;
    TK k = peek().kind;

    // await expr → UnaryExpr("await", expr)
    if (k == TK::KwAwait) {
        advance();
        auto n = std::make_unique<UnaryExpr>(); n->line = line;
        n->op = "await"; n->operand = parseExpr();
        return n;
    }

    // lambda shorthand: |x| -> ...
    if (k == TK::Lambda || (k == TK::LBracket && /* capture list */true)) {
        if (k == TK::Lambda) return parseLambda();
        // check if it's capture list (not array)
        // heuristic: [copy/ref ident, ...]
        if (k == TK::LBracket && pos_+1 < toks_.size() &&
            (toks_[pos_+1].value == "copy" || toks_[pos_+1].value == "ref")) {
            return parseLambda();
        }
    }

    if (k == TK::LitInt) {
        auto n=std::make_unique<IntLit>(); n->line=line;
        std::string v=advance().value;
        if (v.size()>2 && v[0]=='0' && (v[1]=='b'||v[1]=='B'))
            n->value = std::stol(v.substr(2), nullptr, 2);
        else if (v.size()>2 && v[0]=='0' && (v[1]=='x'||v[1]=='X'))
            n->value = std::stol(v.substr(2), nullptr, 16);
        else n->value = std::stol(v);
        return n;
    }
    if (k == TK::LitLong)   { auto n=std::make_unique<LongLit>(); n->line=line; std::string v=advance().value; v.pop_back(); n->value=std::stoll(v); return n; }
    if (k == TK::LitDouble) { auto n=std::make_unique<DoubleLit>(); n->line=line; n->value=std::stod(advance().value); return n; }
    if (k == TK::LitFloat)  { auto n=std::make_unique<FloatLit>(); n->line=line; n->value=std::stof(advance().value); return n; }
    if (k == TK::LitString) { auto n=std::make_unique<StringLit>(); n->line=line; n->value=advance().value; return n; }
    if (k == TK::LitTemplate) {
        // Template string: `Hello ${name}!` → std::string("Hello ") + __drv_to_str(name) + "!"
        std::string raw = advance().value;
        // Split raw into [text, ${...}, text, ${...}, ...]
        // Build chain: str + to_str(expr) + str + ...
        std::vector<ExprPtr> parts;
        size_t p = 0;
        while (p <= raw.size()) {
            size_t found = raw.find("${", p);
            // Add the text segment before ${
            std::string seg = (found == std::string::npos) ? raw.substr(p) : raw.substr(p, found-p);
            if (!seg.empty()) {
                auto sl = std::make_unique<StringLit>(); sl->line = line; sl->value = seg;
                parts.push_back(std::move(sl));
            }
            if (found == std::string::npos) break;
            // Find closing }
            size_t close = raw.find('}', found+2);
            if (close == std::string::npos) { p = raw.size()+1; break; }
            std::string expr_src = raw.substr(found+2, close-(found+2));
            // Parse expr_src as expression (re-lex + re-parse)
            Lexer sub_lex(expr_src, file_);
            auto sub_toks = sub_lex.tokenize();
            if (!sub_lex.errors().empty()) { p = close+1; continue; }
            Parser sub_par(std::move(sub_toks), file_);
            ExprPtr expr_node = sub_par.parseExpr();
            // Wrap in to_str call
            auto to_str_call = std::make_unique<CallExpr>(); to_str_call->line = line;
            auto to_str_id = std::make_unique<IdentExpr>(); to_str_id->name = "__drv_to_str";
            to_str_call->callee = std::move(to_str_id);
            to_str_call->args.push_back(std::move(expr_node));
            parts.push_back(std::move(to_str_call));
            p = close+1;
        }
        if (parts.empty()) {
            auto n = std::make_unique<StringLit>(); n->line = line; n->value = raw; return n;
        }
        // Chain parts with + operator
        ExprPtr result = std::move(parts[0]);
        for (size_t i=1; i<parts.size(); ++i) {
            auto bin = std::make_unique<BinaryExpr>(); bin->line = line;
            bin->op = "+"; bin->left = std::move(result); bin->right = std::move(parts[i]);
            result = std::move(bin);
        }
        return result;
    }
    if (k == TK::LitChar)   { auto n=std::make_unique<CharLit>();  n->line=line; n->value=advance().value[0]; return n; }
    if (k == TK::KwTrue)    { advance(); auto n=std::make_unique<BoolLit>(); n->line=line; n->value=true; return n; }
    if (k == TK::KwFalse)   { advance(); auto n=std::make_unique<BoolLit>(); n->line=line; n->value=false; return n; }
    if (k == TK::KwNull) { advance(); return std::make_unique<NullLit>(); }
    if (k == TK::KwNone) { advance(); return std::make_unique<NoneLit>(); }

    if (k == TK::KwSome) {
        advance(); expect(TK::LParen,"expected '('");
        auto v = parseExpr(); expect(TK::RParen,"expected ')'");
        auto n = std::make_unique<SomeExpr>(); n->line=line; n->value=std::move(v); return n;
    }
    if (k == TK::KwOk) {
        advance(); expect(TK::LParen,"expected '('");
        auto v = parseExpr(); expect(TK::RParen,"expected ')'");
        auto n = std::make_unique<OkExpr>(); n->line=line; n->value=std::move(v); return n;
    }
    if (k == TK::KwErr) {
        advance(); expect(TK::LParen,"expected '('");
        auto v = parseExpr(); expect(TK::RParen,"expected ')'");
        auto n = std::make_unique<ErrExpr>(); n->line=line; n->value=std::move(v); return n;
    }

    if (k == TK::KwNew) {
        advance();
        auto n = std::make_unique<NewExpr>(); n->line=line;
        // Parse full generic type: new Foo<Bar<T>>() — use TypeRef then stringify
        TypeRef tr = parseTypeRef();
        n->type_name = typeRefToStr(tr);
        if (check(TK::LBracket)) {
            advance(); n->array_size = parseExpr(); expect(TK::RBracket,"expected ']'");
        } else if (check(TK::LParen)) {
            advance(); n->ctor_args = parseArgList();
        }
        return n;
    }

    // array literal: [1, 2, 3]
    if (k == TK::LBracket) {
        advance();
        // build as a call to __array_init
        auto call = std::make_unique<CallExpr>(); call->line=line;
        auto callee = std::make_unique<IdentExpr>(); callee->name = "__array_init";
        call->callee = std::move(callee);
        while (!check(TK::RBracket) && !check(TK::Eof)) {
            call->args.push_back(parseExpr());
            if (!match(TK::Comma)) break;
        }
        expect(TK::RBracket,"expected ']'");
        return call;
    }

    // map literal: {"a": 1, "b": 2}  or  {}
    if (k == TK::LBrace) {
        advance();
        auto call = std::make_unique<CallExpr>(); call->line=line;
        auto callee = std::make_unique<IdentExpr>(); callee->name = "__map_init";
        call->callee = std::move(callee);
        while (!check(TK::RBrace) && !check(TK::Eof)) {
            call->args.push_back(parseExpr());
            expect(TK::Colon,"expected ':'");
            call->args.push_back(parseExpr());
            if (!match(TK::Comma)) break;
        }
        expect(TK::RBrace,"expected '}'");
        return call;
    }

    // grouped expr: (expr)
    if (k == TK::LParen) {
        advance();
        auto e = parseExpr();
        // If parseExpr() returned nullptr (panic or depth limit), skip to the
        // matching ')' without trying to build an AST node.
        if (!e) {
            // skip remaining tokens up to the first unmatched ')'
            int depth = 1;
            while (!check(TK::Eof) && depth > 0) {
                if      (check(TK::LParen)) { ++depth; advance(); }
                else if (check(TK::RParen)) { --depth; if (depth >= 0) advance(); }
                else    advance();
            }
            return nullptr;
        }
        expect(TK::RParen, "expected ')'");
        return e;
    }

    // identifier
    if (k == TK::Ident || (k >= TK::KwInt && k <= TK::KwDimKw)) {
        auto n = std::make_unique<IdentExpr>(); n->line=line; n->name=advance().value; return n;
    }

    {
        std::string tv = peek().kind == TK::Eof
            ? "<end of file>"
            : (peek().value.empty() ? std::string(tk_name(peek().kind)) : peek().value);
        error("unexpected token '" + tv + "'");
    }
    advance();
    auto dummy = std::make_unique<IntLit>(); dummy->value = 0; return dummy;
}

} // namespace drv
