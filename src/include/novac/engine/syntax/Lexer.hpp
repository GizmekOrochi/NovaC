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
 * LexerRegistry contains the language-specific parts of lexical analysis:
 * keywords, symbols, identifier rules and comment delimiters.
 *
 * A Lexer reads this registry while tokenizing source code, which allows
 * different languages to reuse the same lexer implementation with different
 * lexical conventions.
 */
class LexerRegistry {
public:
    /**
     * @brief Predicate used to decide whether a character can begin an identifier.
     */
    using IdentifierStartPredicate = std::function<bool(unsigned char)>;

    /**
     * @brief Predicate used to decide whether a character can continue an identifier.
     */
    using IdentifierContinuePredicate = std::function<bool(unsigned char)>;

    /**
     * @brief Creates a lexer registry.
     *
     * The registry starts with default identifier rules compatible with common
     * C-like identifiers and default // and /* ... *\/ comment delimiters.
     *
     * @param duplicatePolicy Policy used when duplicate registrations occur.
     */
    explicit LexerRegistry(registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    /**
     * @brief Registers a keyword.
     *
     * Registered keywords are emitted as token::Kind::Keyword instead of
     * token::Kind::Identifier when the lexer reads matching identifier text.
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
     * Symbols are automatically reordered so longer symbols are tested before
     * shorter ones. This prevents a symbol such as "==" from being split into
     * two "=" tokens when both symbols are registered.
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
     * The first predicate decides which characters can begin identifiers. The
     * second decides which characters can appear after the first character.
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
     * When this sequence is encountered, the lexer skips characters until the
     * next newline. An empty prefix disables line comments.
     *
     * @param prefix Sequence that begins a line comment.
     */
    void setLineCommentPrefix(std::string prefix);

    /**
     * @brief Sets block comment delimiters.
     *
     * When the opening delimiter is encountered, input is skipped until the
     * closing delimiter is found. Both delimiters can be empty to disable
     * block comments.
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
     * The configured identifier-start predicate is used directly.
     *
     * @param value Character to test.
     * @return True if the character is a valid identifier start.
     */
    bool isIdentifierStart(unsigned char value) const;

    /**
     * @brief Tests whether a character may continue an identifier.
     *
     * The configured identifier-continuation predicate is used directly.
     *
     * @param value Character to test.
     * @return True if the character is a valid identifier continuation.
     */
    bool isIdentifierContinue(unsigned char value) const;

    /**
     * @brief Returns the registered symbols.
     *
     * The list is kept in longest-match-first order so Lexer can simply test
     * symbols from beginning to end.
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
 * Lexer performs the actual source scan using configuration stored in a
 * LexerRegistry. It recognizes whitespace, comments, identifiers, keywords,
 * numeric literals, string literals and registered symbols.
 *
 * Source positions are tracked while scanning so every produced token contains
 * line, column and SourceSpan information.
 *
 * The registry is referenced by the lexer and must remain valid for the
 * lifetime of the Lexer instance.
 */
class Lexer {
public:
    /**
     * @brief Creates a lexer using the specified registry.
     *
     * The lexer keeps a reference to the registry instead of copying it, so
     * later tokenization uses the current registry configuration.
     *
     * @param registry Lexer configuration source.
     */
    explicit Lexer(const LexerRegistry &registry);

    /**
     * @brief Tokenizes source text.
     *
     * This overload tokenizes without associating the source with a file name.
     * It forwards to the file-aware overload with an empty name.
     *
     * @param source Source text to tokenize.
     * @return Produced token stream terminated by an End token.
     */
    std::vector<token::Token> tokenize(const std::string &source);

    /**
     * @brief Tokenizes source text and associates tokens with a file name.
     *
     * The source is scanned from left to right while line and column positions
     * are updated. Whitespace and comments are skipped, recognized input is
     * converted to tokens, and an End token is always appended when scanning
     * finishes.
     *
     * Identifier text is classified as Keyword when it appears in the registry.
     * Numeric and string literals may also receive an optional suffix such as
     * "_i32" or "_str", which is stored separately from the literal text.
     *
     * Registered symbols are tested using longest-match-first ordering.
     *
     * @param source Source text to tokenize.
     * @param fileName Logical source file name stored in source locations.
     * @return Produced token stream terminated by an End token.
     *
     * @throws std::runtime_error If the source contains an invalid suffix,
     *         unterminated comment or string, unsupported escape sequence,
     *         or unexpected character.
     */
    std::vector<token::Token> tokenize(const std::string &source, std::string fileName);

private:
    const LexerRegistry &registry_;
};

} // namespace novac::lexer