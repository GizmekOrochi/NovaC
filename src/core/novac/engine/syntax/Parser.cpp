#include "novac/engine/syntax/Parser.hpp"

#include <stdexcept>
#include <utility>

namespace novac::parser {

/**
 * @brief Constructs a `ParserRegistry` instance.
 *
 * @param duplicatePolicy Value supplied for `duplicatePolicy`.
 */
ParserRegistry::ParserRegistry(registry::DuplicatePolicy duplicatePolicy)
    : domains_{}, duplicatePolicy_{duplicatePolicy} {}

/**
 * @brief Implements the `rule` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param key Value supplied for `key`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
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

/**
 * @brief Implements the `rule` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param key Value supplied for `key`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus ParserRegistry::rule(const ids::ParseDomain &domain, std::string key, ParseFn fn) {
    return rule(domain.value, std::move(key), std::move(fn));
}

/**
 * @brief Implements the `fallback` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
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

/**
 * @brief Implements the `fallback` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus ParserRegistry::fallback(const ids::ParseDomain &domain, ParseFn fn) {
    return fallback(domain.value, std::move(fn));
}

/**
 * @brief Implements the `prefix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param key Value supplied for `key`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
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

/**
 * @brief Implements the `prefix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param key Value supplied for `key`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus ParserRegistry::prefix(const ids::ParseDomain &domain, std::string key, PrefixFn fn) {
    return prefix(domain.value, std::move(key), std::move(fn));
}

/**
 * @brief Implements the `prefixFallback` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param key Value supplied for `key`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
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

/**
 * @brief Implements the `prefixFallback` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param key Value supplied for `key`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus ParserRegistry::prefixFallback(const ids::ParseDomain &domain, std::string key, PrefixFn fn) {
    return prefixFallback(domain.value, std::move(key), std::move(fn));
}

/**
 * @brief Implements the `infix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param op Value supplied for `op`.
 * @param precedence Value supplied for `precedence`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus ParserRegistry::infix(std::string domain, std::string op, int precedence, InfixFn fn) {
    return infix(std::move(domain), std::move(op), precedence, Associativity::Left, std::move(fn));
}

/**
 * @brief Implements the `infix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param op Value supplied for `op`.
 * @param precedence Value supplied for `precedence`.
 * @param associativity Value supplied for `associativity`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
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

/**
 * @brief Implements the `infix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param op Value supplied for `op`.
 * @param precedence Value supplied for `precedence`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus ParserRegistry::infix(const ids::ParseDomain &domain, std::string op, int precedence, InfixFn fn) {
    return infix(domain.value, std::move(op), precedence, std::move(fn));
}

/**
 * @brief Implements the `infix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param op Value supplied for `op`.
 * @param precedence Value supplied for `precedence`.
 * @param associativity Value supplied for `associativity`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus ParserRegistry::infix(const ids::ParseDomain &domain, std::string op, int precedence, Associativity associativity, InfixFn fn) {
    return infix(domain.value, std::move(op), precedence, associativity, std::move(fn));
}

/**
 * @brief Implements the `postfix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param op Value supplied for `op`.
 * @param precedence Value supplied for `precedence`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
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

/**
 * @brief Implements the `postfix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param op Value supplied for `op`.
 * @param precedence Value supplied for `precedence`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus ParserRegistry::postfix(const ids::ParseDomain &domain, std::string op, int precedence, PostfixFn fn) {
    return postfix(domain.value, std::move(op), precedence, std::move(fn));
}

/**
 * @brief Parses input through `parse`.
 *
 * @param context Value supplied for `context`.
 * @param domain Value supplied for `domain`.
 * @param minPrecedence Value supplied for `minPrecedence`.
 * @return Value produced by the operation.
 */
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

