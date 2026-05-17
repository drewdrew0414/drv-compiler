#pragma once
#include <string>
#include <string_view>
#include <cstdint>

namespace drv {

enum class TK {
    // Literals
    LitInt, LitLong, LitDouble, LitFloat, LitString, LitChar, LitBool, LitTemplate,

    // Identifier
    Ident,

    // ── Primitive types ────────────────────────────────────────
    KwInt, KwLong, KwFloat, KwDouble, KwChar,
    KwString, KwBoolean, KwVoid, KwVar,

    // ── Collection / generic types ─────────────────────────────
    KwTensor, KwList, KwMap, KwUnion,
    KwOwn, KwRef_, KwBorrow, KwOption, KwResult, KwDim,

    // ── Control flow ───────────────────────────────────────────
    KwIf, KwElse, KwFor, KwWhile, KwSwitch, KwCase, KwDefault,
    KwMatch, KwBreak, KwContinue, KwPass, KwReturn, KwExists,

    // ── OOP ────────────────────────────────────────────────────
    KwClass, KwExtends, KwAbstract, KwTrait, KwImpl,
    KwOverride, KwThis, KwNew, KwSelf,

    // ── Access / storage modifiers ─────────────────────────────
    KwPublic, KwPrivate, KwStatic, KwFinal, KwSynchronized,
    KwConst, KwUnsigned, KwMut,

    // ── Ownership ──────────────────────────────────────────────
    KwMove,

    // ── Parallel / async ───────────────────────────────────────
    KwParallel, KwSimd, KwAsync, KwAwait, KwSpawn,
    KwWhere, KwOtherwise, KwReduction,

    // ── Module ─────────────────────────────────────────────────
    KwModule, KwUse,

    // ── Error handling ─────────────────────────────────────────
    KwTry, KwCatch, KwThrow,

    // ── Logic / type operators ─────────────────────────────────
    KwAnd, KwOr, KwNot, KwIn, KwOf, KwIs, KwAs,

    // ── Value keywords ─────────────────────────────────────────
    KwTrue, KwFalse, KwNull,
    KwNone, KwSome, KwOk, KwErr,

    // ── Misc keywords ──────────────────────────────────────────
    KwLazy, KwExtern, KwCompileEval, KwStaticIf,
    KwUnsafeLegacy, KwRef,   // ref (alias variable)
    KwDimKw,                 // dim

    // ── Annotation ─────────────────────────────────────────────
    At,

    // ── Arithmetic ─────────────────────────────────────────────
    Plus, Minus, Star, Slash, Percent, StarStar, SlashSlash,

    // ── Assignment ─────────────────────────────────────────────
    Eq, PlusEq, MinusEq, StarEq, SlashEq, PercentEq,
    PlusPlus, MinusMinus,

    // ── Comparison ─────────────────────────────────────────────
    EqEq, BangEq, Lt, Gt, LtEq, GtEq,

    // ── Bitwise ────────────────────────────────────────────────
    Amp, Pipe, Caret, Tilde, LtLt, GtGt,

    // ── Boolean (C-style alias, deprecated) ────────────────────
    AmpAmp, PipePipe, Bang,

    // ── Special operators ──────────────────────────────────────
    PipeGt,      // |>
    Arrow,       // ->
    FatArrow,    // =>
    Dot,         // .
    DotDot,      // ..
    ColonColon,  // ::
    Lambda,      // | (lambda open/close)

    // ── Delimiters ─────────────────────────────────────────────
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Semi, Colon, Hash, Backtick,

    // ── End ────────────────────────────────────────────────────
    Eof, Unknown,
};

struct Token {
    TK          kind;
    std::string value;
    int         line{1};
    int         col{1};

    bool is(TK k)  const noexcept { return kind == k; }
    bool isEof()   const noexcept { return kind == TK::Eof; }
    bool isIdent() const noexcept { return kind == TK::Ident; }
};

std::string_view tk_name(TK k) noexcept;

} // namespace drv
