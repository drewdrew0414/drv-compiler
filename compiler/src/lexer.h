#pragma once
#include "token.h"
#include <string>
#include <vector>

namespace drv {

class Lexer {
public:
    explicit Lexer(std::string source, std::string filename = "<stdin>");

    std::vector<Token> tokenize();

    const std::vector<std::string>& errors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    std::string  src_;
    std::string  file_;
    size_t       pos_{0};
    int          line_{1};
    int          col_{1};
    std::vector<std::string> errors_;

    char        peek(int offset = 0) const noexcept;
    char        advance() noexcept;
    bool        match(char c) noexcept;
    void        skipWhitespace() noexcept;
    void        skipLineComment() noexcept;
    void        skipBlockComment() noexcept;

    Token       makeToken(TK kind, std::string value) const noexcept;
    Token       lexNumber();
    Token       lexString();
    Token       lexChar();
    Token       lexTemplateString();  // `...{expr}...`
    Token       lexIdentOrKeyword();
    Token       lexAt();
    Token       lexOperator();

    static TK   keywordKind(std::string_view s) noexcept;

    void        error(const std::string& msg);
};

} // namespace drv
