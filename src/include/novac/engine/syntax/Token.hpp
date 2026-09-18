#pragma once

#include "../foundation/Diagnostic.hpp"

#include <string>

namespace novac::token {

/**
 * @brief Categories of lexical tokens produced by the lexer.
 *
 * The token kind describes the general role of a token without storing
 * language-specific meaning. Keywords and symbols are distinguished from
 * identifiers and literal values so later parsing stages can handle them
 * appropriately.
 */
enum class Kind {
    /** Identifier token, such as a variable or function name. */
    Identifier,

    /** Reserved language keyword registered by the active language. */
    Keyword,

    /** Punctuation or operator symbol. */
    Symbol,

    /** Integer literal. */
    Integer,

    /** Floating-point literal. */
    Float,

    /** String literal. */
    String,

    /** End-of-input marker used to safely terminate token streams. */
    End
};

/**
 * @brief Represents a single lexical token produced by the lexer.
 *
 * A token stores its category, original text and optional literal suffix.
 * Source information is kept so later stages can report diagnostics at the
 * location where the token appeared.
 */
struct Token {
    /** General category of the token. */
    Kind kind{Kind::End};

    /** Original token text read from the source code. */
    std::string text{};

    /**
     * @brief Optional suffix attached to a literal.
     *
     * This can be used by language features to interpret values such as
     * numeric or string literals without hardcoding their meaning in the lexer.
     */
    std::string suffix{};

    /** Full source range occupied by the token. */
    diagnostics::SourceSpan span{};

    /** One-based source line where the token starts. */
    int line{1};

    /** One-based source column where the token starts. */
    int column{1};
};

} // namespace novac::token