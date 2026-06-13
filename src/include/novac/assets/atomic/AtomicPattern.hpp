#pragma once

#include <string>

namespace novac::assets::atomic {

enum class TokenPatternMode {
    Text,
    TokenKey,
    Suffix,
    SuffixRegex
};

struct TokenPattern {
    TokenPatternMode mode{TokenPatternMode::Text};

    std::string token{};
    std::string tokenKey{};
    std::string suffix{};
    std::string suffixPattern{};
    bool keyword{};

    static TokenPattern text(std::string value);
    static TokenPattern key(std::string value);
    static TokenPattern keywordText(std::string value);
    static TokenPattern suffixed(std::string key, std::string suffix);
    static TokenPattern suffixRegex(std::string key, std::string regex);
};

} // namespace novac::assets::atomic