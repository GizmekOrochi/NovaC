#pragma once

#include <string>
#include <utility>

namespace novac::ids {

/**
 * @brief Strong wrapper for an AST node kind identifier.
 */
struct NodeKind {
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
 */
struct FieldName {
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
 */
struct ParseDomain {
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
 */
struct Operation {
    std::string value{};

    /**
     * @brief Creates an operation identifier.
     *
     * @param name Identifier value.
     */
    explicit Operation(std::string name) : value{std::move(name)} {}
};

} // namespace novac::ids