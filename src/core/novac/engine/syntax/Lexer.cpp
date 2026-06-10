#include "novac/engine/syntax/Lexer.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace novac::lexer {

namespace {

bool defaultIdentifierStart(unsigned char value) {
    return std::isalpha(value) != 0 || value == '_';
}

bool defaultIdentifierContinue(unsigned char value) {
    return std::isalnum(value) != 0 || value == '_';
}

bool startsWith(const std::string &source, std::size_t index, const std::string &text) {
    return !text.empty() && source.compare(index, text.size(), text) == 0;
}

std::string escapeMessage(char value) {
    switch (value) {
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
        case '\0': return "\\0";
        default: return std::string{value};
    }
}

} // namespace

LexerRegistry::LexerRegistry(registry::DuplicatePolicy duplicatePolicy)
    : keywords_{},
      symbols_{},
      identifierStart_{defaultIdentifierStart},
      identifierContinue_{defaultIdentifierContinue},
      lineCommentPrefix_{"//"},
      blockCommentBegin_{"/*"},
      blockCommentEnd_{"*/"},
      duplicatePolicy_{duplicatePolicy} {}

registry::RegisterStatus LexerRegistry::keyword(std::string keyword) {
    if (keyword.empty()) {
        throw std::runtime_error("LexerRegistry::keyword: keyword cannot be empty");
    }

    const auto iter{keywords_.find(keyword)};

    if (iter != keywords_.end()) {
        if (duplicatePolicy_ == registry::DuplicatePolicy::Ignore || duplicatePolicy_ == registry::DuplicatePolicy::Replace) {
            return registry::RegisterStatus::Ignored;
        }

        throw std::runtime_error("LexerRegistry::keyword: duplicate keyword '" + keyword + "'");
    }

    keywords_.insert(std::move(keyword));

    return registry::RegisterStatus::Inserted;
}

registry::RegisterStatus LexerRegistry::symbol(std::string symbol) {
    if (symbol.empty()) {
        throw std::runtime_error("LexerRegistry::symbol: symbol cannot be empty");
    }

    const auto iter{std::find(symbols_.begin(), symbols_.end(), symbol)};

    if (iter != symbols_.end()) {
        if (duplicatePolicy_ == registry::DuplicatePolicy::Ignore || duplicatePolicy_ == registry::DuplicatePolicy::Replace) {
            return registry::RegisterStatus::Ignored;
        }

        throw std::runtime_error("LexerRegistry::symbol: duplicate symbol '" + symbol + "'");
    }

    symbols_.push_back(std::move(symbol));

    std::sort(symbols_.begin(), symbols_.end(), [](const std::string &left, const std::string &right) {
        if (left.size() == right.size()) {
            return left < right;
        }

        return left.size() > right.size();
    });

    return registry::RegisterStatus::Inserted;
}

void LexerRegistry::setIdentifierRules(IdentifierStartPredicate start, IdentifierContinuePredicate continuation) {
    if (!start || !continuation) {
        throw std::runtime_error("LexerRegistry::setIdentifierRules: predicates cannot be empty");
    }

    identifierStart_ = std::move(start);
    identifierContinue_ = std::move(continuation);
}

void LexerRegistry::setLineCommentPrefix(std::string prefix) {
    lineCommentPrefix_ = std::move(prefix);
}

void LexerRegistry::setBlockCommentDelimiters(std::string begin, std::string end) {
    if (begin.empty() != end.empty()) {
        throw std::runtime_error("LexerRegistry::setBlockCommentDelimiters: begin and end must both be empty or both be non-empty");
    }

    blockCommentBegin_ = std::move(begin);
    blockCommentEnd_ = std::move(end);
}

bool LexerRegistry::isKeyword(const std::string &value) const {
    return keywords_.find(value) != keywords_.end();
}

bool LexerRegistry::isIdentifierStart(unsigned char value) const {
    return identifierStart_(value);
}

bool LexerRegistry::isIdentifierContinue(unsigned char value) const {
    return identifierContinue_(value);
}

const std::vector<std::string> &LexerRegistry::symbols() const {
    return symbols_;
}

const std::string &LexerRegistry::lineCommentPrefix() const {
    return lineCommentPrefix_;
}

const std::string &LexerRegistry::blockCommentBegin() const {
    return blockCommentBegin_;
}

const std::string &LexerRegistry::blockCommentEnd() const {
    return blockCommentEnd_;
}

Lexer::Lexer(const LexerRegistry &registry)
    : registry_{registry} {}

std::vector<token::Token> Lexer::tokenize(const std::string &source) {
    return tokenize(source, {});
}

