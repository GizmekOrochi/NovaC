#include "../../include/lexer/Lexer.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace novac::lexer {

LexerRegistry::LexerRegistry(registry::DuplicatePolicy duplicatePolicy)
    : keywords_{}, symbols_{}, duplicatePolicy_{duplicatePolicy} {}

registry::RegisterStatus LexerRegistry::keyword(std::string keyword) {
    if (keyword.empty())
        throw std::runtime_error("LexerRegistry::keyword: keyword cannot be empty");

    const auto iter{keywords_.find(keyword)};

    if (iter != keywords_.end()) {
        if (duplicatePolicy_ == registry::DuplicatePolicy::Ignore)
            return registry::RegisterStatus::Ignored;

        if (duplicatePolicy_ == registry::DuplicatePolicy::Replace)
            return registry::RegisterStatus::Replaced;

        throw std::runtime_error("LexerRegistry::keyword: duplicate keyword '" + keyword + "'");
    }

    keywords_.insert(std::move(keyword));

    return registry::RegisterStatus::Inserted;
}

registry::RegisterStatus LexerRegistry::symbol(std::string symbol) {
    if (symbol.empty())
        throw std::runtime_error("LexerRegistry::symbol: symbol cannot be empty");

    const auto iter{std::find(symbols_.begin(), symbols_.end(), symbol)};

    if (iter != symbols_.end()) {
        if (duplicatePolicy_ == registry::DuplicatePolicy::Ignore)
            return registry::RegisterStatus::Ignored;

        if (duplicatePolicy_ == registry::DuplicatePolicy::Replace)
            return registry::RegisterStatus::Replaced;

        throw std::runtime_error("LexerRegistry::symbol: duplicate symbol '" + symbol + "'");
    }

    symbols_.push_back(std::move(symbol));

    std::sort(symbols_.begin(), symbols_.end(), [](const std::string &left, const std::string &right) { return left.size() > right.size(); });

    return registry::RegisterStatus::Inserted;
}

bool LexerRegistry::isKeyword(const std::string &value) const {
    return keywords_.find(value) != keywords_.end();
}

const std::vector<std::string> &LexerRegistry::symbols() const {
    return symbols_;
}

Lexer::Lexer(const LexerRegistry &registry)
    : registry_{registry} {}

std::vector<token::Token> Lexer::tokenize(const std::string &source) {
    std::vector<token::Token> tokens{};

    std::size_t index{};
    int line{1};
    int column{1};

    const auto advance{[&]() {
            if (source[index] == '\n') {
                ++line;
                column = 1;
            } 
            else
                ++column;

            ++index;
        }
    };

    while (index < source.size()) {
        const unsigned char current{static_cast<unsigned char>(source[index])
        };

        if (std::isspace(current)) {
            advance();
            continue;
        }

        if (std::isalpha(current) || source[index] == '_') {
            const int tokenColumn{column};
            std::string text{};

            while (index < source.size() && (std::isalnum(static_cast<unsigned char>(source[index])) || source[index] == '_')) {
                text += source[index];
                advance();
            }

            tokens.push_back({registry_.isKeyword(text) ? token::Kind::Keyword : token::Kind::Identifier, text, line,tokenColumn});

            continue;
        }

        if (std::isdigit(current)) {
            const int tokenColumn{column};
            std::string text{};

            while (index < source.size() && std::isdigit(static_cast<unsigned char>(source[index]))) {
                text += source[index];
                advance();
            }

            tokens.push_back({token::Kind::Integer, text, line, tokenColumn});

            continue;
        }

        bool matched{false};

        for (const std::string &symbol : registry_.symbols()) {
            if (source.compare(index, symbol.size(), symbol) == 0) {
                tokens.push_back({token::Kind::Symbol, symbol, line, column});

                for (std::size_t count{}; count < symbol.size(); ++count)
                    advance();

                matched = true;
                break;
            }
        }

        if (!matched)
            throw std::runtime_error("Lexer::tokenize: unexpected character '" + std::string{source[index]} + "' at line " + std::to_string(line) + ", column " + std::to_string(column));
    }

    tokens.push_back({token::Kind::End, "", line, column});

    return tokens;
}

} // namespace novac::lexer