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
 *
 * Associativity controls how operators with the same precedence are grouped
 * while parsing expressions.
 */
enum class Associativity {
    /** Groups operators from left to right, such as a - b - c. */
    Left,

    /** Groups operators from right to left, such as a = b = c. */
    Right,

    /** Prevents chaining operators with the same precedence. */
    None
};

/**
 * @brief Generic parsing callback used by regular rules and fallbacks.
 */
using ParseFn = std::function<ast::NodePtr(ParserContext &)>;

/**
 * @brief Callback used to parse the beginning of a Pratt expression.
 */
using PrefixFn = std::function<ast::NodePtr(ParserContext &)>;

/**
 * @brief Callback used to combine a left and right expression around an operator.
 */
using InfixFn = std::function<ast::NodePtr(ParserContext &, ast::NodePtr, const token::Token &, ast::NodePtr)>;

/**
 * @brief Callback used to apply an operator appearing after an expression.
 */
using PostfixFn = std::function<ast::NodePtr(ParserContext &, ast::NodePtr, const token::Token &)>;

/**
 * @brief Defines an infix operator rule.
 *
 * The precedence controls how strongly the operator binds while associativity
 * controls how operators at the same precedence are grouped.
 */
struct InfixRule {
    /** Binding strength of the operator. */
    int precedence{};

    /** Associativity used when parsing operators with equal precedence. */
    Associativity associativity{Associativity::Left};

    /** Function used to build the resulting AST node. */
    InfixFn fn{};
};

/**
 * @brief Defines a postfix operator rule.
 */
struct PostfixRule {
    /** Binding strength of the postfix operator. */
    int precedence{};

    /** Function used to transform the expression on the left. */
    PostfixFn fn{};
};

/**
 * @brief Collection of parsing rules belonging to a domain.
 *
 * A domain represents one parsing context, such as expressions, statements or
 * types. Each domain can contain direct rules, Pratt operators and fallback
 * rules.
 */
struct ParseDomain {
    /** Direct parsing rules selected from the current token. */
    std::unordered_map<std::string, ParseFn> rules{};

    /** Rules used to begin Pratt expressions. */
    std::unordered_map<std::string, PrefixFn> prefixes{};

    /** Infix operators available in this domain. */
    std::unordered_map<std::string, InfixRule> infixes{};

    /** Postfix operators available in this domain. */
    std::unordered_map<std::string, PostfixRule> postfixes{};

    /** Rules tried in order when no direct rule matches. */
    std::vector<ParseFn> fallbacks{};
};

/**
 * @brief Registry of parser rules and Pratt parser operators.
 *
 * ParserRegistry stores parsing behavior by domain. Language features can add
 * their own rules without modifying the parser core.
 *
 * When parsing starts, the registry first tries direct rules, then Pratt
 * prefixes when available, and finally fallback rules.
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

    /**
     * @brief Registers a direct parse rule.
     *
     * The rule is selected when the current token matches the provided key.
     *
     * @param domain Parse domain receiving the rule.
     * @param key Token key or token text matched by the rule.
     * @param fn Parsing function.
     * @return Registration result.
     */
    registry::RegisterStatus rule(std::string domain, std::string key, ParseFn fn);

    /**
     * @brief Registers a direct parse rule using a typed domain identifier.
     */
    registry::RegisterStatus rule(const ids::ParseDomain &domain, std::string key, ParseFn fn);

    /**
     * @brief Registers a fallback parse rule.
     *
     * Fallbacks are tried in registration order when no direct or Pratt rule
     * can parse the current token. Returning nullptr lets the next fallback run.
     *
     * @param domain Parse domain receiving the fallback.
     * @param fn Fallback parsing function.
     * @return Registration result.
     */
    registry::RegisterStatus fallback(std::string domain, ParseFn fn);

    /**
     * @brief Registers a fallback using a typed domain identifier.
     */
    registry::RegisterStatus fallback(const ids::ParseDomain &domain, ParseFn fn);

    /**
     * @brief Registers a Pratt prefix rule.
     *
     * Prefix rules are responsible for creating the first expression before
     * infix and postfix operators are processed.
     *
     * @param domain Parse domain receiving the rule.
     * @param key Token key or token text matched by the rule.
     * @param fn Prefix parsing function.
     * @return Registration result.
     */
    registry::RegisterStatus prefix(std::string domain, std::string key, PrefixFn fn);

    /**
     * @brief Registers a Pratt prefix rule using a typed domain identifier.
     */
    registry::RegisterStatus prefix(const ids::ParseDomain &domain, std::string key, PrefixFn fn);

    /**
     * @brief Registers a left-associative infix operator.
     *
     * This overload uses Associativity::Left by default.
     */
    registry::RegisterStatus infix(std::string domain, std::string op, int precedence, InfixFn fn);

    /**
     * @brief Registers an infix operator with explicit associativity.
     *
     * During Pratt parsing, the precedence decides whether the operator belongs
     * to the current expression or must be handled by an outer parse call.
     */
    registry::RegisterStatus infix(std::string domain, std::string op, int precedence, Associativity associativity, InfixFn fn);

    /**
     * @brief Registers a left-associative infix operator using a typed domain identifier.
     */
    registry::RegisterStatus infix(const ids::ParseDomain &domain, std::string op, int precedence, InfixFn fn);

    /**
     * @brief Registers an infix operator using a typed domain identifier.
     */
    registry::RegisterStatus infix(const ids::ParseDomain &domain, std::string op, int precedence, Associativity associativity, InfixFn fn);

    /**
     * @brief Registers a postfix operator rule.
     *
     * Postfix rules are applied after an expression has already been parsed on
     * the left, such as a function call or postfix increment.
     */
    registry::RegisterStatus postfix(std::string domain, std::string op, int precedence, PostfixFn fn);

    /**
     * @brief Registers a postfix operator using a typed domain identifier.
     */
    registry::RegisterStatus postfix(const ids::ParseDomain &domain, std::string op, int precedence, PostfixFn fn);

    /**
     * @brief Parses according to a registered domain.
     *
     * The current token is converted to a parser key. The registry first tries
     * direct rules, then Pratt parsing when a prefix rule matches, and finally
     * the registered fallback functions.
     *
     * @param context Parsing context.
     * @param domain Domain name.
     * @param minPrecedence Minimum accepted precedence.
     * @return Parsed AST node.
     *
     * @throws std::runtime_error If the domain does not exist or no rule matches.
     */
    ast::NodePtr parse(ParserContext &context, const std::string &domain, int minPrecedence = 0) const;

    /**
     * @brief Parses according to a registered domain.
     *
     * This overload forwards the typed identifier to the string-based parser.
     *
     * @param context Parsing context.
     * @param domain Domain identifier.
     * @param minPrecedence Minimum accepted precedence.
     * @return Parsed AST node.
     */
    ast::NodePtr parse(ParserContext &context, const ids::ParseDomain &domain, int minPrecedence = 0) const;

