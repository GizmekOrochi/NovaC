#include "../../include/parser/Parser.hpp"

#include <stdexcept>
#include <utility>

namespace novac::parser {

void ParserRegistry::rule(std::string domain, std::string key, ParseFn fn) {
    domains_[std::move(domain)].rules[std::move(key)] = std::move(fn);
}

void ParserRegistry::fallback(std::string domain, ParseFn fn) {
    domains_[std::move(domain)].fallbacks.push_back(std::move(fn));
}

void ParserRegistry::prefix(std::string domain, std::string key, PrefixFn fn) {
    domains_[std::move(domain)].prefixes[std::move(key)] = std::move(fn);
}

void ParserRegistry::infix(std::string domain, std::string op, int precedence, InfixFn fn) {
    domains_[std::move(domain)].infixes[std::move(op)] = {precedence, std::move(fn)};
}

void ParserRegistry::postfix(std::string domain, std::string op, int precedence, PostfixFn fn) {
    domains_[std::move(domain)].postfixes[std::move(op)] = {precedence, std::move(fn)};
}

ast::NodePtr ParserRegistry::parse(
    ParserContext &context,
    const std::string &domain,
    int minPrecedence) const {
    const auto domainIter{domains_.find(domain)};

    if (domainIter == domains_.end()) {
        throw std::runtime_error(
            "ParserRegistry::parse: unknown parse domain '" + domain + "'");
    }

    const ParseDomain &rules{domainIter->second};
    const std::string key{tokenKey(context.cur())};

    if (!rules.prefixes.empty()) {
        return parsePratt(context, domain, rules, minPrecedence);
    }

    const auto ruleIter{rules.rules.find(key)};

    if (ruleIter != rules.rules.end()) {
        return ruleIter->second(context);
    }

    const auto textRuleIter{rules.rules.find(context.cur().text)};

    if (textRuleIter != rules.rules.end()) {
        return textRuleIter->second(context);
    }

    for (const ParseFn &fallback : rules.fallbacks) {
        if (ast::NodePtr node{fallback(context)}) {
            return node;
        }
    }

    throw std::runtime_error(
        "ParserRegistry::parse: no rule matched domain '" + domain + "'");
}

ast::NodePtr ParserRegistry::parsePratt(
    ParserContext &context,
    const std::string &domain,
    const ParseDomain &rules,
    int minPrecedence) const {
    const auto prefixIter{rules.prefixes.find(tokenKey(context.cur()))};

    if (prefixIter == rules.prefixes.end()) {
        throw std::runtime_error(
            "ParserRegistry::parsePratt: expected expression in domain '" + domain + "'");
    }

    ast::NodePtr left{prefixIter->second(context)};

    while (!context.end()) {
        const std::string op{context.cur().text};

        const auto postfixIter{rules.postfixes.find(op)};

        if (postfixIter != rules.postfixes.end()
            && postfixIter->second.precedence >= minPrecedence) {
            left = postfixIter->second.fn(context, left);
            continue;
        }

        const auto infixIter{rules.infixes.find(op)};

        if (infixIter == rules.infixes.end()
            || infixIter->second.precedence < minPrecedence) {
            break;
        }

        left = infixIter->second.fn(context, left);
    }

    return left;
}

std::string ParserRegistry::tokenKey(const token::Token &token) {
    if (token.kind == token::Kind::Integer) {
        return "$int";
    }

    if (token.kind == token::Kind::Identifier) {
        return "$identifier";
    }

    if (token.kind == token::Kind::Keyword) {
        return "$keyword";
    }

    if (token.kind == token::Kind::End) {
        return "$end";
    }

    return token.text;
}

ParserContext::ParserContext(
    std::vector<token::Token> tokens,
    const ParserRegistry &registry)
    : tokens_{std::move(tokens)},
      pos_{},
      registry_{registry} {}

const token::Token &ParserContext::cur() const {
    return tokens_[pos_];
}

const token::Token &ParserContext::peek(std::size_t offset) const {
    return tokens_[pos_ + offset];
}

bool ParserContext::end() const {
    return cur().kind == token::Kind::End;
}

bool ParserContext::check(const std::string &value) const {
    return cur().text == value;
}

const token::Token &ParserContext::advance() {
    if (!end()) {
        ++pos_;
    }

    return tokens_[pos_ - 1];
}

const token::Token &ParserContext::consume(const std::string &value) {
    if (!check(value)) {
        throw std::runtime_error(
            "ParserContext::consume: expected '" + value + "'");
    }

    return advance();
}

const token::Token &ParserContext::consumeKind(token::Kind kind) {
    if (cur().kind != kind) {
        throw std::runtime_error(
            "ParserContext::consumeKind: unexpected token");
    }

    return advance();
}

ast::NodePtr ParserContext::parse(const std::string &domain, int minPrecedence) {
    return registry_.parse(*this, domain, minPrecedence);
}

Parser::Parser(const ParserRegistry &registry, std::string startDomain)
    : registry_{registry}, startDomain_{std::move(startDomain)} {}

ast::NodePtr Parser::parse(std::vector<token::Token> tokens) const {
    ParserContext context{std::move(tokens), registry_};

    return context.parse(startDomain_);
}

} // namespace novac::parser