#include "novac/engine/controllers/CompilerController.hpp"

#include "novac/engine/controllers/LanguageController.hpp"

#include "novac/engine/compilation/compiler/Compiler.hpp"
#include "novac/engine/compilation/compiler/Pass.hpp"

#include <stdexcept>
#include <utility>

namespace novac::controllers {

struct CompilerController::Impl final {
    Impl(
        const LanguageController &languageController,
        std::string startDomain)
        : compiler{languageController.language(), std::move(startDomain)}
    {
    }

    compiler::Compiler compiler;
};

CompilerController::CompilerController(
    const LanguageController &language,
    std::string startDomain)
    : impl_{std::make_unique<Impl>(language, std::move(startDomain))}
{
}

CompilerController::~CompilerController() = default;

CompilerController::CompilerController(CompilerController &&other) noexcept = default;

CompilerController &CompilerController::operator=(CompilerController &&other) noexcept = default;

void CompilerController::addPass(std::unique_ptr<compiler::Pass> pass)
{
    if (!pass) {
        throw std::runtime_error{"CompilerController::addPass: pass cannot be null"};
    }

    impl_->compiler.addPass(std::move(pass));
}

void CompilerController::addCorePipeline(const CompilationArtifacts &artifacts)
{
    impl_->compiler.addPass<compiler::ParsePass>(artifacts.ast);
    impl_->compiler.addPass<compiler::AstValidationPass>(artifacts.ast);
    impl_->compiler.addPass<compiler::SemanticPass>(artifacts.ast, artifacts.semantic);
    impl_->compiler.addPass<compiler::HIRLoweringPass>(artifacts.ast, artifacts.hir);
    impl_->compiler.addPass<compiler::MIRLoweringPass>(artifacts.hir, artifacts.mir);
}

void CompilerController::addExecutionPipeline(const CompilationArtifacts &artifacts)
{
    impl_->compiler.addPass<compiler::RuntimePass>(artifacts.ast, artifacts.result);
}

void CompilerController::addFullPipeline(const CompilationArtifacts &artifacts)
{
    addCorePipeline(artifacts);
    addExecutionPipeline(artifacts);
}

std::unique_ptr<compiler::CompilationContext> CompilerController::run(std::string source) const
{
    return std::make_unique<compiler::CompilationContext>(impl_->compiler.run(std::move(source)));
}

} // namespace novac::controllers
