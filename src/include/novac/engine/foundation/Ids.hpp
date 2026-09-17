#pragma once

#include <string>
#include <utility>

namespace novac::ids {

/**
 * @brief Strong wrapper for an AST node kind identifier.
 *
 * Using a dedicated type makes APIs clearer and prevents node kind names from
 * being accidentally confused with other string-based identifiers.
 */
struct NodeKind {
    /** Stored identifier value. */
    std::string value{};

    /**
     * @brief Creates a node kind identifier.
     *
     * @param name Identifier value.
     */
    explicit NodeKind(std::string name) : value{std::move(name)} {}
};

/**
 * @brief Strong wrapper for an AST field name.
 *
 * FieldName distinguishes AST field identifiers from regular strings and from
 * other identifier types used by the engine.
 */
struct FieldName {
    /** Stored identifier value. */
    std::string value{};

    /**
     * @brief Creates a field name identifier.
     *
     * @param name Identifier value.
     */
    explicit FieldName(std::string name) : value{std::move(name)} {}
};

/**
 * @brief Strong wrapper for a parser domain identifier.
 *
 * Parse domains group parser rules by purpose, for example expressions,
 * statements or types.
 */
struct ParseDomain {
    /** Stored identifier value. */
    std::string value{};

    /**
     * @brief Creates a parse domain identifier.
     *
     * @param name Identifier value.
     */
    explicit ParseDomain(std::string name) : value{std::move(name)} {}
};

/**
 * @brief Strong wrapper for an operation or instruction identifier.
 *
 * Operation is used to name generic IR operations without exposing plain
 * strings throughout the public API.
 */
struct Operation {
    /** Stored identifier value. */
    std::string value{};

    /**
     * @brief Creates an operation identifier.
     *
     * @param name Identifier value.
     */
    explicit Operation(std::string name) : value{std::move(name)} {}
};

} // namespace novac::ids