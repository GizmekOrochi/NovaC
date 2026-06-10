#include "../../../../include/novac/engine/syntax/Parser.hpp"

#include <stdexcept>
#include <utility>

namespace novac::parser {

ParserRegistry::ParserRegistry(registry::DuplicatePolicy duplicatePolicy)
    : domains_{}, duplicatePolicy_{duplicatePolicy} {}

registry::RegisterStatus ParserRegistry::rule(std::string domain, std::string key, ParseFn fn) {
    return registry::registerEntry(domains_[std::move(domain)].rules, std::move(key), std::move(fn), duplicatePolicy_, "ParserRegistry::rule");
}

registry::RegisterStatus ParserRegistry::rule(const ids::ParseDomain &domain, std::string key, ParseFn fn) {
    return rule(domain.value, std::move(key), std::move(fn));
}

registry::RegisterStatus ParserRegistry::fallback(std::string domain, ParseFn fn) {
    domains_[std::move(domain)].fallbacks.push_back(std::move(fn));

    return registry::RegisterStatus::Inserted;
}

registry::RegisterStatus ParserRegistry::fallback(const ids::ParseDomain &domain, ParseFn fn) {
    return fallback(domain.value, std::move(fn));
}

registry::RegisterStatus ParserRegistry::prefix(std::string domain, std::string key, PrefixFn fn) {
    return registry::registerEntry(domains_[std::move(domain)].prefixes, std::move(key), std::move(fn), duplicatePolicy_, "ParserRegistry::prefix");
}

registry::RegisterStatus ParserRegistry::prefix(const ids::ParseDomain &domain, std::string key, PrefixFn fn) {
    return prefix(domain.value, std::move(key), std::move(fn));
}

registry::RegisterStatus ParserRegistry::infix(std::string domain, std::string op, int precedence, InfixFn fn) {
    return registry::registerEntry(domains_[std::move(domain)].infixes, std::move(op), InfixRule{precedence, std::move(fn)}, duplicatePolicy_, "ParserRegistry::infix");
}

registry::RegisterStatus ParserRegistry::infix(const ids::ParseDomain &domain, std::string op, int precedence, InfixFn fn) {
    return infix(domain.value, std::move(op), precedence, std::move(fn));
}

registry::RegisterStatus ParserRegistry::postfix(std::string domain, std::string op, int precedence, PostfixFn fn) {
    return registry::registerEntry(domains_[std::move(domain)].postfixes, std::move(op), PostfixRule{precedence, std::move(fn)}, duplicatePolicy_, "ParserRegistry::postfix");
}

registry::RegisterStatus ParserRegistry::postfix(const ids::ParseDomain &domain, std::string op, int precedence, PostfixFn fn) {
    return postfix(domain.value, std::move(op), precedence, std::move(fn));
}

ast::NodePtr ParserRegistry::parse(ParserContext &context, const std::string &domain, int minPrecedence) const {
    const auto domainIter{domains_.find(domain)};

    if (domainIter == domains_.end())
        throw std::runtime_error("ParserRegistry::parse: unknown parse domain '" + domain + "'");

    const ParseDomain &rules{domainIter->second};
    const std::string key{tokenKey(context.cur())};

    const auto ruleIter{rules.rules.find(key)};

    if (ruleIter != rules.rules.end())
        return ruleIter->second(context);

    const auto textRuleIter{rules.rules.find(context.cur().text)};

    if (textRuleIter != rules.rules.end())
        return textRuleIter->second(context);

    if (!rules.prefixes.empty()) {
        const auto prefixIter{rules.prefixes.find(key)};

        if (prefixIter != rules.prefixes.end())
            return parsePratt(context, domain, rules, minPrecedence);
    }

    for (const ParseFn &fallback : rules.fallbacks)
        if (ast::NodePtr node{fallback(context)})
            return node;

    throw std::runtime_error("ParserRegistry::parse: no rule matched domain '" + domain + "'");
}

ast::NodePtr ParserRegistry::parse(ParserContext &context, const ids::ParseDomain &domain, int minPrecedence) const {
    return parse(context, domain.value, minPrecedence);
}

ast::NodePtr ParserRegistry::parsePratt(ParserContext &context, const std::string &domain, const ParseDomain &rules, int minPrecedence) const {
    const auto prefixIter{rules.prefixes.find(tokenKey(context.cur()))};

    if (prefixIter == rules.prefixes.end())
        throw std::runtime_error("ParserRegistry::parsePratt: expected expression in domain '" + domain + "'");

    ast::NodePtr left{prefixIter->second(context)};

    while (!context.end()) {
        const std::string op{context.cur().text};
        const auto postfixIter{rules.postfixes.find(op)};

        if (postfixIter != rules.postfixes.end() && postfixIter->second.precedence >= minPrecedence) {
            left = postfixIter->second.fn(context, left);
            continue;
        }

        const auto infixIter{rules.infixes.find(op)};

        if (infixIter == rules.infixes.end() || infixIter->second.precedence < minPrecedence)
            break;

        left = infixIter->second.fn(context, left);
    }

    return left;
}

std::string ParserRegistry::tokenKey(const token::Token &token) {
    if (token.kind == token::Kind::Integer) return "$int";
    if (token.kind == token::Kind::Identifier) return "$identifier";
    if (token.kind == token::Kind::Keyword) return "$keyword";
    if (token.kind == token::Kind::End) return "$end";

    return token.text;
}

ParserContext::ParserContext(std::vector<token::Token> tokens, const ParserRegistry &registry)
    : tokens_{std::move(tokens)}, pos_{}, registry_{registry} {}

const token::Token &ParserContext::cur() const {
    return tokens_[pos_];
}

const token::Token &ParserContext::peek(std::size_t offset) const {
    const std::size_t index{pos_ + offset};

    if (index >= tokens_.size())
        return tokens_.back();

    return tokens_[index];
}

bool ParserContext::end() const {
    return cur().kind == token::Kind::End;
}

bool ParserContext::check(const std::string &value) const {
    return cur().text == value;
}

const token::Token &ParserContext::advance() {
    if (!end())
        ++pos_;

    return tokens_[pos_ - 1];
}

const token::Token &ParserContext::consume(const std::string &value) {
    if (!check(value))
        throw std::runtime_error("ParserContext::consume: expected '" + value + "'");

    return advance();
}

const token::Token &ParserContext::consumeKind(token::Kind kind) {
    if (cur().kind != kind)
        throw std::runtime_error("ParserContext::consumeKind: unexpected token");

    return advance();
}

ast::NodePtr ParserContext::parse(const std::string &domain, int minPrecedence) {
    return registry_.parse(*this, domain, minPrecedence);
}

ast::NodePtr ParserContext::parse(const ids::ParseDomain &domain, int minPrecedence) {
    return parse(domain.value, minPrecedence);
}

Parser::Parser(const ParserRegistry &registry, std::string startDomain)
    : registry_{registry}, startDomain_{std::move(startDomain)} {}

Parser::Parser(const ParserRegistry &registry, const ids::ParseDomain &startDomain)
    : Parser{registry, startDomain.value} {}

ast::NodePtr Parser::parse(std::vector<token::Token> tokens) const {
    ParserContext context{std::move(tokens), registry_};

    return context.parse(startDomain_);
}

} // namespace novac::parser