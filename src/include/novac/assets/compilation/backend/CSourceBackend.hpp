#pragma once

#include "novac/assets/compilation/backend/Backend.hpp"

namespace novac::backend {

/** Proof-of-composition backend: emits a C translation unit for a deliberately
 * small, explicitly checked MIR subset. NOT a general native compiler. */
class CSourceBackend final : public Backend {
public:
    std::optional<BackendCapabilities> capabilities() const override {
        return BackendCapabilities{"mir", "c-source", {"novac.i32.const", "novac.i32.add"}, {"return"}};
    }
    void run(compilation::CompilationSession &session) const override;
};

} // namespace novac::backend
