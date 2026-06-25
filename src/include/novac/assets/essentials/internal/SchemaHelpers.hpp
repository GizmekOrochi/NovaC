#pragma once

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"

#include <vector>

namespace novac::assets::essentials::internal {

inline std::vector<std::string> expressionTraits(const EssentialsController &controller) {
    return controller.options().enforceChildTraits
        ? std::vector<std::string>{traits::Expression}
        : std::vector<std::string>{};
}

inline std::vector<std::string> statementTraits(const EssentialsController &controller) {
    return controller.options().enforceChildTraits
        ? std::vector<std::string>{traits::Statement}
        : std::vector<std::string>{};
}

inline std::vector<std::string> declarationTraits(const EssentialsController &controller) {
    return controller.options().enforceChildTraits
        ? std::vector<std::string>{traits::Declaration}
        : std::vector<std::string>{};
}

} // namespace novac::assets::essentials::internal
