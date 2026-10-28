#pragma once

#include "novac/assets/compilation/Compilation.hpp"
#include "novac/assets/compilation/CompilationOptions.hpp"
#include "novac/assets/compilation/feature/CompilationFacet.hpp"
#include "novac/assets/compilation/backend/Backend.hpp"
#include "novac/assets/compilation/target/TargetRegistry.hpp"

namespace novac::compilation {

/** Experimental opt-in compilation path. No source parser, runtime or backend
 * imposed on Engine or interpreter-only features. */
class ModularCompilationController final {
public:
    CompilationFacetRegistry &features() noexcept { return features_; }
    ir::LoweringRegistry &lowering() noexcept { return lowering_; }
    backend::BackendRegistry &backends() noexcept { return backends_; }
    /** Opt-in providers for architecture + OS + ABI + object format tuples. */
    target::TargetRegistry &targets() noexcept { return targets_; }
    const target::TargetRegistry &targets() const noexcept { return targets_; }

    /** Build a MIR program and select an exact registered native target. */
    CompilationSession compileForTarget(const ast::Node &root, const SemanticResolver &semantics,
                                        const target::TargetSpec &target) const;
    /** Explicit provider plus session-scoped code-generation preferences. */
    CompilationSession compileForTarget(const ast::Node &root, const SemanticResolver &semantics,
                                        const target::TargetSpec &target,
                                        const CompilationOptions &options,
                                        const std::string &providerId = {}) const;

    /** A backend is optional: empty id requests only HIR and MIR artifacts. */
    CompilationSession compile(const ast::Node &root, const SemanticResolver &semantics,
                               std::string backendId = {}) const;
private:
    CompilationFacetRegistry features_{};
    ir::LoweringRegistry lowering_{};
    backend::BackendRegistry backends_{};
    target::TargetRegistry targets_{};
};

} // namespace novac::compilation
