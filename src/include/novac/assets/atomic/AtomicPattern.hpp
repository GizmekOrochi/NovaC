#pragma once

#include <string>

namespace novac::assets::atomic {

/**
 * @brief Describes how a token pattern is matched by the parser.
 */
enum class TokenPatternMode {
    /**
     * @brief Matches a concrete token text.
     */
    Text,

    /**
     * @brief Matches a token using a symbolic token key.
     */
    TokenKey,

    /**
     * @brief Matches a token key with an exact suffix requirement.
     */
    Suffix,

    /**
     * @brief Matches a token key with a suffix validated by a regular expression.
     */
    SuffixRegex
};

/**
 * @brief Describes how a token or operator is recognized during parsing.
 *
 * Token patterns are used by literal and operation features to register
 * parser bindings. Depending on the selected mode, matching can be based
 * on literal text, token keys, or token suffix constraints.
 */
struct TokenPattern {
    /**
     * @brief Matching strategy used by this pattern.
     */
    TokenPatternMode mode{TokenPatternMode::Text};

    /**
     * @brief Literal token text used for matching.
     */
    std::string token{};

    /**
     * @brief Symbolic token key used for matching.
     */
    std::string tokenKey{};

    /**
     * @brief Required token suffix when using suffix-based matching.
     */
    std::string suffix{};

    /**
     * @brief Regular expression used to validate token suffixes.
     */
    std::string suffixPattern{};

    /**
     * @brief Indicates whether the token should be registered as a keyword.
     */
    bool keyword{};

    /**
     * @brief Creates a pattern that matches an exact token text.
     *
     * @param value Token text to match.
     * @return Configured token pattern.
     */
    static TokenPattern text(std::string value);

    /**
     * @brief Creates a pattern that matches a token key.
     *
     * @param value Token key to match.
     * @return Configured token pattern.
     */
    static TokenPattern key(std::string value);

    /**
     * @brief Creates a keyword token pattern.
     *
     * The token is matched by exact text and registered as a keyword.
     *
     * @param value Keyword text to match.
     * @return Configured token pattern.
     */
    static TokenPattern keywordText(std::string value);

    /**
     * @brief Creates a token-key pattern with an exact suffix requirement.
     *
     * @param key Token key to match.
     * @param suffix Required suffix value.
     * @return Configured token pattern.
     */
    static TokenPattern suffixed(std::string key, std::string suffix);

    /**
     * @brief Creates a token-key pattern with regex-based suffix validation.
     *
     * @param key Token key to match.
     * @param regex Regular expression applied to the token suffix.
     * @return Configured token pattern.
     */
    static TokenPattern suffixRegex(std::string key, std::string regex);
};

} // namespace novac::assets::atomic