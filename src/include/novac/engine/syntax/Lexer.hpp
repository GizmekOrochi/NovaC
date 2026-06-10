#pragma once

#include "../foundation/registry/Registry.hpp"
#include "../syntax/Token.hpp"

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace novac::lexer {

class LexerRegistry {
public:
    using IdentifierStartPredicate = std::function<bool(unsigned char)>;
    using IdentifierContinuePredicate = std::function<bool(unsigned char)>;

    explicit LexerRegistry(registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    registry::RegisterStatus keyword(std::string keyword);
    registry::RegisterStatus symbol(std::string symbol);

    void setIdentifierRules(IdentifierStartPredicate start, IdentifierContinuePredicate continuation);
    void setLineCommentPrefix(std::string prefix);
    void setBlockCommentDelimiters(std::string begin, std::string end);

    bool isKeyword(const std::string &value) const;
    bool isIdentifierStart(unsigned char value) const;
    bool isIdentifierContinue(unsigned char value) const;

    const std::vector<std::string> &symbols() const;
    const std::string &lineCommentPrefix() const;
    const std::string &blockCommentBegin() const;
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

class Lexer {
public:
    explicit Lexer(const LexerRegistry &registry);

    std::vector<token::Token> tokenize(const std::string &source);
    std::vector<token::Token> tokenize(const std::string &source, std::string fileName);

private:
    const LexerRegistry &registry_;
};

} // namespace novac::lexer