        if (prefixIter != rules.prefixes.end() ||
            textPrefixIter != rules.prefixes.end() ||
            fallbackIter != rules.prefixFallbacks.end() ||
            textFallbackIter != rules.prefixFallbacks.end()) {
            if (ast::NodePtr node{parsePratt(context, domain, rules, minPrecedence)}) {
                return node;
            }
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

/**
 * @brief Parses input through `parse`.
 *
 * @param context Value supplied for `context`.
 * @param domain Value supplied for `domain`.
 * @param minPrecedence Value supplied for `minPrecedence`.
 * @return Value produced by the operation.
 */
ast::NodePtr ParserRegistry::parse(ParserContext &context, const ids::ParseDomain &domain, int minPrecedence) const {
    return parse(context, domain.value, minPrecedence);
}

/**
 * @brief Parses input through `parsePratt`.
 *
 * @param context Value supplied for `context`.
 * @param domain Value supplied for `domain`.
 * @param rules Value supplied for `rules`.
 * @param minPrecedence Value supplied for `minPrecedence`.
 * @return Value produced by the operation.
 */
ast::NodePtr ParserRegistry::parsePratt(ParserContext &context, const std::string &domain, const ParseDomain &rules, int minPrecedence) const {
    const std::string prefixKey{tokenKey(context.cur())};
    const std::string prefixText{context.cur().text};

    auto prefixIter{rules.prefixes.find(prefixKey)};

    if (prefixIter == rules.prefixes.end()) {
        prefixIter = rules.prefixes.find(prefixText);
    }

    ast::NodePtr left{};

    auto tryPrefixFallbacks = [&](const std::string &candidateKey) {
        const auto fallbackIter{rules.prefixFallbacks.find(candidateKey)};

        if (fallbackIter == rules.prefixFallbacks.end()) {
            return false;
        }

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

    bool matchedFallback{tryPrefixFallbacks(prefixKey)};

    if (!matchedFallback && prefixText != prefixKey) {
        matchedFallback = tryPrefixFallbacks(prefixText);
    }

    if (!matchedFallback) {
        if (prefixIter == rules.prefixes.end()) {
            return nullptr;
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

        if (infixIter->second.associativity == Associativity::None &&
            infixIter->second.precedence == minPrecedence) {
            break;
        }

        const InfixRule rule{infixIter->second};
        const token::Token opToken{context.advance()};
        const int nextMinPrecedence{
            rule.associativity == Associativity::Left ? rule.precedence + 1 : rule.precedence};
        ast::NodePtr right{parse(context, domain, nextMinPrecedence)};

        left = rule.fn(context, left, opToken, right);
    }

    return left;
}

/**
 * @brief Implements the `tokenKey` operation.
 *
 * @param token Value supplied for `token`.
 * @return Value produced by the operation.
 */
std::string ParserRegistry::tokenKey(const token::Token &token) {
    if (!token.tag.empty()) {
        return token.tag;
    }

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

/**
 * @brief Constructs a `ParserContext` instance.
 *
 * @param tokens Value supplied for `tokens`.
 * @param registry Value supplied for `registry`.
 */
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

/**
 * @brief Implements the `cur` operation.
 *
 * @return Value produced by the operation.
 */
const token::Token &ParserContext::cur() const {
    return tokens_[pos_];
}

/**
 * @brief Implements the `peek` operation.
 *
 * @param offset Value supplied for `offset`.
 * @return Value produced by the operation.
 */
const token::Token &ParserContext::peek(std::size_t offset) const {
    const std::size_t index{pos_ + offset};

    if (index >= tokens_.size()) {
        return tokens_.back();
    }

    return tokens_[index];
}

/**
 * @brief Completes the operation represented by `end`.
 *
 * @return Value produced by the operation.
 */
bool ParserContext::end() const {
    return cur().kind == token::Kind::End;
}

/**
 * @brief Implements the `check` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
bool ParserContext::check(const std::string &value) const {
    return cur().text == value;
}

/**
 * @brief Implements the `advance` operation.
 *
 * @return Value produced by the operation.
 */
const token::Token &ParserContext::advance() {
    if (end()) {
        return cur();
    }

    const token::Token &current{tokens_[pos_]};
    ++pos_;
    return current;
}

/**
 * @brief Implements the `consume` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
const token::Token &ParserContext::consume(const std::string &value) {
    if (!check(value)) {
        throw std::runtime_error("ParserContext::consume: expected '" + value + "', got '" + cur().text + "' at line " + std::to_string(cur().line) + ", column " + std::to_string(cur().column));
    }

    return advance();
}

/**
 * @brief Implements the `consumeKind` operation.
 *
 * @param kind Value supplied for `kind`.
 * @return Value produced by the operation.
 */
const token::Token &ParserContext::consumeKind(token::Kind kind) {
    if (cur().kind != kind) {
        throw std::runtime_error("ParserContext::consumeKind: unexpected token '" + cur().text + "' at line " + std::to_string(cur().line) + ", column " + std::to_string(cur().column));
    }

    return advance();
}

/**
 * @brief Parses input through `parse`.
 *
 * @param domain Value supplied for `domain`.
 * @param minPrecedence Value supplied for `minPrecedence`.
 * @return Value produced by the operation.
 */
ast::NodePtr ParserContext::parse(const std::string &domain, int minPrecedence) {
    return registry_.parse(*this, domain, minPrecedence);
}

/**
 * @brief Parses input through `parse`.
 *
 * @param domain Value supplied for `domain`.
 * @param minPrecedence Value supplied for `minPrecedence`.
 * @return Value produced by the operation.
 */
ast::NodePtr ParserContext::parse(const ids::ParseDomain &domain, int minPrecedence) {
    return parse(domain.value, minPrecedence);
}

/**
 * @brief Constructs a `Parser` instance.
 *
 * @param registry Value supplied for `registry`.
 * @param startDomain Value supplied for `startDomain`.
 */
Parser::Parser(const ParserRegistry &registry, std::string startDomain)
    : registry_{registry}, startDomain_{std::move(startDomain)} {
    if (startDomain_.empty()) {
        throw std::runtime_error("Parser::Parser: start domain cannot be empty");
    }
}

/**
 * @brief Constructs a `Parser` instance.
 *
 * @param registry Value supplied for `registry`.
 * @param startDomain Value supplied for `startDomain`.
 */
Parser::Parser(const ParserRegistry &registry, const ids::ParseDomain &startDomain)
    : Parser{registry, startDomain.value} {}

/**
 * @brief Parses input through `parse`.
 *
 * @param tokens Value supplied for `tokens`.
 * @return Value produced by the operation.
 */
ast::NodePtr Parser::parse(std::vector<token::Token> tokens) const {
    ParserContext context{std::move(tokens), registry_};
    ast::NodePtr root{context.parse(startDomain_)};

    if (!context.end()) {
        throw std::runtime_error("Parser::parse: unexpected token '" + context.cur().text + "' after complete parse");
    }

    return root;
}

/**
 * @brief Parses input through `parsePartial`.
 *
 * @param tokens Value supplied for `tokens`.
 * @return Value produced by the operation.
 */
ast::NodePtr Parser::parsePartial(std::vector<token::Token> tokens) const {
    ParserContext context{std::move(tokens), registry_};

    return context.parse(startDomain_);
}

} // namespace novac::parser