std::vector<token::Token> Lexer::tokenize(const std::string &source, std::string fileName) {
    std::vector<token::Token> tokens{};

    std::size_t index{};
    int line{1};
    int column{1};

    const auto location{[&]() {
        return diagnostics::SourceLocation{fileName, index, line, column};
    }};

    const auto spanFrom{[](diagnostics::SourceLocation begin, diagnostics::SourceLocation end) {
        return diagnostics::SourceSpan{std::move(begin), std::move(end)};
    }};

    const auto advance{[&]() {
        if (index >= source.size()) {
            return;
        }

        if (source[index] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }

        ++index;
    }};

    const auto pushToken{[&](token::Kind kind, std::string text, diagnostics::SourceLocation begin, int tokenLine, int tokenColumn) {
        tokens.push_back({kind, std::move(text), spanFrom(std::move(begin), location()), tokenLine, tokenColumn});
    }};

    while (index < source.size()) {
        const unsigned char current{static_cast<unsigned char>(source[index])};

        if (std::isspace(current) != 0) {
            advance();
            continue;
        }

        if (startsWith(source, index, registry_.lineCommentPrefix())) {
            while (index < source.size() && source[index] != '\n') {
                advance();
            }
            continue;
        }

        if (startsWith(source, index, registry_.blockCommentBegin())) {
            const diagnostics::SourceLocation begin{location()};

            for (std::size_t count{}; count < registry_.blockCommentBegin().size(); ++count) {
                advance();
            }

            while (index < source.size() && !startsWith(source, index, registry_.blockCommentEnd())) {
                advance();
            }

            if (index >= source.size()) {
                throw std::runtime_error(
                    "Lexer::tokenize: unterminated block comment at line " + std::to_string(begin.line)
                    + ", column " + std::to_string(begin.column));
            }

            for (std::size_t count{}; count < registry_.blockCommentEnd().size(); ++count) {
                advance();
            }

            continue;
        }

        if (registry_.isIdentifierStart(current)) {
            const diagnostics::SourceLocation begin{location()};
            const int tokenLine{line};
            const int tokenColumn{column};
            std::string text{};

            while (index < source.size() && registry_.isIdentifierContinue(static_cast<unsigned char>(source[index]))) {
                text += source[index];
                advance();
            }

            pushToken(registry_.isKeyword(text) ? token::Kind::Keyword : token::Kind::Identifier, text, begin, tokenLine, tokenColumn);
            continue;
        }

        if (std::isdigit(current) != 0) {
            const diagnostics::SourceLocation begin{location()};
            const int tokenLine{line};
            const int tokenColumn{column};
            std::string text{};
            bool isFloat{};

            while (index < source.size() && std::isdigit(static_cast<unsigned char>(source[index])) != 0) {
                text += source[index];
                advance();
            }

            if (index < source.size() && source[index] == '.' && index + 1 < source.size()
                && std::isdigit(static_cast<unsigned char>(source[index + 1])) != 0) {
                isFloat = true;
                text += source[index];
                advance();

                while (index < source.size() && std::isdigit(static_cast<unsigned char>(source[index])) != 0) {
                    text += source[index];
                    advance();
                }
            }

            pushToken(isFloat ? token::Kind::Float : token::Kind::Integer, text, begin, tokenLine, tokenColumn);
            continue;
        }

        if (source[index] == '"') {
            const diagnostics::SourceLocation begin{location()};
            const int tokenLine{line};
            const int tokenColumn{column};
            std::string text{};
            advance();

            while (index < source.size() && source[index] != '"') {
                if (source[index] == '\n') {
                    throw std::runtime_error(
                        "Lexer::tokenize: unterminated string literal at line " + std::to_string(tokenLine)
                        + ", column " + std::to_string(tokenColumn));
                }

                if (source[index] == '\\') {
                    advance();

                    if (index >= source.size()) {
                        throw std::runtime_error("Lexer::tokenize: unterminated escape sequence");
                    }

                    switch (source[index]) {
                        case 'n': text += '\n'; break;
                        case 'r': text += '\r'; break;
                        case 't': text += '\t'; break;
                        case '"': text += '"'; break;
                        case '\\': text += '\\'; break;
                        default:
                            throw std::runtime_error(
                                "Lexer::tokenize: unsupported escape sequence '\\" + escapeMessage(source[index]) + "'");
                    }

                    advance();
                    continue;
                }

                text += source[index];
                advance();
            }

            if (index >= source.size()) {
                throw std::runtime_error(
                    "Lexer::tokenize: unterminated string literal at line " + std::to_string(tokenLine)
                    + ", column " + std::to_string(tokenColumn));
            }

            advance();
            pushToken(token::Kind::String, text, begin, tokenLine, tokenColumn);
            continue;
        }

        bool matched{false};

        for (const std::string &symbol : registry_.symbols()) {
            if (source.compare(index, symbol.size(), symbol) == 0) {
                const diagnostics::SourceLocation begin{location()};
                const int tokenLine{line};
                const int tokenColumn{column};

                for (std::size_t count{}; count < symbol.size(); ++count) {
                    advance();
                }

                pushToken(token::Kind::Symbol, symbol, begin, tokenLine, tokenColumn);
                matched = true;
                break;
            }
        }

        if (!matched) {
            throw std::runtime_error(
                "Lexer::tokenize: unexpected character '" + escapeMessage(source[index])
                + "' at line " + std::to_string(line) + ", column " + std::to_string(column));
        }
    }

    tokens.push_back({token::Kind::End, "", spanFrom(location(), location()), line, column});

    return tokens;
}

} // namespace novac::lexer
