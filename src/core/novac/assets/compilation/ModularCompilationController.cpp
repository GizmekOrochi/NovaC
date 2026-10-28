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

} // namespace novac::compilation
