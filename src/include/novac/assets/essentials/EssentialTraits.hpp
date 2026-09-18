#pragma once

/**
 * @brief Standard semantic traits used by Essentials AST schemas.
 *
 * Traits provide a lightweight compatibility layer between independently
 * installable features. Instead of coupling schemas to concrete node kinds,
 * features can describe children using semantic categories such as expressions
 * or statements.
 *
 * EssentialsControllerOptions::enforceChildTraits controls whether these
 * restrictions are attached to generated child field schemas.
 */
namespace novac::assets::essentials::traits {

/**
 * @brief Marks nodes that can be evaluated as expressions.
 */
inline constexpr const char *Expression{"expr"};

/**
 * @brief Marks nodes that can execute as statements.
 */
inline constexpr const char *Statement{"stmt"};

/**
 * @brief Marks nodes that declare a language-level entity.
 */
inline constexpr const char *Declaration{"decl"};

/**
 * @brief Marks nodes that introduce or represent a runtime scope.
 */
inline constexpr const char *Scope{"scope"};

/**
 * @brief Marks the root node of a complete Essentials program.
 */
inline constexpr const char *Program{"program"};

/**
 * @brief Marks nodes that represent callable language entities.
 */
inline constexpr const char *Callable{"callable"};

} // namespace novac::assets::essentials::traits
