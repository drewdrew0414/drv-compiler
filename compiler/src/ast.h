#pragma once
#include "token.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace drv {

// ── Forward declarations ──────────────────────────────────────────────────────
struct Expr; struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using ExprList = std::vector<ExprPtr>;
using StmtList = std::vector<StmtPtr>;

// ─────────────────────────────────────────────────────────────────────────────
// Expressions
// ─────────────────────────────────────────────────────────────────────────────
struct Expr {
    int line{0}, col{0};
    virtual ~Expr() = default;
};

struct IntLit    : Expr { long value; };
struct LongLit   : Expr { long long value; };
struct DoubleLit : Expr { double value; };
struct FloatLit  : Expr { float value; };
struct BoolLit   : Expr { bool value; };
struct StringLit : Expr { std::string value; bool is_template{false}; };
struct CharLit   : Expr { char value; };
struct NullLit   : Expr {};
struct NoneLit   : Expr {};

struct IdentExpr : Expr { std::string name; };

struct BinaryExpr : Expr {
    std::string op;
    ExprPtr left, right;
};

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr operand;
    bool prefix{true};
};

struct CallExpr : Expr {
    ExprPtr   callee;
    ExprList  args;
};

struct IndexExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    ExprPtr index2; // for 2D: mat[r, c] or slice mat[a..b, c..d]
};

struct MemberExpr : Expr {
    ExprPtr     object;
    std::string field;
};

struct SomeExpr : Expr { ExprPtr value; };
struct OkExpr   : Expr { ExprPtr value; };
struct ErrExpr  : Expr { ExprPtr value; };

// new T() / new T[N]
struct NewExpr : Expr {
    std::string type_name;
    ExprList    type_args;  // generic params
    ExprList    ctor_args;
    ExprPtr     array_size; // non-null for new T[N]
};

// move x
struct MoveExpr : Expr { ExprPtr operand; };

// |params| -> expr  or  [copy/ref vars] |params| { body }
struct LambdaParam { std::string type; std::string name; };
struct CaptureSpec { std::string mode; std::string var; }; // "copy" / "ref"

struct LambdaExpr : Expr {
    std::vector<CaptureSpec> captures;
    std::vector<LambdaParam> params;
    ExprPtr  body_expr;  // short form
    StmtList body_stmts; // block form
};

// expr |> func
struct PipeExpr : Expr {
    ExprPtr left;
    ExprPtr right;
};

// a..b
struct RangeExpr : Expr { ExprPtr from, to; };

// ─────────────────────────────────────────────────────────────────────────────
// Type representation (used in declarations)
// ─────────────────────────────────────────────────────────────────────────────
struct TypeRef {
    std::string             name;       // "int", "String", "Own", ...
    std::vector<TypeRef>    args;       // generic params
    bool                    is_array{false};  // int[]
    bool                    is_mut{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// Statements
// ─────────────────────────────────────────────────────────────────────────────
struct Stmt {
    int line{0}, col{0};
    std::vector<std::string> annotations; // @inline, @pure, ...
    virtual ~Stmt() = default;
};

// var / typed declaration
struct VarDecl : Stmt {
    TypeRef   type;
    std::string name;
    ExprPtr   init;
};

// function declaration
struct Param { TypeRef type; std::string name; ExprPtr default_val; };

struct FuncDecl : Stmt {
    std::vector<std::string> modifiers;
    TypeRef                  ret_type;
    std::string              name;
    std::vector<std::string> type_params;            // generics <T>
    std::vector<std::string> trait_bounds;           // T implements Foo
    std::vector<Param>       params;
    StmtList                 body;
    bool                     is_async{false};
};

// class declaration
struct FieldDecl { std::vector<std::string> modifiers; TypeRef type; std::vector<std::string> names; };

struct ClassDecl : Stmt {
    std::vector<std::string> modifiers;
    std::string              name;
    std::string              base;            // extends
    std::vector<std::string> type_params;     // generic <T, U>
    std::vector<std::string> trait_bounds;    // T implements Foo
    std::vector<FieldDecl>   fields;
    std::vector<StmtPtr>     methods;
};

// trait declaration
struct TraitDecl : Stmt {
    std::string            name;
    std::vector<FuncDecl*> methods; // raw ptrs to method stmts inside
    StmtList               body;
};

// impl Trait for Class
struct ImplDecl : Stmt {
    std::string trait_name;
    std::string class_name;
    StmtList    methods;
};

// dim Unit = "symbol"
struct DimDecl : Stmt {
    std::string name;
    std::string symbol;
};

// module math_utils;
struct ModuleDecl : Stmt { std::string name; };

// use math_utils; / use math_utils::fn;
struct UseDecl : Stmt { std::string module; std::string symbol; /* empty = all */ };

// extern "C" / extern "FFI" { ... }
struct ExternDecl : Stmt {
    std::string abi;          // "C", "FFI"
    bool        unsafe_legacy{false};
    StmtList    declarations;
};

// ── Control flow ─────────────────────────────────────────────────────────────
struct ExprStmt : Stmt { ExprPtr expr; };

struct Block : Stmt { StmtList stmts; };

struct ReturnStmt : Stmt { ExprPtr value; };
struct BreakStmt  : Stmt {};
struct ContinueStmt : Stmt {};
struct ThrowStmt  : Stmt { ExprPtr value; };
struct PassStmt   : Stmt {};

struct IfStmt : Stmt {
    ExprPtr  cond;
    StmtList then_body;
    StmtList else_body;
};

// for (i in 0..N) { } — range
struct ForRangeStmt : Stmt {
    std::string var;
    ExprPtr     from, to, step;   // step may be nullptr (default step=1)
    StmtList    body;
    bool        is_simd{false};
    bool        is_parallel{false};
    // reduction: "operator:varname"
    std::vector<std::string> reductions;
};

// for (item of collection) { }
struct ForEachStmt : Stmt {
    std::string var;
    ExprPtr     collection;
    StmtList    body;
    bool        is_simd{false};
    bool        is_parallel{false};
    std::vector<std::string> reductions;
};

// for (int i = 0; i < N; i++) { }
struct ForCStyleStmt : Stmt {
    StmtPtr  init;
    ExprPtr  cond;
    ExprPtr  step;
    StmtList body;
};

struct WhileStmt : Stmt { ExprPtr cond; StmtList body; };

// switch (x) { case(v) -> { }; }
struct CaseArm  { ExprPtr value; bool is_default{false}; StmtList body; };
struct SwitchStmt : Stmt { ExprPtr expr; std::vector<CaseArm> arms; };

// match (x) { Some(v) => { }; Ok(v) => { }; _ => { }; }
struct MatchArm {
    std::string pattern;      // "Some", "None", "Ok", "Err", "_", literal
    std::string bind_var;     // variable bound in pattern
    StmtList    body;
};
struct MatchStmt : Stmt { ExprPtr expr; std::vector<MatchArm> arms; };

// try { } catch (err) { }
struct TryCatchStmt : Stmt {
    StmtList    try_body;
    std::string catch_var;
    StmtList    catch_body;
};

// @region name { }
struct RegionStmt : Stmt { std::string name; StmtList body; };

// spawn { }
struct SpawnStmt : Stmt { StmtList body; };

// exists (var) { } else { }
struct ExistsStmt : Stmt {
    std::string var;
    StmtList    then_body;
    StmtList    else_body;
};

// static_if (COND) { } else { }
struct StaticIfStmt : Stmt {
    ExprPtr  cond;
    StmtList then_body;
    StmtList else_body;
};

// ── Top-level program ─────────────────────────────────────────────────────────
struct Program { StmtList stmts; };

} // namespace drv
