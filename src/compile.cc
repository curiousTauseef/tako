#include "checker.h"
#include "eval.h"
#include "parser.h"
#include "show.h"
#include <iostream>

std::optional<Tree<Token>> getTree(Context &ctx) {
  if (ctx.done()) {
    return {};
  }

  Tokens toks = lex(ctx);
  if (ctx.done()) {
    std::cerr << "Lexed " << toks.size() << " tokens.\n";
    std::cerr << show(toks, ctx) << "\n";
    return {};
  }

  std::optional<Tree<Token>> tree = ast::ast(toks, ctx, ast::parseModule);
  if (ctx.done()) {
    std::cerr << show(*tree, ctx) << "\n";
  }
  return tree;
}

void finish(parser::ParserContext &ctx) {
  for (const auto msg : ctx.getMsgs()) {
    std::cerr << show(msg, ctx, 2) << "\n";
  }
  ctx.getMsgs() = {}; // Clear out the message log.
}

Prim runCompilerInteractive(Context &ctx) {
  try {
    auto tree = getTree(ctx);
    if (!tree || ctx.done()) {
      return PrimError("Program not run");
    }

    parser::ParserContext p_ctx(std::move(ctx));
    p_ctx.allowOverrides = true;
    std::optional<Module> o_mod =
        parser::parse<std::optional<Module>>(*tree, p_ctx, parser::parseModule);
    if (!o_mod) {
      return PrimError("Parse Failed");
    }
    if (p_ctx.done()) {
      std::cerr << show(*o_mod) << "\n";
      for (const auto msg : p_ctx.getMsgs()) {
        std::cerr << show(msg, p_ctx, 2) << "\n";
      }
      return PrimError("Program not run");
    }
    auto mod = *o_mod;
    CheckedModule checked = check(mod, p_ctx);
    if (p_ctx.done()) {
      std::cerr << show(checked) << "\n";
      return PrimError("Program not run");
    }

    // TODO
    Path context = {};
    const auto res = eval(context, mod, p_ctx);
    if (std::holds_alternative<PrimError>(res)) {
      p_ctx.msgAt(mod.loc, MessageType::Warning, std::get<PrimError>(res).msg);
    }
    std::cerr << show(res) << "\n";

    if (!p_ctx.done()) {
      finish(p_ctx);
    }
    return res;
  } catch (const std::runtime_error &er) {
    std::cerr << "Parser crashed with: " << er.what() << "\n";
  }
  return PrimError("Program crashed");
}

void runCompiler(Context &ctx) {
  try {
    auto tree = getTree(ctx);
    if (!tree || ctx.done()) {
      return;
    }

    parser::ParserContext p_ctx(std::move(ctx));
    Module module = parser::parse<Module>(*tree, p_ctx, parser::parseModule);
    if (p_ctx.done()) {
      auto &symbs = p_ctx.symbols;
      symbs.forAll([](Path &context, Definition &def) {
        std::cerr << "path: " << show(context, 0, "/") << "\n";
        std::cerr << "def: " << show(def) << "\n";
      });
      // std::cerr << show(module, 0) << "\n";
      return;
    }

    CheckedModule checked = check(module, p_ctx);
    if (p_ctx.done()) {
      std::cerr << show(checked) << "\n";
      return;
    }

    // TODO: Code gen (no biggy)
    finish(p_ctx);
  } catch (const std::runtime_error &er) {
    std::cerr << "Parser crashed with: " << er.what() << "\n";
  }
}
