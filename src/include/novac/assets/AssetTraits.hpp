#pragma once

/**
 * @brief Semantic traits shared by asset modules.
 *
 * These names belong to the asset layer rather than the engine. The engine
 * only stores and matches trait strings; it does not know what an expression,
 * statement, declaration, or other language-level asset concept means.
 */
namespace novac::assets::traits {

/** Marks AST nodes that can be evaluated as expressions. */
inline constexpr const char *Expression{"expr"};

} // namespace novac::assets::traits
