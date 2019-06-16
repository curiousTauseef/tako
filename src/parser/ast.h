#pragma once
#ifndef AST_H
#define AST_H

#include <vector>
#include <string>

#include "../lib/enums.h"
#include "../util/util.h"

BETTER_ENUM(
    TokenType,
    char,
    OpenParen,
    CloseParen,
    OpenBrace,
    CloseBrace,
    OpenBracket,
    CloseBracket,
    Declaration,
    SemiColon,
    PreCond,
    PostCond,
    SingleQuote,
    DoubleQuote,
    BackQuote,
    Dot,
    Comma,
    WhiteSpace,
    NumberLiteral,
    Operator,
    Symbol,
    Error
    );

struct Token {
  TokenType type;
  Location loc;
};

using Tokens = std::vector<Token>;

Result<Tokens> lex(const std::string& filename, const std::string& content);
Result<Tree<Token>> ast(Result<Tokens>& toks, const std::string& content, const std::string& filename);

#endif // #ifndef AST_H
