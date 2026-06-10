#include "../../../../../include/novac/engine/compilation/compiler/Pass.hpp"

#include <stdexcept>
#include <utility>

namespace novac::compiler {

Pass::~Pass() = default;

PassManager::PassManager()
    : passes_{} {}

void PassManager::add(std::unique_ptr<Pass> pass) {
    if (!pass)
        throw std::runtime_error("PassManager::add: pass is null");

    passes_.push_back(std::move(pass));
}

void PassManager::run(CompilationContext &context) const {
    for (const std::unique_ptr<Pass> &pass : passes_) {
        pass->run(context);

        if (context.diagnostics().hasErrors())
            throw std::runtime_error("PassManager::run: pass '" + pass->name() + "' failed with diagnostics:\n" + context.diagnostics().format());
    }
}

ParsePass::ParsePass(std::string outputArtifact)
    : outputArtifact_{std::move(outputArtifact)} {}

std::string ParsePass::name() const {
    return "parse";
}

void ParsePass::run(CompilationContext &context) const {
    lexer::Lexer lexer{context.language().lexer};
    auto tokens{lexer.tokenize(context.source())};

    parser::Parser parser{context.language().parser, context.startDomain()};

    ast::NodePtr root{parser.parse(std::move(tokens))};

    if (!root)
        throw std::runtime_error("ParsePass::run: parser returned null AST");

    context.setArtifact(outputArtifact_, root);
}

AstValidationPass::AstValidationPass(std::string astArtifact)
    : astArtifact_{std::move(astArtifact)} {}

std::string AstValidationPass::name() const {
    return "ast.validation";
}

void AstValidationPass::run(CompilationContext &context) const {
    const ast::NodePtr &root{context.requireArtifact<ast::NodePtr>(astArtifact_)};
    context.language().nodes.validate(*root);
}

SemanticPass::SemanticPass(std::string inputAst, std::string outputSemantic)
    : inputAst_{std::move(inputAst)}, outputSemantic_{std::move(outputSemantic)} {}

std::string SemanticPass::name() const {
    return "semantic";
}

void SemanticPass::run(CompilationContext &context) const {
    const ast::NodePtr &root{context.requireArtifact<ast::NodePtr>(inputAst_)};

    semantic::SemanticContext semanticContext{};

    context.language().semantic.analyze(root, semanticContext);
    context.setArtifact(outputSemantic_,std::move(semanticContext));
}

HIRLoweringPass::HIRLoweringPass(std::string inputAst, std::string outputHir)
    : inputAst_{std::move(inputAst)}, outputHir_{std::move(outputHir)} {}

std::string HIRLoweringPass::name() const {
    return "hir.lowering";
}

void HIRLoweringPass::run(CompilationContext &context) const {
    const ast::NodePtr &root{context.requireArtifact<ast::NodePtr>(inputAst_)};
    ir::ASTLoweringPass pass{context.language().lowering};
    context.setArtifact(outputHir_, pass.lower(*root));
}

MIRLoweringPass::MIRLoweringPass(std::string inputHir, std::string outputMir)
    : inputHir_{std::move(inputHir)}, outputMir_{std::move(outputMir)} {}

std::string MIRLoweringPass::name() const {
    return "mir.lowering";
}

void MIRLoweringPass::run(CompilationContext &context) const {
    const ir::HIRModule &hir{context.requireArtifact<ir::HIRModule>(inputHir_)};
    ir::HIRLoweringPass pass{context.language().lowering};
    context.setArtifact(outputMir_, pass.lower(hir));
}

RuntimePass::RuntimePass(std::string inputAst, std::string outputValue)
    : inputAst_{std::move(inputAst)}, outputValue_{std::move(outputValue)} {}

std::string RuntimePass::name() const {
    return "runtime";
}

void RuntimePass::run(CompilationContext &context) const {
    const ast::NodePtr &root{context.requireArtifact<ast::NodePtr>(inputAst_)};
    runtime::Runtime runtime{context.language().runtime};
    context.setArtifact(outputValue_, runtime.eval(*root));
}

} // namespace novac::compiler