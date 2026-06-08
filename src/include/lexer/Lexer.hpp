#pragma once

#include "../registry/Registry.hpp"
#include "../token/Token.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace novac::lexer {

class LexerRegistry {
public:
    explicit LexerRegistry(registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    registry::RegisterStatus keyword(std::string keyword);
    registry::RegisterStatus symbol(std::string symbol);

    bool isKeyword(const std::string &value) const;

    const std::vector<std::string> &symbols() const;

private:
    std::unordered_set<std::string> keywords_;
    std::vector<std::string> symbols_;
    registry::DuplicatePolicy duplicatePolicy_;
};

class Lexer {
public:
    explicit Lexer(const LexerRegistry &registry);

    std::vector<token::Token> tokenize(const std::string &source);

private:
    const LexerRegistry &registry_;
};

} // namespace novac::lexer