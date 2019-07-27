#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include "lex.h"

const std::string lower = "abcdefghijklmnopqrstuvwxyz";
const std::string upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string nums = "0123456789";

const std::string whiteSpace = " \t\n\r";
const std::string numberChar = "."+nums;
const std::string operatorChar = "-+&#@<>^~∆%•|=÷×°$\\/*:?!.";
const std::string symbolChar = lower+upper+nums+"_";
const std::string quotes = "'\"`";

const std::vector<std::pair<std::string, TokenType>> matchToken = {
  {"(", TokenType::OpenParen},
  {")", TokenType::CloseParen},
  {"{", TokenType::OpenBrace},
  {"}", TokenType::CloseBrace},
  {"[", TokenType::OpenBracket},
  {"]", TokenType::CloseBracket},
  {";", TokenType::SemiColon},
  // {"'", TokenType::SingleQuote},
  // {"\"", TokenType::DoubleQuote},
  // {"`", TokenType::BackQuote},
  {"-|", TokenType::PreCond},
  {"|-", TokenType::PostCond},
  {",", TokenType::Comma}
};

Offset consumeStringLiteral(const std::string content) {
  Offset len = 0;
  char start = content[len];
  if(quotes.find(start) != std::string::npos) {
    len++;
    while(len < content.size() && content[len] != start) {
      len++;
    }
    if (len < content.size()) {
      len++; // consume the 'end'
    } else {
      // TODO: Warn that the literal was unterminated.
    }
  }
  return len;
}

Offset consumeWhiteSpace(const std::string content) {
  Offset loc = 0;
  for(;loc < content.size(); loc++) {
    char cur = content[loc];
    if(whiteSpace.find(cur) != std::string::npos) {
      continue;
    }
    if(cur == '/' && loc+1 < content.size() && content[loc+1] == '*') {
      loc++;
      for(loc++; loc < content.size(); loc++) {
        if(content[loc] == '/' && content[loc-1] == '*') break;
      }
      loc++;
    }
    if((cur == '/' && loc+1 < content.size() && content[loc+1] == '/')||(cur == '#')) {
      for(loc++; loc < content.size(); loc++) {
        if(content[loc] == '\n') break;
      }
    }
    break;
  }
  return loc;
}

int matchesFrom(const std::string chars, const std::string content) {
  Offset length = 0;
  while(length < content.size()) {
    if(chars.find(content[length]) == std::string::npos) {
      break;
    }
    length++;
  }
  return length;
}

std::pair<TokenType, Offset> chooseTok(std::string content) {
  // TODO: use string views.
  for(const auto sym : matchToken) {
    const auto& tokS = sym.first;
    if(content.substr(0, tokS.size()) == tokS) {
      return {sym.second, tokS.size()};
    }
  }
  if(Offset length = consumeStringLiteral(content)) {
    return {TokenType::StringLiteral, length};
  }
  if(Offset length = consumeWhiteSpace(content)) {
    return {TokenType::WhiteSpace, length};
  }
  if(Offset length = matchesFrom(operatorChar, content)) {
    return {TokenType::Operator, length};
  }
  if(Offset length = matchesFrom(numberChar, content)) {
    return {TokenType::NumberLiteral, length};
  }
  if(Offset length = matchesFrom(symbolChar, content)) {
    return {TokenType::Symbol, length};
  }
  return {TokenType::Error, 1};
}

Tokens lex(Context &ctx) {
  ctx.startStep(PassStep::Lex);
  Tokens toks;

  Position pos = 0;
  while(pos < ctx.content.size()) {
    std::pair<TokenType, Offset> next = chooseTok(ctx.content.substr(pos));
    TokenType type = next.first;
    Offset length = next.second;
    Location loc = {pos, length, ctx.filename};
    if(length == 0) {
      ctx.msg(
        loc,
        MessageType::InternalError,
        "Illegal empty token"
      );
    } else if(type == +TokenType::Error) {
      ctx.msg(
        loc,
        MessageType::Error,
        "Unexpected character"
      );
    } else {
      toks.push_back({type, loc});
    }
    pos += length;
  }
  return toks;
}
