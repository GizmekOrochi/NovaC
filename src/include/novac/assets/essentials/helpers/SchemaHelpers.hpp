#pragma once

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/engine/syntax/Node.hpp"

#include <string>
#include <vector>

namespace novac::assets::essentials::helpers {

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

inline ast::FieldSchema nodeField(std::string name, bool required, std::vector<std::string> allowedKinds = {}, std::vector<std::string> allowedTraits = {}) {
    return {std::move(name), ast::FieldKind::Node, required, std::move(allowedKinds), std::move(allowedTraits)};
}

inline ast::FieldSchema nodeListField(std::string name, bool required, std::vector<std::string> allowedKinds = {}, std::vector<std::string> allowedTraits = {}) {
    return {std::move(name), ast::FieldKind::NodeList, required, std::move(allowedKinds), std::move(allowedTraits)};
}

inline ast::FieldSchema stringField(std::string name, bool required) {
    return {std::move(name), ast::FieldKind::String, required, {}, {}};
}

} // namespace novac::assets::essentials::helpers
