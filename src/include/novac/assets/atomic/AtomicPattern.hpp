#pragma once

#include <string>

namespace novac::assets::atomic {

/**
 * @brief Describes how a token pattern is matched by the parser.
 *
 * The selected mode determines which TokenPattern fields are used during
 * parser registration and matching.
 */
enum class TokenPatternMode {
    /**
     * @brief Matches a concrete token text.
     *
     * The pattern uses TokenPattern::token.
     */
    Text,

    /**
     * @brief Matches a token using a symbolic token key.
     *
     * The pattern uses TokenPattern::tokenKey.
     */
    TokenKey,

    /**
     * @brief Matches a token key with an exact suffix requirement.
     *
     * Both TokenPattern::tokenKey and TokenPattern::suffix are used.
     */
    Suffix,

    /**
     * @brief Matches a token key with a suffix validated by a regular expression.
     *
     * TokenPattern::tokenKey selects the token category while
     * TokenPattern::suffixPattern validates the suffix.
     */
    SuffixRegex
};

/**
 * @brief Describes how a token or operator is recognized during parsing.
 *
 * TokenPattern is a small description object used by atomic literal and
 * operation features when registering parser bindings.
 *
 * Depending on mode, matching can use exact token text, a symbolic token key,
 * an exact suffix, or a regular expression applied to the suffix.
 */
struct TokenPattern {
    /**
     * @brief Matching strategy used by this pattern.
     */
    TokenPatternMode mode{TokenPatternMode::Text};

    /**
     * @brief Literal token text used for Text matching.
     */
    std::string token{};

    /**
     * @brief Symbolic token key used for TokenKey and suffix-based matching.
     */
    std::string tokenKey{};

    /**
     * @brief Required token suffix when using Suffix matching.
     */
    std::string suffix{};

    /**
     * @brief Regular expression used to validate suffixes in SuffixRegex mode.
     */
    std::string suffixPattern{};

    /**
     * @brief Indicates whether the token should also be registered as a keyword.
     *
     * This is mainly used by keywordText(), which still matches by exact text.
     */
    bool keyword{};

    /**
     * @brief Creates a pattern that matches an exact token text.
     *
     * The returned pattern uses TokenPatternMode::Text and stores the supplied
     * value in token.
     *
     * @param value Token text to match.
     * @return Configured token pattern.
     */
    static TokenPattern text(std::string value);

    /**
     * @brief Creates a pattern that matches a symbolic token key.
     *
     * The returned pattern uses TokenPatternMode::TokenKey and stores the
     * supplied value in tokenKey.
     *
     * @param value Token key to match.
     * @return Configured token pattern.
     */
    static TokenPattern key(std::string value);

    /**
     * @brief Creates a keyword token pattern.
     *
     * The returned pattern still uses exact text matching, but also marks the
     * token for keyword registration.
     *
     * @param value Keyword text to match.
     * @return Configured token pattern.
     */
    static TokenPattern keywordText(std::string value);

    /**
     * @brief Creates a token-key pattern with an exact suffix requirement.
     *
     * The returned pattern uses TokenPatternMode::Suffix and stores both the
     * token key and required suffix.
     *
     * @param key Token key to match.
     * @param suffix Required suffix value.
     * @return Configured token pattern.
     */
    static TokenPattern suffixed(std::string key, std::string suffix);

    /**
     * @brief Creates a token-key pattern with regex-based suffix validation.
     *
     * The returned pattern uses TokenPatternMode::SuffixRegex and stores the
     * regular expression separately from the token key.
     *
     * @param key Token key to match.
     * @param regex Regular expression applied to the token suffix.
     * @return Configured token pattern.
     */
    static TokenPattern suffixRegex(std::string key, std::string regex);
};

} // namespace novac::assets::atomic