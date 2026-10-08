#include "novac/engine/syntax/Lexer.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace novac::lexer {

namespace {

/**
 * @brief Implements the `defaultIdentifierStart` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
bool defaultIdentifierStart(unsigned char value) {
    return std::isalpha(value) != 0 || value == '_';
}

/**
 * @brief Implements the `defaultIdentifierContinue` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
bool defaultIdentifierContinue(unsigned char value) {
    return std::isalnum(value) != 0 || value == '_';
}

/**
 * @brief Starts the operation represented by `startsWith`.
 *
 * @param source Value supplied for `source`.
 * @param index Value supplied for `index`.
 * @param text Value supplied for `text`.
 * @return Value produced by the operation.
 */
bool startsWith(const std::string &source, std::size_t index, const std::string &text) {
    return !text.empty() && source.compare(index, text.size(), text) == 0;
}

/**
 * @brief Implements the `escapeMessage` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
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

/**
 * @brief Constructs a `LexerRegistry` instance.
 *
 * @param duplicatePolicy Value supplied for `duplicatePolicy`.
 */
LexerRegistry::LexerRegistry(registry::DuplicatePolicy duplicatePolicy)
    : keywords_{}, symbols_{}, tokenRules_{}, nextTokenRuleOrder_{}, identifierStart_{defaultIdentifierStart},
      identifierContinue_{defaultIdentifierContinue}, lineCommentPrefix_{"//"}, blockCommentBegin_{"/*"}, blockCommentEnd_{"*/"},
      duplicatePolicy_{duplicatePolicy} {}

/**
 * @brief Implements the `keyword` operation.
 *
 * @param keyword Value supplied for `keyword`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus LexerRegistry::keyword(std::string keyword) {
    if (keyword.empty()) {
        throw std::runtime_error("LexerRegistry::keyword: keyword cannot be empty");
    }

    const auto iter{keywords_.find(keyword)};

    if (iter != keywords_.end()) {
        return registry::RegisterStatus::Ignored;
    }

    keywords_.insert(std::move(keyword));

    return registry::RegisterStatus::Inserted;
}

/**
 * @brief Implements the `symbol` operation.
 *
 * @param symbol Value supplied for `symbol`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus LexerRegistry::symbol(std::string symbol) {
    if (symbol.empty()) {
        throw std::runtime_error("LexerRegistry::symbol: symbol cannot be empty");
    }

    const auto iter{
        std::find(symbols_.begin(), symbols_.end(), symbol)
    };

    if (iter != symbols_.end()) {
        return registry::RegisterStatus::Ignored;
    }

    symbols_.push_back(std::move(symbol));

    std::sort(
        symbols_.begin(),
        symbols_.end(),
        [](const std::string &left, const std::string &right) {
            if (left.size() == right.size()) {
                return left < right;
            }

            return left.size() > right.size();
        });

    return registry::RegisterStatus::Inserted;
}

/**
 * @brief Implements the `tokenRule` operation.
 *
 * @param id Value supplied for `id`.
 * @param priority Value supplied for `priority`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus LexerRegistry::tokenRule(std::string id, int priority, TokenRuleFn fn) {
    if (id.empty()) {
        throw std::runtime_error("LexerRegistry::tokenRule: id cannot be empty");
    }
    if (!fn) {
        throw std::runtime_error("LexerRegistry::tokenRule: callback cannot be empty");
    }

    const auto existing{std::find_if(tokenRules_.begin(), tokenRules_.end(), [&](const TokenRuleEntry &entry) { return entry.id == id; })};
    if (existing != tokenRules_.end()) {
        if (duplicatePolicy_ == registry::DuplicatePolicy::Ignore) {
            return registry::RegisterStatus::Ignored;
        }
        if (duplicatePolicy_ == registry::DuplicatePolicy::Error) {
            throw std::runtime_error("LexerRegistry::tokenRule: duplicate rule '" + id + "'");
        }
        existing->priority = priority;
        existing->fn = std::move(fn);
        std::stable_sort(tokenRules_.begin(), tokenRules_.end(), [](const TokenRuleEntry &left, const TokenRuleEntry &right) {
            return left.priority == right.priority ? left.registrationOrder < right.registrationOrder : left.priority > right.priority;
        });
        return registry::RegisterStatus::Replaced;
    }

    tokenRules_.push_back(TokenRuleEntry{std::move(id), priority, nextTokenRuleOrder_++, std::move(fn)});
    std::stable_sort(tokenRules_.begin(), tokenRules_.end(), [](const TokenRuleEntry &left, const TokenRuleEntry &right) {
        return left.priority == right.priority ? left.registrationOrder < right.registrationOrder : left.priority > right.priority;
    });
    return registry::RegisterStatus::Inserted;
}

/**
 * @brief Sets the value handled by `setIdentifierRules`.
 *
 * @param start Value supplied for `start`.
 * @param continuation Value supplied for `continuation`.
 */
void LexerRegistry::setIdentifierRules(
    IdentifierStartPredicate start,
    IdentifierContinuePredicate continuation) {
    if (!start || !continuation) {
        throw std::runtime_error("LexerRegistry::setIdentifierRules: predicates cannot be empty");
    }

    identifierStart_ = std::move(start);
    identifierContinue_ = std::move(continuation);
}

/**
 * @brief Sets the value handled by `setLineCommentPrefix`.
 *
 * @param prefix Value supplied for `prefix`.
 */
void LexerRegistry::setLineCommentPrefix(std::string prefix) {
    lineCommentPrefix_ = std::move(prefix);
}

/**
 * @brief Sets the value handled by `setBlockCommentDelimiters`.
 *
 * @param begin Value supplied for `begin`.
 * @param end Value supplied for `end`.
 */
void LexerRegistry::setBlockCommentDelimiters(std::string begin, std::string end) {
    if (begin.empty() != end.empty()) {
        throw std::runtime_error("LexerRegistry::setBlockCommentDelimiters: begin and end must both be empty or both be non-empty");
    }

    blockCommentBegin_ = std::move(begin);
    blockCommentEnd_ = std::move(end);
}

/**
 * @brief Checks the condition represented by `isKeyword`.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
bool LexerRegistry::isKeyword(const std::string &value) const {
    return keywords_.find(value) != keywords_.end();
}

/**
 * @brief Checks the condition represented by `isIdentifierStart`.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
bool LexerRegistry::isIdentifierStart(unsigned char value) const {
    return identifierStart_(value);
}

/**
 * @brief Checks the condition represented by `isIdentifierContinue`.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
bool LexerRegistry::isIdentifierContinue(unsigned char value) const {
    return identifierContinue_(value);
}

/**
 * @brief Implements the `symbols` operation.
 *
 * @return Value produced by the operation.
 */
const std::vector<std::string> &LexerRegistry::symbols() const {
    return symbols_;
}

/**
 * @brief Implements the `tokenRules` operation.
 *
 * @return Value produced by the operation.
 */
const std::vector<LexerRegistry::TokenRuleEntry> &LexerRegistry::tokenRules() const noexcept {
    return tokenRules_;
}

/**
 * @brief Implements the `lineCommentPrefix` operation.
 *
 * @return Value produced by the operation.
 */
const std::string &LexerRegistry::lineCommentPrefix() const {
    return lineCommentPrefix_;
}

/**
 * @brief Implements the `blockCommentBegin` operation.
 *
 * @return Value produced by the operation.
 */
const std::string &LexerRegistry::blockCommentBegin() const {
    return blockCommentBegin_;
}

/**
 * @brief Implements the `blockCommentEnd` operation.
 *
 * @return Value produced by the operation.
 */
const std::string &LexerRegistry::blockCommentEnd() const {
    return blockCommentEnd_;
}

/**
 * @brief Constructs a `Lexer` instance.
 *
 * @param registry Value supplied for `registry`.
 */
Lexer::Lexer(const LexerRegistry &registry)
    : registry_{registry} {}

/**
 * @brief Implements the `tokenize` operation.
 *
 * @param source Value supplied for `source`.
 * @return Value produced by the operation.
 */
std::vector<token::Token> Lexer::tokenize(const std::string &source) {
    return tokenize(source, std::string{});
}

/**
 * @brief Implements the `tokenize` operation.
 *
 * @param source Value supplied for `source`.
 * @param fileName Value supplied for `fileName`.
 * @return Value produced by the operation.
 */
std::vector<token::Token> Lexer::tokenize(const std::string &source, std::string fileName) {
    return tokenize(source, diagnostics::SourceLocation{std::move(fileName), 0, 1, 1});
}

/**
 * @brief Implements the `tokenize` operation.
 *
 * @param source Value supplied for `source`.
 * @param origin Value supplied for `origin`.
 * @return Value produced by the operation.
 */
std::vector<token::Token> Lexer::tokenize(
    const std::string &source,
    diagnostics::SourceLocation origin) {
    std::vector<token::Token> tokens{};

    std::size_t index{};
    int line{origin.line};
    int column{origin.column};

    const auto location{[&]() {
        return diagnostics::SourceLocation{origin.file, origin.offset + index, line, column};
    }};
    const auto spanFrom{[](diagnostics::SourceLocation begin, diagnostics::SourceLocation end) {return diagnostics::SourceSpan{std::move(begin), std::move(end)};}};

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

    const auto pushToken{
        [&](token::Kind kind, std::string text, std::string suffix, diagnostics::SourceLocation begin, int tokenLine, int tokenColumn, std::string tag = {}) {
            tokens.push_back({kind, std::move(text), std::move(suffix), spanFrom(std::move(begin), location()), tokenLine, tokenColumn, std::move(tag)});
        }
    };

    const auto readSuffix{[&]() {
        std::string suffix{};

        if (index < source.size() && source[index] == '_') {
            suffix += source[index];
            advance();

            if (index >= source.size() || !registry_.isIdentifierStart(static_cast<unsigned char>(source[index]))) {
                throw std::runtime_error("Lexer::tokenize: invalid literal suffix at line " + std::to_string(line) + ", column " + std::to_string(column));
            }

            while (index < source.size() && registry_.isIdentifierContinue(static_cast<unsigned char>(source[index]))) {
                suffix += source[index];
                advance();
            }
        }

        return suffix;
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
                throw std::runtime_error("Lexer::tokenize: unterminated block comment at line " + std::to_string(begin.line)  + ", column " + std::to_string(begin.column));
            }

            for (std::size_t count{}; count < registry_.blockCommentEnd().size(); ++count) {
                advance();
            }

            continue;
        }

        bool customMatched{false};
        for (const LexerRegistry::TokenRuleEntry &entry : registry_.tokenRules()) {
            const std::string_view remaining{source.data() + index, source.size() - index};
            std::optional<TokenRuleMatch> match{entry.fn(remaining)};
            if (!match.has_value()) {
                continue;
            }
            if (match->length == 0 || match->length > remaining.size()) {
                throw std::runtime_error("Lexer::tokenize: custom rule '" + entry.id + "' returned invalid length");
            }
            if (match->kind == token::Kind::End) {
                throw std::runtime_error("Lexer::tokenize: custom rule '" + entry.id + "' cannot emit End token");
            }

            const diagnostics::SourceLocation begin{location()};
            const int tokenLine{line};
            const int tokenColumn{column};
            std::string text{match->text.empty() ? std::string{remaining.substr(0, match->length)} : std::move(match->text)};
            for (std::size_t count{}; count < match->length; ++count) {
                advance();
            }
            pushToken(match->kind, std::move(text), std::move(match->suffix), begin, tokenLine, tokenColumn, std::move(match->tag));
            customMatched = true;
            break;
        }
        if (customMatched) {
            continue;
        }

        if (registry_.isIdentifierStart(current)) {
            const diagnostics::SourceLocation begin{location()};
            const int tokenLine{line};
            const int tokenColumn{column};
            std::string text{};

            while (index < source.size()
                && registry_.isIdentifierContinue(static_cast<unsigned char>(source[index]))) {
                text += source[index];
                advance();
            }

            pushToken(registry_.isKeyword(text) ? token::Kind::Keyword : token::Kind::Identifier, text, "", begin, tokenLine, tokenColumn);
            continue;
        }

        if (std::isdigit(current) != 0) {
            const diagnostics::SourceLocation begin{location()};
            const int tokenLine{line};
            const int tokenColumn{column};
            std::string text{};
            bool isFloat{};

            while (index < source.size()
                && std::isdigit(static_cast<unsigned char>(source[index])) != 0) {
                text += source[index];
                advance();
            }

            if (index < source.size() && source[index] == '.' && index + 1 < source.size() && std::isdigit(static_cast<unsigned char>(source[index + 1])) != 0) {
                isFloat = true;
                text += source[index];
                advance();

                while (index < source.size()
                    && std::isdigit(static_cast<unsigned char>(source[index])) != 0) {
                    text += source[index];
                    advance();
                }
            }

            pushToken(isFloat ? token::Kind::Float : token::Kind::Integer, text, readSuffix(), begin, tokenLine, tokenColumn);
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
                    throw std::runtime_error("Lexer::tokenize: unterminated string literal at line " + std::to_string(tokenLine) + ", column " + std::to_string(tokenColumn));
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
                            throw std::runtime_error("Lexer::tokenize: unsupported escape sequence '\\" + escapeMessage(source[index]) + "'");
                    }

                    advance();
                    continue;
                }

                text += source[index];
                advance();
            }

            if (index >= source.size()) {
                throw std::runtime_error("Lexer::tokenize: unterminated string literal at line " + std::to_string(tokenLine) + ", column " + std::to_string(tokenColumn));
            }

            advance();
            pushToken(token::Kind::String, text, readSuffix(), begin, tokenLine, tokenColumn);
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

                pushToken( token::Kind::Symbol, symbol, "", begin, tokenLine, tokenColumn);
                matched = true;
                break;
            }
        }

        if (!matched) {
            throw std::runtime_error("Lexer::tokenize: unexpected character '" + escapeMessage(source[index]) + "' at line " + std::to_string(line) + ", column " + std::to_string(column));
        }
    }

    tokens.push_back({token::Kind::End, "", "", spanFrom(location(), location()), line, column});

    return tokens;
}

} // namespace novac::lexer
