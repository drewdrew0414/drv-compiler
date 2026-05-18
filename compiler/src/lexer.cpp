#include "lexer.h"
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace drv {

// ── keyword table ────────────────────────────────────────────────────────────
static const std::unordered_map<std::string_view, TK> KEYWORDS = {
    {"int",           TK::KwInt},      {"long",          TK::KwLong},
    {"float",         TK::KwFloat},    {"double",        TK::KwDouble},
    {"char",          TK::KwChar},     {"String",        TK::KwString},
    {"boolean",       TK::KwBoolean},  {"void",          TK::KwVoid},
    {"var",           TK::KwVar},
    {"tensor",        TK::KwTensor},   {"list",          TK::KwList},
    {"Map",           TK::KwMap},      {"Union",         TK::KwUnion},
    {"Own",           TK::KwOwn},      {"Ref",           TK::KwRef_},
    {"Borrow",        TK::KwBorrow},   {"Option",        TK::KwOption},
    {"Result",        TK::KwResult},
    {"if",            TK::KwIf},       {"else",          TK::KwElse},
    {"for",           TK::KwFor},      {"while",         TK::KwWhile},
    {"switch",        TK::KwSwitch},   {"case",          TK::KwCase},
    {"default",       TK::KwDefault},  {"match",         TK::KwMatch},
    {"break",         TK::KwBreak},    {"continue",      TK::KwContinue},
    {"pass",          TK::KwPass},     {"return",        TK::KwReturn},
    {"exists",        TK::KwExists},
    {"class",         TK::KwClass},    {"extends",       TK::KwExtends},
    {"abstract",      TK::KwAbstract}, {"trait",         TK::KwTrait},
    {"impl",          TK::KwImpl},     {"override",      TK::KwOverride},
    {"this",          TK::KwThis},     {"new",           TK::KwNew},
    {"Self",          TK::KwSelf},
    {"public",        TK::KwPublic},   {"private",       TK::KwPrivate},
    {"static",        TK::KwStatic},   {"final",         TK::KwFinal},
    {"synchronized",  TK::KwSynchronized},
    {"const",         TK::KwConst},    {"unsigned",      TK::KwUnsigned},
    {"mut",           TK::KwMut},      {"move",          TK::KwMove},
    {"ref",           TK::KwRef},
    {"parallel",      TK::KwParallel}, {"simd",          TK::KwSimd},
    {"async",         TK::KwAsync},    {"await",         TK::KwAwait},
    {"spawn",         TK::KwSpawn},
    {"where",         TK::KwWhere},    {"otherwise",     TK::KwOtherwise},
    {"reduction",     TK::KwReduction},
    {"module",        TK::KwModule},   {"use",           TK::KwUse},
    {"try",           TK::KwTry},      {"catch",         TK::KwCatch},
    {"throw",         TK::KwThrow},
    {"and",           TK::KwAnd},      {"or",            TK::KwOr},
    {"not",           TK::KwNot},      {"in",            TK::KwIn},
    {"of",            TK::KwOf},       {"is",            TK::KwIs},
    {"as",            TK::KwAs},
    {"true",          TK::KwTrue},     {"false",         TK::KwFalse},
    {"null",          TK::KwNull},     {"None",          TK::KwNone},
    {"Some",          TK::KwSome},     {"Ok",            TK::KwOk},
    {"Err",           TK::KwErr},
    {"lazy",          TK::KwLazy},        {"extern",        TK::KwExtern},
    {"compile_eval",  TK::KwCompileEval}, {"static_if",     TK::KwStaticIf},
    {"dim",           TK::KwDimKw},       {"atomic",        TK::KwAtomic},
    {"fn",            TK::KwFn},
    // sys.sync memory barrier builtins
    {"fence_full",    TK::KwFenceFull},   {"fence_acquire", TK::KwFenceAcquire},
    {"fence_release", TK::KwFenceRelease},
    {"atomic_load",   TK::KwAtomicLoad},  {"atomic_store",  TK::KwAtomicStore},
    {"atomic_cas",    TK::KwAtomicCAS},
};

TK Lexer::keywordKind(std::string_view s) noexcept {
    auto it = KEYWORDS.find(s);
    return it != KEYWORDS.end() ? it->second : TK::Ident;
}

// ── helpers ──────────────────────────────────────────────────────────────────
Lexer::Lexer(std::string source, std::string filename)
    : src_(std::move(source)), file_(std::move(filename)) {}

char Lexer::peek(int offset) const noexcept {
    size_t idx = pos_ + offset;
    return idx < src_.size() ? src_[idx] : '\0';
}

char Lexer::advance() noexcept {
    char c = src_[pos_++];
    if (c == '\n') { ++line_; col_ = 1; }
    else           { ++col_; }
    return c;
}

bool Lexer::match(char c) noexcept {
    if (peek() == c) { advance(); return true; }
    return false;
}

Token Lexer::makeToken(TK kind, std::string value) const noexcept {
    return Token{kind, std::move(value), line_, col_};
}

void Lexer::error(const std::string& msg) {
    errors_.push_back(file_ + ":" + std::to_string(line_) + ":" +
                      std::to_string(col_) + ": error: " + msg);
}

// ── whitespace / comments ────────────────────────────────────────────────────
void Lexer::skipWhitespace() noexcept {
    while (pos_ < src_.size() && std::isspace((unsigned char)src_[pos_]))
        advance();
}

void Lexer::skipLineComment() noexcept {
    while (pos_ < src_.size() && src_[pos_] != '\n')
        advance();
}

void Lexer::skipBlockComment() noexcept {
    // drv block comment: !# ... ##
    while (pos_ < src_.size()) {
        if (src_[pos_] == '#' && pos_ + 1 < src_.size() && src_[pos_+1] == '#') {
            advance(); advance();
            return;
        }
        advance();
    }
    error("unterminated block comment (expected ##)");
}

// ── lexing ───────────────────────────────────────────────────────────────────
Token Lexer::lexNumber() {
    size_t start = pos_ - 1;
    int    startLine = line_, startCol = col_;
    bool   isFloat = false;

    // hex / binary literal
    if (src_[start] == '0' && pos_ < src_.size()) {
        if (src_[pos_] == 'x' || src_[pos_] == 'X') {
            advance();
            while (pos_ < src_.size() && std::isxdigit((unsigned char)src_[pos_])) advance();
            return Token{TK::LitInt, src_.substr(start, pos_-start), startLine, startCol};
        }
        if (src_[pos_] == 'b' || src_[pos_] == 'B') {
            advance();
            while (pos_ < src_.size() && (src_[pos_]=='0'||src_[pos_]=='1')) advance();
            return Token{TK::LitInt, src_.substr(start, pos_-start), startLine, startCol};
        }
    }

    while (pos_ < src_.size() && std::isdigit((unsigned char)src_[pos_])) advance();

    if (pos_ < src_.size() && src_[pos_] == '.' && pos_+1 < src_.size() && std::isdigit((unsigned char)src_[pos_+1])) {
        isFloat = true;
        advance();
        while (pos_ < src_.size() && std::isdigit((unsigned char)src_[pos_])) advance();
    }

    // exponent
    if (pos_ < src_.size() && (src_[pos_]=='e'||src_[pos_]=='E')) {
        isFloat = true;
        advance();
        if (pos_ < src_.size() && (src_[pos_]=='+'||src_[pos_]=='-')) advance();
        while (pos_ < src_.size() && std::isdigit((unsigned char)src_[pos_])) advance();
    }

    std::string raw = src_.substr(start, pos_-start);

    // suffix
    if (pos_ < src_.size() && src_[pos_] == 'L') { advance(); return Token{TK::LitLong,   raw+"L", startLine, startCol}; }
    if (pos_ < src_.size() && src_[pos_] == 'f') { advance(); return Token{TK::LitFloat,  raw+"f", startLine, startCol}; }

    return Token{isFloat ? TK::LitDouble : TK::LitInt, raw, startLine, startCol};
}

Token Lexer::lexString() {
    int startLine = line_, startCol = col_;
    std::string val;
    while (pos_ < src_.size() && src_[pos_] != '"') {
        if (src_[pos_] == '\\') {
            advance();
            char esc = advance();
            switch (esc) {
                case 'n':  val += '\n'; break;
                case 't':  val += '\t'; break;
                case '"':  val += '"';  break;
                case '\\': val += '\\'; break;
                default:   val += '\\'; val += esc; break;
            }
        } else {
            val += advance();
        }
    }
    if (pos_ >= src_.size()) error("unterminated string literal");
    else advance(); // closing "
    return Token{TK::LitString, val, startLine, startCol};
}

Token Lexer::lexChar() {
    int startLine = line_, startCol = col_;
    std::string val;
    if (pos_ < src_.size() && src_[pos_] == '\\') {
        advance(); val += '\\'; val += advance();
    } else if (pos_ < src_.size()) {
        val += advance();
    }
    if (pos_ < src_.size() && src_[pos_] == '\'') advance();
    else error("unterminated char literal");
    return Token{TK::LitChar, val, startLine, startCol};
}

Token Lexer::lexTemplateString() {
    // `...{expr}...` — store raw content, parser handles interpolation
    int startLine = line_, startCol = col_;
    std::string val;
    while (pos_ < src_.size() && src_[pos_] != '`') {
        val += advance();
    }
    if (pos_ >= src_.size()) error("unterminated template string");
    else advance(); // closing `
    return Token{TK::LitTemplate, val, startLine, startCol};
}

Token Lexer::lexIdentOrKeyword() {
    size_t start = pos_ - 1;
    int startLine = line_, startCol = col_;
    while (pos_ < src_.size() && (std::isalnum((unsigned char)src_[pos_]) || src_[pos_] == '_'))
        advance();
    std::string text = src_.substr(start, pos_-start);
    TK kind = keywordKind(text);
    return Token{kind, text, startLine, startCol};
}

Token Lexer::lexAt() {
    int startLine = line_, startCol = col_;
    std::string name = "@";
    while (pos_ < src_.size() && (std::isalnum((unsigned char)src_[pos_]) || src_[pos_]=='_'))
        name += advance();
    return Token{TK::At, name, startLine, startCol};
}

Token Lexer::lexOperator() {
    int startLine = line_, startCol = col_;
    char c = src_[pos_-1]; // already advanced by caller

    auto mk = [&](TK k, std::string s) {
        return Token{k, std::move(s), startLine, startCol};
    };

    switch (c) {
        case '+': if (match('=')) return mk(TK::PlusEq,"+=" ); if (match('+')) return mk(TK::PlusPlus,"++");  return mk(TK::Plus,  "+");
        case '-': if (match('=')) return mk(TK::MinusEq,"-="); if (match('-')) return mk(TK::MinusMinus,"--"); if (match('>')) return mk(TK::Arrow,"->"); return mk(TK::Minus, "-");
        case '*': if (match('*')) return mk(TK::StarStar,"**"); if (match('=')) return mk(TK::StarEq,"*="); return mk(TK::Star,"*");
        case '/': if (match('/')) { if (match('=')) return mk(TK::SlashEq,"/=="); return mk(TK::SlashSlash,"//"); } if (match('=')) return mk(TK::SlashEq,"/="); return mk(TK::Slash,"/");
        case '%': if (match('=')) return mk(TK::PercentEq,"%="); return mk(TK::Percent,"%");
        case '=': if (match('=')) return mk(TK::EqEq,"=="); if (match('>')) return mk(TK::FatArrow,"=>"); return mk(TK::Eq,"=");
        case '!': if (match('=')) return mk(TK::BangEq,"!="); return mk(TK::Bang,"!");
        case '<': if (match('<')) return mk(TK::LtLt,"<<"); if (match('=')) return mk(TK::LtEq,"<="); return mk(TK::Lt,"<");
        case '>': if (match('>')) return mk(TK::GtGt,">>"); if (match('=')) return mk(TK::GtEq,">="); return mk(TK::Gt,">");
        case '&': if (match('&')) return mk(TK::AmpAmp,"&&"); return mk(TK::Amp,"&");
        case '^': return mk(TK::Caret,"^");
        case '~': return mk(TK::Tilde,"~");
        case '.': if (match('.')) return mk(TK::DotDot,".."); return mk(TK::Dot,".");
        case ':': if (match(':')) return mk(TK::ColonColon,"::"); return mk(TK::Colon,":");
        case '|': if (match('>')) return mk(TK::PipeGt,"|>"); if (match('|')) return mk(TK::PipePipe,"||"); return mk(TK::Lambda,"|");
        case '(': return mk(TK::LParen,"(");
        case ')': return mk(TK::RParen,")");
        case '{': return mk(TK::LBrace,"{");
        case '}': return mk(TK::RBrace,"}");
        case '[': return mk(TK::LBracket,"[");
        case ']': return mk(TK::RBracket,"]");
        case ',': return mk(TK::Comma,",");
        case ';': return mk(TK::Semi,";");
        case '#': return mk(TK::Hash,"#");
        default:  return mk(TK::Unknown, std::string(1,c));
    }
}

// ── main tokenize loop ───────────────────────────────────────────────────────
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (pos_ < src_.size()) {
        skipWhitespace();
        if (pos_ >= src_.size()) break;

        int startLine = line_, startCol = col_;
        char c = advance();

        // single-line comment: #
        if (c == '#') {
            skipLineComment();
            continue;
        }

        // block comment: !#...##
        if (c == '!' && peek() == '#') {
            advance(); // skip #
            skipBlockComment();
            continue;
        }

        if (c == '@') { tokens.push_back(lexAt()); continue; }
        if (c == '`') { tokens.push_back(lexTemplateString()); continue; }
        if (c == '"') { tokens.push_back(lexString()); continue; }
        if (c == '\'') { tokens.push_back(lexChar()); continue; }

        if (std::isdigit((unsigned char)c)) { tokens.push_back(lexNumber()); continue; }

        if (std::isalpha((unsigned char)c) || c == '_') {
            tokens.push_back(lexIdentOrKeyword());
            continue;
        }

        tokens.push_back(lexOperator());
    }

    tokens.push_back(Token{TK::Eof, "", line_, col_});
    return tokens;
}

