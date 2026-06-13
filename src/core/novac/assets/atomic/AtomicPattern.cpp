#include "novac/assets/atomic/AtomicPattern.hpp"

#include <utility>

namespace novac::assets::atomic {

TokenPattern TokenPattern::text(std::string value) {
    return TokenPattern{TokenPatternMode::Text, std::move(value), {}, {}, {}, false};
}

TokenPattern TokenPattern::key(std::string value) {
    return TokenPattern{TokenPatternMode::TokenKey, {}, std::move(value), {}, {}, false};
}

TokenPattern TokenPattern::keywordText(std::string value) {
    return TokenPattern{TokenPatternMode::Text, std::move(value), {}, {}, {}, true};
}

TokenPattern TokenPattern::suffixed(std::string key, std::string suffix) {
    return TokenPattern{TokenPatternMode::Suffix, {}, std::move(key), std::move(suffix), {}, false};
}

TokenPattern TokenPattern::suffixRegex(std::string key, std::string regex) {
    return TokenPattern{TokenPatternMode::SuffixRegex, {}, std::move(key), {}, std::move(regex), false};
}

} // namespace novac::assets::atomic