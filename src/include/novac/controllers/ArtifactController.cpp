#include "novac/engine/controllers/ArtifactController.hpp"

#include "novac/engine/compilation/compiler/CompilationContext.hpp"

#include <stdexcept>
#include <string>

namespace novac::controllers {

ArtifactController::ArtifactController(compiler::CompilationContext &context)
    : context_{&context}
{
}

compiler::CompilationContext &ArtifactController::context() const
{
    if (context_ == nullptr) {
        throw std::runtime_error{"ArtifactController::context: context is null"};
    }

    return *context_;
}

} // namespace novac::controllers
