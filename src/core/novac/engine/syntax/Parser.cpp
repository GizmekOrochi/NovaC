#include "novac/engine/syntax/Parser.hpp"

#include <stdexcept>
#include <utility>

namespace novac::parser {

ParserRegistry::ParserRegistry(registry::DuplicatePolicy duplicatePolicy)
    : domains_{}, duplicatePolicy_{duplicatePolicy} {}

registry::RegisterStatus ParserRegistry::rule(std::string domain, std::string key, ParseFn fn) {
    if (domain.empty()) {
        throw std::runtime_error("ParserRegistry::rule: domain cannot be empty");
    }

    if (key.empty()) {
        throw std::runtime_error("ParserRegistry::rule: key cannot be empty");
    }

    if (!fn) {
        throw std::runtime_error("ParserRegistry::rule: parse function cannot be empty");
    }

    return registry::registerEntry(domains_[std::move(domain)].rules, std::move(key), std::move(fn), duplicatePolicy_, "ParserRegistry::rule");
}

registry::RegisterStatus ParserRegistry::rule(const ids::ParseDomain &domain, std::string key, ParseFn fn) {
    return rule(domain.value, std::move(key), std::move(fn));
}

registry::RegisterStatus ParserRegistry::fallback(std::string domain, ParseFn fn) {
    if (domain.empty()) {
        throw std::runtime_error("ParserRegistry::fallback: domain cannot be empty");
    }

    if (!fn) {
        throw std::runtime_error("ParserRegistry::fallback: parse function cannot be empty");
    }

    domains_[std::move(domain)].fallbacks.push_back(std::move(fn));

    return registry::RegisterStatus::Inserted;
}

registry::RegisterStatus ParserRegistry::fallback(const ids::ParseDomain &domain, ParseFn fn) {
    return fallback(domain.value, std::move(fn));
}

registry::RegisterStatus ParserRegistry::prefix(std::string domain, std::string key, PrefixFn fn) {
    if (domain.empty()) {
        throw std::runtime_error("ParserRegistry::prefix: domain cannot be empty");
    }

    if (key.empty()) {
        throw std::runtime_error("ParserRegistry::prefix: key cannot be empty");
    }

    if (!fn) {
        throw std::runtime_error("ParserRegistry::prefix: prefix function cannot be empty");
    }

    return registry::registerEntry(domains_[std::move(domain)].prefixes, std::move(key), std::move(fn), duplicatePolicy_, "ParserRegistry::prefix");
}

registry::RegisterStatus ParserRegistry::prefix(const ids::ParseDomain &domain, std::string key, PrefixFn fn) {
    return prefix(domain.value, std::move(key), std::move(fn));
}

registry::RegisterStatus ParserRegistry::prefixFallback(std::string domain, std::string key, PrefixFn fn) {
    if (domain.empty()) {
        throw std::runtime_error("ParserRegistry::prefixFallback: domain cannot be empty");
    }

    if (key.empty()) {
        throw std::runtime_error("ParserRegistry::prefixFallback: key cannot be empty");
    }

    if (!fn) {
        throw std::runtime_error("ParserRegistry::prefixFallback: prefix function cannot be empty");
    }

    domains_[std::move(domain)].prefixFallbacks[std::move(key)].push_back(std::move(fn));
    return registry::RegisterStatus::Inserted;
}

registry::RegisterStatus ParserRegistry::prefixFallback(const ids::ParseDomain &domain, std::string key, PrefixFn fn) {
    return prefixFallback(domain.value, std::move(key), std::move(fn));
}

registry::RegisterStatus ParserRegistry::infix(std::string domain, std::string op, int precedence, InfixFn fn) {
    return infix(std::move(domain), std::move(op), precedence, Associativity::Left, std::move(fn));
}

registry::RegisterStatus ParserRegistry::infix(std::string domain, std::string op, int precedence, Associativity associativity, InfixFn fn) {
    if (domain.empty()) {
        throw std::runtime_error("ParserRegistry::infix: domain cannot be empty");
    }

    if (op.empty()) {
        throw std::runtime_error("ParserRegistry::infix: operator cannot be empty");
    }

    if (!fn) {
        throw std::runtime_error("ParserRegistry::infix: infix function cannot be empty");
    }

    return registry::registerEntry(
        domains_[std::move(domain)].infixes,
        std::move(op),
        InfixRule{precedence, associativity, std::move(fn)},
        duplicatePolicy_,
        "ParserRegistry::infix");
}

registry::RegisterStatus ParserRegistry::infix(const ids::ParseDomain &domain, std::string op, int precedence, InfixFn fn) {
    return infix(domain.value, std::move(op), precedence, std::move(fn));
}

registry::RegisterStatus ParserRegistry::infix(const ids::ParseDomain &domain, std::string op, int precedence, Associativity associativity, InfixFn fn) {
    return infix(domain.value, std::move(op), precedence, associativity, std::move(fn));
}

registry::RegisterStatus ParserRegistry::postfix(std::string domain, std::string op, int precedence, PostfixFn fn) {
    if (domain.empty()) {
        throw std::runtime_error("ParserRegistry::postfix: domain cannot be empty");
    }

    if (op.empty()) {
        throw std::runtime_error("ParserRegistry::postfix: operator cannot be empty");
    }

    if (!fn) {
        throw std::runtime_error("ParserRegistry::postfix: postfix function cannot be empty");
    }

    return registry::registerEntry(
        domains_[std::move(domain)].postfixes,
        std::move(op),
        PostfixRule{precedence, std::move(fn)},
        duplicatePolicy_,
        "ParserRegistry::postfix");
}

registry::RegisterStatus ParserRegistry::postfix(const ids::ParseDomain &domain, std::string op, int precedence, PostfixFn fn) {
    return postfix(domain.value, std::move(op), precedence, std::move(fn));
}

ast::NodePtr ParserRegistry::parse(ParserContext &context, const std::string &domain, int minPrecedence) const {
    const auto domainIter{domains_.find(domain)};

    if (domainIter == domains_.end()) {
        throw std::runtime_error("ParserRegistry::parse: unknown parse domain '" + domain + "'");
    }

    const ParseDomain &rules{domainIter->second};
    const std::string key{tokenKey(context.cur())};

    const auto ruleIter{rules.rules.find(key)};

    if (ruleIter != rules.rules.end()) {
        return ruleIter->second(context);
    }

    const auto textRuleIter{rules.rules.find(context.cur().text)};

    if (textRuleIter != rules.rules.end()) {
        return textRuleIter->second(context);
    }

    if (!rules.prefixes.empty() || !rules.prefixFallbacks.empty()) {
        const auto prefixIter{rules.prefixes.find(key)};
        const auto textPrefixIter{rules.prefixes.find(context.cur().text)};
        const auto fallbackIter{rules.prefixFallbacks.find(key)};
        const auto textFallbackIter{rules.prefixFallbacks.find(context.cur().text)};

        if (prefixIter != rules.prefixes.end() || textPrefixIter != rules.prefixes.end() ||
            fallbackIter != rules.prefixFallbacks.end() || textFallbackIter != rules.prefixFallbacks.end()) {
            return parsePratt(context, domain, rules, minPrecedence);
        }
    }

    for (const ParseFn &fallback : rules.fallbacks) {
        const std::size_t fallbackStart{context.pos_};

        if (ast::NodePtr node{fallback(context)}) {
            return node;
        }

        context.pos_ = fallbackStart;
    }

    throw std::runtime_error("ParserRegistry::parse: no rule matched domain '" + domain + "' at line " + std::to_string(context.cur().line) + ", column " + std::to_string(context.cur().column));
}

ast::NodePtr ParserRegistry::parse(ParserContext &context, const ids::ParseDomain &domain, int minPrecedence) const {
    return parse(context, domain.value, minPrecedence);
}

ast::NodePtr ParserRegistry::parsePratt(ParserContext &context, const std::string &domain, const ParseDomain &rules, int minPrecedence) const {
    const std::string prefixKey{tokenKey(context.cur())};
    auto prefixIter{rules.prefixes.find(prefixKey)};

    if (prefixIter == rules.prefixes.end()) {
        prefixIter = rules.prefixes.find(context.cur().text);
    }

    ast::NodePtr left{};

    auto tryPrefixFallbacks = [&](const std::string &candidateKey) {
        const auto fallbackIter{rules.prefixFallbacks.find(candidateKey)};
        if (fallbackIter == rules.prefixFallbacks.end())
            return false;

        for (const PrefixFn &fallback : fallbackIter->second) {
            const std::size_t fallbackStart{context.pos_};

            if (ast::NodePtr node{fallback(context)}) {
                left = std::move(node);
                return true;
            }

            context.pos_ = fallbackStart;
        }

        return false;
    };

    const std::string tokenText{context.cur().text};
    if (!tryPrefixFallbacks(prefixKey) && !tryPrefixFallbacks(tokenText)) {
        if (prefixIter == rules.prefixes.end()) {
            throw std::runtime_error("ParserRegistry::parsePratt: expected expression in domain '" + domain + "' at line " + std::to_string(context.cur().line) + ", column " + std::to_string(context.cur().column));
        }

        left = prefixIter->second(context);
    }

    while (!context.end()) {
        const std::string op{context.cur().text};
        const auto postfixIter{rules.postfixes.find(op)};

        if (postfixIter != rules.postfixes.end() && postfixIter->second.precedence >= minPrecedence) {
            const token::Token opToken{context.advance()};
            left = postfixIter->second.fn(context, left, opToken);
            continue;
        }

        const auto infixIter{rules.infixes.find(op)};

        if (infixIter == rules.infixes.end() || infixIter->second.precedence < minPrecedence) {
            break;
        }

        if (infixIter->second.associativity == Associativity::None && infixIter->second.precedence == minPrecedence) {
            break;
        }

        const InfixRule rule{infixIter->second};
        const token::Token opToken{context.advance()};
        const int nextMinPrecedence{rule.associativity == Associativity::Left ? rule.precedence + 1 : rule.precedence};
        ast::NodePtr right{parse(context, domain, nextMinPrecedence)};

        left = rule.fn(context, left, opToken, right);
    }

    return left;
}

std::string ParserRegistry::tokenKey(const token::Token &token) {
    if (token.kind == token::Kind::Integer) {
        return "$int";
    }

    if (token.kind == token::Kind::Float) {
        return "$float";
    }

    if (token.kind == token::Kind::String) {
        return "$string";
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

ParserContext::ParserContext(std::vector<token::Token> tokens, const ParserRegistry &registry)
    : tokens_{std::move(tokens)}, pos_{}, registry_{registry} {
    if (tokens_.empty()) {
        tokens_.push_back({token::Kind::End, "", "", {}, 1, 1});
        return;
    }

    if (tokens_.back().kind != token::Kind::End) {
        const token::Token &last{tokens_.back()};

        tokens_.push_back({token::Kind::End, "", "", {}, last.line, last.column});
    }
}

const token::Token &ParserContext::cur() const {
    return tokens_[pos_];
}

const token::Token &ParserContext::peek(std::size_t offset) const {
    const std::size_t index{pos_ + offset};

    if (index >= tokens_.size()) {
        return tokens_.back();
    }

    return tokens_[index];
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
        throw std::runtime_error("ParserContext::consume: expected '" + value + "', got '" + cur().text + "' at line " + std::to_string(cur().line) + ", column " + std::to_string(cur().column));
    }

    return advance();
}

const token::Token &ParserContext::consumeKind(token::Kind kind) {
    if (cur().kind != kind) {
        throw std::runtime_error("ParserContext::consumeKind: unexpected token '" + cur().text + "' at line " + std::to_string(cur().line) + ", column " + std::to_string(cur().column));
    }

    return advance();
}

ast::NodePtr ParserContext::parse(const std::string &domain, int minPrecedence) {
    return registry_.parse(*this, domain, minPrecedence);
}

ast::NodePtr ParserContext::parse(const ids::ParseDomain &domain, int minPrecedence) {
    return parse(domain.value, minPrecedence);
}

Parser::Parser(const ParserRegistry &registry, std::string startDomain)
    : registry_{registry}, startDomain_{std::move(startDomain)} {
    if (startDomain_.empty()) {
        throw std::runtime_error("Parser::Parser: start domain cannot be empty");
    }
}

Parser::Parser(const ParserRegistry &registry, const ids::ParseDomain &startDomain)
    : Parser{registry, startDomain.value} {}

ast::NodePtr Parser::parse(std::vector<token::Token> tokens) const {
    ParserContext context{std::move(tokens), registry_};
    ast::NodePtr root{context.parse(startDomain_)};

    if (!context.end()) {
        throw std::runtime_error("Parser::parse: unexpected token '" + context.cur().text + "' after complete parse");
    }

    return root;
}

ast::NodePtr Parser::parsePartial(std::vector<token::Token> tokens) const {
    ParserContext context{std::move(tokens), registry_};

    return context.parse(startDomain_);
}

} // namespace novac::parser