#pragma once

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/engine/syntax/Node.hpp"

#include <string>
#include <vector>

namespace novac::assets::essentials::helpers {

/**
 * @brief Builds a trait list only when trait enforcement is enabled.
 *
 * This helper is used when constructing AST schemas whose child trait
 * restrictions are optional. When enforcement is disabled, an empty list is
 * returned so the generated field schema accepts nodes without trait checks.
 *
 * @param enabled Whether the supplied traits should be included.
 * @param traits Trait names to copy into the schema list.
 * @return Copied traits when enabled, otherwise an empty vector.
 */
inline std::vector<std::string> maybeTraits(bool enabled, std::initializer_list<const char *> traits) {
    if (!enabled) {
        return {};
    }

    std::vector<std::string> result{};
    for (const char *trait : traits) {
        result.emplace_back(trait);
    }

    return result;
}

/**
 * @brief Creates a schema entry for a single child AST node.
 *
 * The generated schema uses ast::FieldKind::Node and forwards any concrete
 * node-kind or trait restrictions to the resulting FieldSchema.
 *
 * @param name Field name.
 * @param required Whether the field must be present.
 * @param allowedKinds Optional concrete node-kind restrictions.
 * @param allowedTraits Optional semantic trait restrictions.
 * @return Configured node field schema.
 */
inline ast::FieldSchema nodeField(std::string name, bool required, std::vector<std::string> allowedKinds = {}, std::vector<std::string> allowedTraits = {}) {
    return {std::move(name), ast::FieldKind::Node, required, std::move(allowedKinds), std::move(allowedTraits)};
}

/**
 * @brief Creates a schema entry for a list of child AST nodes.
 *
 * The generated schema uses ast::FieldKind::NodeList. Any supplied kind or
 * trait restrictions are applied to the child nodes contained in the list.
 *
 * @param name Field name.
 * @param required Whether the field must be present.
 * @param allowedKinds Optional concrete node-kind restrictions.
 * @param allowedTraits Optional semantic trait restrictions.
 * @return Configured node-list field schema.
 */
inline ast::FieldSchema nodeListField(std::string name, bool required, std::vector<std::string> allowedKinds = {}, std::vector<std::string> allowedTraits = {}) {
    return {std::move(name), ast::FieldKind::NodeList, required, std::move(allowedKinds), std::move(allowedTraits)};
}

/**
 * @brief Creates a schema entry for a string field.
 *
 * The generated schema uses ast::FieldKind::String and does not apply child
 * node kind or trait restrictions.
 *
 * @param name Field name.
 * @param required Whether the field must be present.
 * @return Configured string field schema.
 */
inline ast::FieldSchema stringField(std::string name, bool required) {
    return {std::move(name), ast::FieldKind::String, required, {}, {}};
}

} // namespace novac::assets::essentials::helpers
