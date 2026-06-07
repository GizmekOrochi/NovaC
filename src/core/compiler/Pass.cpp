#include "../../include/compiler/Pass.hpp"

#include "../../include/lexer/Lexer.hpp"
#include "../../include/parser/Parser.hpp"
#include "../../include/runtime/Runtime.hpp"

#include <stdexcept>
#include <utility>

namespace novac::compiler {

Pass::~Pass() = default;

PassManager::PassManager() : passes_{} {}

void PassManager::add(std::unique_ptr<Pass> pass) {
    if (!pass) {
        throw std::runtime_error("PassManager::add: pass is null");
    }

    passes_.push_back(std::move(pass));
}

void PassManager::run(CompilationContext &context) const {
    for (const std::unique_ptr<Pass> &pass : passes_) {
        pass->run(context);

        if (context.diagnostics().hasErrors()) {
            throw std::runtime_error( "PassManager::run: pass '" + pass->name() + "' failed with diagnostics:\n" + context.diagnostics().format());
        }
    }
}

ParsePass::ParsePass(std::string outputArtifact)
    : outputArtifact_{std::move(outputArtifact)} {
}

std::string ParsePass::name() const {
    return "parse";
}

std::string AstValidationPass::name() const {
    return "ast.validation";
}

std::string HIRLoweringPass::name() const {
    return "hir.lowering";
}

std::string MIRLoweringPass::name() const {
    return "mir.lowering";
}

std::string RuntimePass::name() const {
    return "runtime";
}

std::string BackendEmitPass::name() const {
    return "backend.emit." + backendName_;
}

void ParsePass::run(CompilationContext &context) const {
    lexer::Lexer lexer{context.language().lexer};

    auto tokens{lexer.tokenize(context.source())};

    parser::Parser parser{context.language().parser, context.startDomain()};

    auto ast{parser.parse(std::move(tokens))};

    if (!ast) {
        throw std::runtime_error("ParsePass::run: parser returned null AST");
    }

    context.setArtifact(outputArtifact_, ast);
}

AstValidationPass::AstValidationPass(std::string astArtifact)
    : astArtifact_{std::move(astArtifact)} {
}

void AstValidationPass::run(CompilationContext &context) const {
    const ast::NodePtr &root{context.requireArtifact<ast::NodePtr>(astArtifact_)};

    context.language().nodes.validate(*root);
}

HIRLoweringPass::HIRLoweringPass(std::string inputAst, std::string outputHir)
    : inputAst_{std::move(inputAst)}, outputHir_{std::move(outputHir)} {
}

void HIRLoweringPass::run(CompilationContext &context) const {
    const ast::NodePtr &root{context.requireArtifact<ast::NodePtr>(inputAst_)};

    ir::ASTLoweringPass pass{context.language().lowering};

    context.setArtifact(outputHir_, pass.lower(*root));
}

MIRLoweringPass::MIRLoweringPass(std::string inputHir, std::string outputMir)
    : inputHir_{std::move(inputHir)}, outputMir_{std::move(outputMir)} {
}

void MIRLoweringPass::run(CompilationContext &context) const {
    const ir::HIRModule &hir{context.requireArtifact<ir::HIRModule>(inputHir_)};

    ir::HIRLoweringPass pass{context.language().lowering};

    context.setArtifact(outputMir_, pass.lower(hir));
}

RuntimePass::RuntimePass(std::string inputAst, std::string outputValue)
    : inputAst_{std::move(inputAst)}, outputValue_{std::move(outputValue)} {
}

void RuntimePass::run(
    CompilationContext &context) const {
    const ast::NodePtr &root{context.requireArtifact<ast::NodePtr>(inputAst_)};

    runtime::Runtime runtime{context.language().runtime};

    context.setArtifact(outputValue_, runtime.eval(*root));
}

BackendEmitPass::BackendEmitPass(std::string backendName, std::string mirArtifact)
    : backendName_{std::move(backendName)}, mirArtifact_{std::move(mirArtifact)} {
}

void BackendEmitPass::run(CompilationContext &context) const {
    auto backend{
        context.language().backends.create(
            backendName_)
    };

    if (!backend) {
        throw std::runtime_error(
            "BackendEmitPass::run: backend not found '" +
            backendName_ + "'");
    }

    const ir::MIRModule &mir{
        context.requireArtifact<ir::MIRModule>(
            mirArtifact_)
    };

    backend->emit(mir);
}

} // namespace novac::compiler