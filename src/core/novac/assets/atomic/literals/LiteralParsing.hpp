#pragma once

#include "novac/assets/atomic/AtomicPattern.hpp"
#include "novac/engine/syntax/Token.hpp"

#include <charconv>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <system_error>

namespace novac::assets::atomic::literals::detail {

struct PreparedSuffixPattern {
    TokenPattern pattern;
    std::shared_ptr<const std::regex> regex;
};

inline PreparedSuffixPattern prepareSuffixPattern(TokenPattern pattern, const std::string &owner) {
    PreparedSuffixPattern prepared{std::move(pattern), {}};
    if (prepared.pattern.mode == TokenPatternMode::SuffixRegex) {
        try {
            prepared.regex = std::make_shared<const std::regex>(prepared.pattern.suffixPattern);
        } catch (const std::regex_error &error) {
            throw std::runtime_error(owner + ": invalid suffix regex '" + prepared.pattern.suffixPattern + "': " + error.what());
        }
    }
    return prepared;
}

inline void validateSuffix(const PreparedSuffixPattern &prepared, const token::Token &item, const std::string &owner) {
    const TokenPattern &pattern{prepared.pattern};
    if (pattern.mode == TokenPatternMode::Suffix) {
        if (item.suffix != pattern.suffix) {
            throw std::runtime_error(owner + ": expected suffix '" + pattern.suffix + "', got '" + item.suffix + "'");
        }
        return;
    }
    if (pattern.mode == TokenPatternMode::SuffixRegex &&
        (!prepared.regex || !std::regex_match(item.suffix, *prepared.regex))) {
        throw std::runtime_error(owner + ": suffix '" + item.suffix + "' does not match regex '" + pattern.suffixPattern + "'");
    }
}

inline int parseInteger(const std::string &text, const std::string &owner) {
    int value{};
    const char *first{text.data()};
    const char *last{first + text.size()};
    const auto [ptr, ec]{std::from_chars(first, last, value)};
    if (ec == std::errc::result_out_of_range) {
        throw std::runtime_error(owner + ": integer literal out of range: '" + text + "'");
    }
    if (ec != std::errc{} || ptr != last) {
        throw std::runtime_error(owner + ": invalid integer literal: '" + text + "'");
    }
    return value;
}

inline double parseFloat(const std::string &text, const std::string &owner) {
    double value{};
    const char *first{text.data()};
    const char *last{first + text.size()};
    const auto [ptr, ec]{std::from_chars(first, last, value, std::chars_format::general)};
    if (ec == std::errc::result_out_of_range) {
        throw std::runtime_error(owner + ": floating-point literal out of range: '" + text + "'");
    }
    if (ec != std::errc{} || ptr != last) {
        throw std::runtime_error(owner + ": invalid floating-point literal: '" + text + "'");
    }
    return value;
}

} // namespace novac::assets::atomic::literals::detail
