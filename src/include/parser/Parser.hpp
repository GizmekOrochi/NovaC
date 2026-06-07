#pragma once

#include "../ast/Node.hpp"
#include "../token/Token.hpp"
#include "../registry/Registry.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace novac::parser {

class ParserContext;

using ParseFn = std::function<ast::NodePtr(ParserContext &)>;
using PrefixFn = std::function<ast::NodePtr(ParserContext &)>;
using InfixFn = std::function<ast::NodePtr(ParserContext &, ast::NodePtr)>;
using PostfixFn = std::function<ast::NodePtr(ParserContext &, ast::NodePtr)>;

struct InfixRule {
    int precedence{};
    InfixFn fn{};
};

struct PostfixRule {
    int precedence{};
    PostfixFn fn{};
};

struct ParseDomain {
    std::unordered_map<std::string, ParseFn> rules{};
    std::unordered_map<std::string, PrefixFn> prefixes{};
    std::unordered_map<std::string, InfixRule> infixes{};
    std::unordered_map<std::string, PostfixRule> postfixes{};
    std::vector<ParseFn> fallbacks{};
};

class ParserRegistry {
public:
    explicit ParserRegistry(registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    registry::RegisterStatus rule(std::string domain, std::string key, ParseFn fn);
    registry::RegisterStatus fallback(std::string domain, ParseFn fn);
    registry::RegisterStatus prefix(std::string domain, std::string key, PrefixFn fn);
    registry::RegisterStatus infix(std::string domain, std::string op, int precedence, InfixFn fn);
    registry::RegisterStatus postfix(std::string domain, std::string op, int precedence, PostfixFn fn);

    ast::NodePtr parse(ParserContext &context, const std::string &domain, int minPrecedence = 0) const;

private:
    static std::string tokenKey(const token::Token &token);

    ast::NodePtr parsePratt(ParserContext &context, const std::string &domain, const ParseDomain &rules, int minPrecedence) const;

    std::unordered_map<std::string, ParseDomain> domains_;
    registry::DuplicatePolicy duplicatePolicy_;
};

class ParserContext {
public:
    ParserContext(std::vector<token::Token> tokens, const ParserRegistry &registry);

    const token::Token &cur() const;
    const token::Token &peek(std::size_t offset = 1) const;

    bool end() const;
    bool check(const std::string &value) const;

    const token::Token &advance();

    const token::Token &consume(const std::string &value);
    const token::Token &consumeKind(token::Kind kind);

    ast::NodePtr parse(const std::string &domain, int minPrecedence = 0);

private:
    std::vector<token::Token> tokens_;
    std::size_t pos_;
    const ParserRegistry &registry_;
};

class Parser {
public:
    Parser(const ParserRegistry &registry, std::string startDomain);

    ast::NodePtr parse(std::vector<token::Token> tokens) const;

private:
    const ParserRegistry &registry_;
    std::string startDomain_;
};

} // namespace novac::parser