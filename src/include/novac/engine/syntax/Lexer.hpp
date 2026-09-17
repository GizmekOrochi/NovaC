#pragma once

#include "../foundation/registry/Registry.hpp"
#include "../syntax/Token.hpp"

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace novac::lexer {

/**
 * @brief Configures lexical analysis rules and token definitions.
 *
 * Stores registered keywords, symbols, identifier classification rules,
 * and comment delimiters used by a Lexer instance.
 */
class LexerRegistry {
public:
    using IdentifierStartPredicate = std::function<bool(unsigned char)>;
    using IdentifierContinuePredicate = std::function<bool(unsigned char)>;

    /**
     * @brief Creates a lexer registry.
     *
     * @param duplicatePolicy Policy used when duplicate registrations occur.
     */
    explicit LexerRegistry(registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    /**
     * @brief Registers a keyword.
     *
     * @param keyword Keyword text to register.
     * @return Registration result.
     *
     * @throws std::runtime_error If the keyword is empty.
     */
    registry::RegisterStatus keyword(std::string keyword);

    /**
     * @brief Registers a symbol token.
     *
     * Symbols are matched using longest-match-first ordering.
     *
     * @param symbol Symbol text to register.
     * @return Registration result.
     *
     * @throws std::runtime_error If the symbol is empty.
     */
    registry::RegisterStatus symbol(std::string symbol);

    /**
     * @brief Replaces the identifier classification rules.
     *
     * @param start Predicate used to validate the first identifier character.
     * @param continuation Predicate used to validate subsequent identifier characters.
     *
     * @throws std::runtime_error If either predicate is empty.
     */
    void setIdentifierRules(IdentifierStartPredicate start, IdentifierContinuePredicate continuation);

    /**
     * @brief Sets the line comment prefix.
     *
     * @param prefix Sequence that begins a line comment.
     */
    void setLineCommentPrefix(std::string prefix);

    /**
     * @brief Sets block comment delimiters.
     *
     * Both delimiters must be empty or both must be non-empty.
     *
     * @param begin Opening delimiter.
     * @param end Closing delimiter.
     *
     * @throws std::runtime_error If only one delimiter is empty.
     */
    void setBlockCommentDelimiters(std::string begin, std::string end);

    /**
     * @brief Determines whether a value is a registered keyword.
     *
     * @param value Text to test.
     * @return True if the value is a keyword.
     */
    bool isKeyword(const std::string &value) const;

    /**
     * @brief Tests whether a character may begin an identifier.
     *
     * @param value Character to test.
     * @return True if the character is a valid identifier start.
     */
    bool isIdentifierStart(unsigned char value) const;

    /**
     * @brief Tests whether a character may continue an identifier.
     *
     * @param value Character to test.
     * @return True if the character is a valid identifier continuation.
     */
    bool isIdentifierContinue(unsigned char value) const;

    /**
     * @brief Returns the registered symbols.
     *
     * @return Symbol list ordered by matching priority.
     */
    const std::vector<std::string> &symbols() const;

    /**
     * @brief Returns the configured line comment prefix.
     *
     * @return Line comment prefix.
     */
    const std::string &lineCommentPrefix() const;

    /**
     * @brief Returns the configured block comment opening delimiter.
     *
     * @return Opening delimiter.
     */
    const std::string &blockCommentBegin() const;

    /**
     * @brief Returns the configured block comment closing delimiter.
     *
     * @return Closing delimiter.
     */
    const std::string &blockCommentEnd() const;

private:
    std::unordered_set<std::string> keywords_;
    std::vector<std::string> symbols_;
    IdentifierStartPredicate identifierStart_;
    IdentifierContinuePredicate identifierContinue_;
    std::string lineCommentPrefix_;
    std::string blockCommentBegin_;
    std::string blockCommentEnd_;
    registry::DuplicatePolicy duplicatePolicy_;
};

/**
 * @brief Converts source text into a sequence of tokens.
 *
 * The lexer uses rules defined by a referenced LexerRegistry. The registry
 * must remain valid for the lifetime of the lexer.
 */
class Lexer {
public:
    /**
     * @brief Creates a lexer using the specified registry.
     *
     * @param registry Lexer configuration source.
     */
    explicit Lexer(const LexerRegistry &registry);

    /**
     * @brief Tokenizes source text.
     *
     * @param source Source text to tokenize.
     * @return Produced token stream terminated by an End token.
     */
    std::vector<token::Token> tokenize(const std::string &source);

    /**
     * @brief Tokenizes source text and associates tokens with a file name.
     *
     * @param source Source text to tokenize.
     * @param fileName Logical source file name used in diagnostics.
     * @return Produced token stream terminated by an End token.
     */
    std::vector<token::Token> tokenize(const std::string &source, std::string fileName);

private:
    const LexerRegistry &registry_;
};

} // namespace novac::lexer