#include "novac/assets/compilation/ModularCompilationController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::compilation {

CompilationSession ModularCompilationController::compile(const ast::Node &root,
        const SemanticResolver &semantics, std::string backendId) const {
    CompilationSession session{};
    auto hir = features_.lower(root, semantics);
    session.artifacts().insert<ir::HIRModule>("hir", std::move(hir));
    auto mir = ir::HIRLoweringPass{lowering_}.lower(session.artifacts().require<ir::HIRModule>("hir"),
                                                   {{.requireTerminators = true}});
    session.artifacts().insert<ir::MIRModule>("mir", std::move(mir));
    if (!backendId.empty()) {
        backends_.runChecked(backendId, session);
    }
    return session;
}

CompilationSession ModularCompilationController::compileForTarget(const ast::Node &root,
        const SemanticResolver &semantics, const target::TargetSpec &requested) const {
    return compileForTarget(root, semantics, requested, {});
}

CompilationSession ModularCompilationController::compileForTarget(const ast::Node &root,
        const SemanticResolver &semantics, const target::TargetSpec &requested,
        const CompilationOptions &options, const std::string &providerId) const {
    // Resolve before touching the AST: an unsupported/ambiguous target fails early.
    auto provider = providerId.empty() ? targets_.resolveShared(requested)
                                       : targets_.resolveShared(requested, providerId);
    auto session = compile(root, semantics);
    backend::BackendRegistry selected;
    selected.add("selected-native", std::make_shared<backend::NativeBackend>(std::move(provider), options));
    selected.runChecked("selected-native", session);
    return session;
}

} // namespace novac::compilation
