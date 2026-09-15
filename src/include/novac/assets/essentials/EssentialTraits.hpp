#pragma once

/**
 * @brief Standard semantic traits used by Essentials AST schemas.
 *
 * Traits provide a lightweight compatibility layer between independently
 * installable features. When child trait enforcement is enabled, schemas can
 * require semantic categories without hard-coding concrete node kinds.
 */
namespace novac::assets::essentials::traits {

/** @brief Marks nodes that can be evaluated as expressions. */
inline constexpr const char *Expression{"expr"};
/** @brief Marks nodes that can execute as statements. */
inline constexpr const char *Statement{"stmt"};
/** @brief Marks nodes that declare a language-level entity. */
inline constexpr const char *Declaration{"decl"};
/** @brief Marks nodes that introduce a runtime scope. */
inline constexpr const char *Scope{"scope"};
/** @brief Marks the root node of a complete program. */
inline constexpr const char *Program{"program"};
/** @brief Marks nodes that represent callable entities. */
inline constexpr const char *Callable{"callable"};

} // namespace novac::assets::essentials::traits