private:
    /**
     * @brief Converts a token to the generic key used by parser rules.
     *
     * Literal and identifier kinds use stable keys such as "$int" or
     * "$identifier". Symbols use their original text.
     *
     * @param token Token to convert.
     * @return Parser rule key.
     */
    static std::string tokenKey(const token::Token &token);

    /**
     * @brief Parses an expression using Pratt precedence rules.
     *
     * Parsing begins with a prefix rule. Postfix and infix operators are then
     * consumed while their precedence is high enough for the current parse.
     *
     * @param context Parsing context.
     * @param domain Active parse domain.
     * @param rules Rules belonging to the domain.
     * @param minPrecedence Minimum precedence accepted by this parse call.
     * @return Parsed expression node.
     */
    ast::NodePtr parsePratt(ParserContext &context, const std::string &domain, const ParseDomain &rules, int minPrecedence) const;

    std::unordered_map<std::string, ParseDomain> domains_;
    registry::DuplicatePolicy duplicatePolicy_;
};

/**
 * @brief Maintains parser state while consuming tokens.
 *
 * ParserContext owns the token sequence and tracks the current position. It
 * also provides helper functions for lookahead, consumption and recursive
 * parsing through the associated ParserRegistry.
 *
 * The context guarantees that the token stream always ends with an End token.
 */
class ParserContext {
public:
    /**
     * @brief Creates a parsing context.
     *
     * If the supplied token sequence is empty or does not already end with an
     * End token, one is added automatically.
     *
     * @param tokens Input token sequence.
     * @param registry Parser rule registry.
     */
    ParserContext(std::vector<token::Token> tokens, const ParserRegistry &registry);

    /**
     * @brief Returns the token currently being parsed.
     *
     * @return Current token.
     */
    const token::Token &cur() const;

    /**
     * @brief Returns a token at a lookahead offset.
     *
     * The parser position is not changed. If the requested position is beyond
     * the available tokens, the final End token is returned.
     *
     * @param offset Lookahead distance.
     * @return Requested token or the end token if out of range.
     */
    const token::Token &peek(std::size_t offset = 1) const;

    /**
     * @brief Checks whether parsing reached the end of the token stream.
     *
     * @return True when the current token is the End token.
     */
    bool end() const;

    /**
     * @brief Tests the current token text without consuming it.
     *
     * @param value Expected token text.
     * @return True if the token matches.
     */
    bool check(const std::string &value) const;

    /**
     * @brief Consumes and returns the current token.
     *
     * The parser position moves forward unless the current token is already the
     * final End token.
     *
     * @return Consumed token.
     */
    const token::Token &advance();

    /**
     * @brief Consumes a token with a specific text value.
     *
     * The token is checked before advancing the parser position.
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
     * The token kind is checked before advancing the parser position.
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
     * This forwards parsing to the associated ParserRegistry while keeping the
     * current token position in this context.
     *
     * @param domain Parse domain name.
     * @param minPrecedence Minimum accepted precedence.
     * @return Parsed AST node.
     */
    ast::NodePtr parse(const std::string &domain, int minPrecedence = 0);

    /**
     * @brief Parses using the specified typed domain identifier.
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
 * Parser owns no parsing rules itself. It keeps a reference to a configured
 * ParserRegistry and starts parsing from one selected domain.
 */
class Parser {
public:
    /**
     * @brief Creates a parser.
     *
     * @param registry Parser rule registry.
     * @param startDomain Initial parse domain.
     *
     * @throws std::runtime_error If the start domain is empty.
     */
    Parser(const ParserRegistry &registry, std::string startDomain);

    /**
     * @brief Creates a parser using a typed start domain identifier.
     *
     * @param registry Parser rule registry.
     * @param startDomain Initial parse domain.
     */
    Parser(const ParserRegistry &registry, const ids::ParseDomain &startDomain);

    /**
     * @brief Parses an entire token stream.
     *
     * A temporary ParserContext is created and parsing starts from the configured
     * start domain. The parse succeeds only if the resulting AST consumes the
     * complete token stream.
     *
     * @param tokens Input tokens.
     * @return Root AST node.
     *
     * @throws std::runtime_error If unconsumed tokens remain.
     */
    ast::NodePtr parse(std::vector<token::Token> tokens) const;

    /**
     * @brief Parses only the first matching part of a token stream.
     *
     * Unlike parse(), this function does not require parsing to reach the End
     * token and is useful when a caller intentionally wants a partial parse.
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