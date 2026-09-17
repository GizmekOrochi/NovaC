#pragma once

#include "../syntax/Node.hpp"
#include "../foundation/Ids.hpp"
#include "../foundation/registry/Registry.hpp"
#include "../foundation/registry/RegistryHelpers.hpp"
#include "../syntax/Token.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace novac::parser {

class ParserContext;

/**
 * @brief Operator associativity used by Pratt parsing rules.
 */
enum class Associativity {
    Left,
    Right,
    None
};

using ParseFn = std::function<ast::NodePtr(ParserContext &)>;
using PrefixFn = std::function<ast::NodePtr(ParserContext &)>;
using InfixFn = std::function<ast::NodePtr(ParserContext &, ast::NodePtr, const token::Token &, ast::NodePtr)>;
using PostfixFn = std::function<ast::NodePtr(ParserContext &, ast::NodePtr, const token::Token &)>;

/**
 * @brief Defines an infix operator rule.
 */
struct InfixRule {
    int precedence{};
    Associativity associativity{Associativity::Left};
    InfixFn fn{};
};

/**
 * @brief Defines a postfix operator rule.
 */
struct PostfixRule {
    int precedence{};
    PostfixFn fn{};
};

/**
 * @brief Collection of parsing rules belonging to a domain.
 */
struct ParseDomain {
    std::unordered_map<std::string, ParseFn> rules{};
    std::unordered_map<std::string, PrefixFn> prefixes{};
    std::unordered_map<std::string, InfixRule> infixes{};
    std::unordered_map<std::string, PostfixRule> postfixes{};
    std::vector<ParseFn> fallbacks{};
};

/**
 * @brief Registry of parser rules and Pratt parser operators.
 */
class ParserRegistry {
public:
    /**
     * @brief Creates a parser registry.
     *
     * @param duplicatePolicy Policy used when duplicate registrations occur.
     */
    explicit ParserRegistry(
        registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    /** @brief Registers a parse rule. */
    registry::RegisterStatus rule(std::string domain, std::string key, ParseFn fn);
    registry::RegisterStatus rule(const ids::ParseDomain &domain, std::string key, ParseFn fn);

    /** @brief Registers a fallback parse rule. */
    registry::RegisterStatus fallback(std::string domain, ParseFn fn);
    registry::RegisterStatus fallback(const ids::ParseDomain &domain, ParseFn fn);

    /** @brief Registers a prefix operator rule. */
    registry::RegisterStatus prefix(std::string domain, std::string key, PrefixFn fn);
    registry::RegisterStatus prefix(const ids::ParseDomain &domain, std::string key, PrefixFn fn);

    /** @brief Registers an infix operator rule. */
    registry::RegisterStatus infix(std::string domain, std::string op, int precedence, InfixFn fn);
    registry::RegisterStatus infix(std::string domain, std::string op, int precedence, Associativity associativity, InfixFn fn);
    registry::RegisterStatus infix(const ids::ParseDomain &domain, std::string op, int precedence, InfixFn fn);
    registry::RegisterStatus infix(const ids::ParseDomain &domain, std::string op, int precedence, Associativity associativity, InfixFn fn);

    /** @brief Registers a postfix operator rule. */
    registry::RegisterStatus postfix(std::string domain, std::string op, int precedence, PostfixFn fn);
    registry::RegisterStatus postfix(const ids::ParseDomain &domain, std::string op, int precedence, PostfixFn fn);

    /**
     * @brief Parses according to a registered domain.
     *
     * @param context Parsing context.
     * @param domain Domain name.
     * @param minPrecedence Minimum accepted precedence.
     * @return Parsed AST node.
     */
    ast::NodePtr parse(ParserContext &context, const std::string &domain, int minPrecedence = 0) const;

    /**
     * @brief Parses according to a registered domain.
     *
     * @param context Parsing context.
     * @param domain Domain identifier.
     * @param minPrecedence Minimum accepted precedence.
     * @return Parsed AST node.
     */
    ast::NodePtr parse(ParserContext &context, const ids::ParseDomain &domain, int minPrecedence = 0) const;

private:
    static std::string tokenKey(const token::Token &token);

    ast::NodePtr parsePratt(ParserContext &context, const std::string &domain, const ParseDomain &rules, int minPrecedence) const;

    std::unordered_map<std::string, ParseDomain> domains_;
    registry::DuplicatePolicy duplicatePolicy_;
};

/**
 * @brief Maintains parser state while consuming tokens.
 */
class ParserContext {
public:
    /**
     * @brief Creates a parsing context.
     *
     * @param tokens Input token sequence.
     * @param registry Parser rule registry.
     */
    ParserContext(std::vector<token::Token> tokens, const ParserRegistry &registry);

    /** @return Current token. */
    const token::Token &cur() const;

    /**
     * @brief Returns a token at a lookahead offset.
     *
     * @param offset Lookahead distance.
     * @return Requested token or the end token if out of range.
     */
    const token::Token &peek(std::size_t offset = 1) const;

    /** @return True when parsing reached the end token. */
    bool end() const;

    /**
     * @brief Tests the current token text.
     *
     * @param value Expected token text.
     * @return True if the token matches.
     */
    bool check(const std::string &value) const;

    /**
     * @brief Consumes and returns the current token.
     *
     * @return Consumed token.
     */
    const token::Token &advance();

    /**
     * @brief Consumes a token with a specific text value.
     *
     * @param value Expected token text.
     * @return Consumed token.
     *
     * @throws std::runtime_error If the current token does not match.
     */
    const token::Token &consume(const std::string &value);

    /**
     * @brief Consumes a token with a specific kind.
     *
     * @param kind Expected token kind.
     * @return Consumed token.
     *
     * @throws std::runtime_error If the current token kind does not match.
     */
    const token::Token &consumeKind(token::Kind kind);

    /**
     * @brief Parses using the specified domain.
     *
     * @param domain Parse domain name.
     * @param minPrecedence Minimum accepted precedence.
     * @return Parsed AST node.
     */
    ast::NodePtr parse(const std::string &domain, int minPrecedence = 0);

    /**
     * @brief Parses using the specified domain.
     *
     * @param domain Parse domain identifier.
     * @param minPrecedence Minimum accepted precedence.
     * @return Parsed AST node.
     */
    ast::NodePtr parse(const ids::ParseDomain &domain, int minPrecedence = 0);

private:
    std::vector<token::Token> tokens_;
    std::size_t pos_;
    const ParserRegistry &registry_;
};

/**
 * @brief High-level parser entry point.
 *
 * Parses token streams using a configured start domain.
 */
class Parser {
public:
    /**
     * @brief Creates a parser.
     *
     * @param registry Parser rule registry.
     * @param startDomain Initial parse domain.
     */
    Parser(const ParserRegistry &registry, std::string startDomain);

    /**
     * @brief Creates a parser.
     *
     * @param registry Parser rule registry.
     * @param startDomain Initial parse domain.
     */
    Parser(const ParserRegistry &registry, const ids::ParseDomain &startDomain);

    /**
     * @brief Parses an entire token stream.
     *
     * @param tokens Input tokens.
     * @return Root AST node.
     *
     * @throws std::runtime_error If unconsumed tokens remain.
     */
    ast::NodePtr parse(std::vector<token::Token> tokens) const;

    /**
     * @brief Parses a prefix of a token stream.
     *
     * @param tokens Input tokens.
     * @return Parsed AST node.
     */
    ast::NodePtr parsePartial(std::vector<token::Token> tokens) const;

private:
    const ParserRegistry &registry_;
    std::string startDomain_;
};

} // namespace novac::parser