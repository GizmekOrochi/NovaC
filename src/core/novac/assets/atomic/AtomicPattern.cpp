#include "novac/assets/atomic/AtomicPattern.hpp"

#include <utility>

namespace novac::assets::atomic {

/**
 * @brief Implements the `text` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
TokenPattern TokenPattern::text(std::string value) {
    return TokenPattern{TokenPatternMode::Text, std::move(value), {}, {}, {}, false};
}

/**
 * @brief Implements the `key` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
TokenPattern TokenPattern::key(std::string value) {
    return TokenPattern{TokenPatternMode::TokenKey, {}, std::move(value), {}, {}, false};
}

/**
 * @brief Implements the `keywordText` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
TokenPattern TokenPattern::keywordText(std::string value) {
    return TokenPattern{TokenPatternMode::Text, std::move(value), {}, {}, {}, true};
}

/**
 * @brief Implements the `suffixed` operation.
 *
 * @param key Value supplied for `key`.
 * @param suffix Value supplied for `suffix`.
 * @return Value produced by the operation.
 */
TokenPattern TokenPattern::suffixed(std::string key, std::string suffix) {
    return TokenPattern{TokenPatternMode::Suffix, {}, std::move(key), std::move(suffix), {}, false};
}

/**
 * @brief Implements the `suffixRegex` operation.
 *
 * @param key Value supplied for `key`.
 * @param regex Value supplied for `regex`.
 * @return Value produced by the operation.
 */
TokenPattern TokenPattern::suffixRegex(std::string key, std::string regex) {
    return TokenPattern{TokenPatternMode::SuffixRegex, {}, std::move(key), {}, std::move(regex), false};
}

} // namespace novac::assets::atomic