// ── token name (for diagnostics) ─────────────────────────────────────────────
std::string_view tk_name(TK k) noexcept {
    switch (k) {
#define CASE(x) case TK::x: return #x;
        CASE(LitInt) CASE(LitLong) CASE(LitDouble) CASE(LitFloat)
        CASE(LitString) CASE(LitChar) CASE(LitBool)
        CASE(Ident)
        CASE(KwInt) CASE(KwLong) CASE(KwFloat) CASE(KwDouble) CASE(KwChar)
        CASE(KwString) CASE(KwBoolean) CASE(KwVoid) CASE(KwVar)
        CASE(KwIf) CASE(KwElse) CASE(KwFor) CASE(KwWhile) CASE(KwReturn)
        CASE(KwClass) CASE(KwNew) CASE(KwThis)
        CASE(KwPublic) CASE(KwPrivate) CASE(KwConst) CASE(KwMut)
        CASE(KwTrue) CASE(KwFalse) CASE(KwNull)
        CASE(KwAnd) CASE(KwOr) CASE(KwNot) CASE(KwIn) CASE(KwAs)
        CASE(Plus) CASE(Minus) CASE(Star) CASE(Slash) CASE(Percent)
        CASE(Eq) CASE(EqEq) CASE(BangEq) CASE(Lt) CASE(Gt) CASE(LtEq) CASE(GtEq)
        CASE(LParen) CASE(RParen) CASE(LBrace) CASE(RBrace)
        CASE(LBracket) CASE(RBracket)
        CASE(Comma) CASE(Semi) CASE(Colon) CASE(Dot) CASE(DotDot)
        CASE(Arrow) CASE(FatArrow) CASE(PipeGt) CASE(Lambda)
        CASE(At) CASE(Eof) CASE(Unknown)
#undef CASE
        default: return "?";
    }
}

} // namespace drv
