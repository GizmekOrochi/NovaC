// Token.hpp

#pragma once

#include "../foundation/Diagnostic.hpp"

#include <string>

namespace novac::token {

/**
 * @brief Categories of lexical tokens produced by the lexer.
 */
enum class Kind {
    /** Identifier token. */
    Identifier,

    /** Reserved language keyword. */
    Keyword,

    /** Punctuation or operator symbol. */
    Symbol,

    /** Integer literal. */
    Integer,

    /** Floating-point literal. */
    Float,

    /** String literal. */
    String,

    /** End-of-input marker. */
    End
};

/**
 * @brief Represents a single lexical token.
 */
struct Token {
    Kind kind{Kind::End};
    std::string text{};
    std::string suffix{};
    diagnostics::SourceSpan span{};

    int line{1};
    int column{1};
};

} // namespace novac::token