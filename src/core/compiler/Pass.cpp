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
            throw std::runtime_error("PassManager::run: pass '" + pass->name() + "' failed with diagnostics:\n" + context.diagnostics().format());
        }
    }
}

std::string ParsePass::name() const {
    return "parse";
}

void ParsePass::run(CompilationContext &context) const {
    lexer::Lexer lexer{context.language().lexer};
    std::vector<token::Token> tokens{lexer.tokenize(context.source())};

    parser::Parser parser{
        context.language().parser,
        context.startDomain()
    };

    ast::NodePtr ast{parser.parse(std::move(tokens))};

    if (!ast) {
        throw std::runtime_error(
            "ParsePass::run: parser returned null AST");
    }

    context.setAst(ast);
}

std::string AstValidationPass::name() const {
    return "ast.validation";
}

void AstValidationPass::run(CompilationContext &context) const {
    context.language().nodes.validate(context.requireAst());
}

std::string HIRLoweringPass::name() const {
    return "hir.lowering";
}

void HIRLoweringPass::run(CompilationContext &context) const {
    ir::ASTLoweringPass pass{context.language().lowering};

    context.setHIR(pass.lower(context.requireAst()));
}

std::string MIRLoweringPass::name() const {
    return "mir.lowering";
}

void MIRLoweringPass::run(CompilationContext &context) const {
    ir::HIRLoweringPass pass{context.language().lowering};

    context.setMIR(pass.lower(context.requireHIR()));
}

std::string RuntimePass::name() const {
    return "runtime";
}

void RuntimePass::run(CompilationContext &context) const {
    runtime::Runtime runtime{context.language().runtime};

    context.setRuntimeValue(runtime.run(context.requireAst()));
}

BackendEmitPass::BackendEmitPass(std::string backendName)
    : backendName_{std::move(backendName)} {}

std::string BackendEmitPass::name() const {
    return "backend.emit." + backendName_;
}

void BackendEmitPass::run(CompilationContext &context) const {
    std::unique_ptr<backend::Backend> backend{context.language().backends.create(backendName_)};

    if (!backend) {
        throw std::runtime_error(
            "BackendEmitPass::run: backend not found '" + backendName_ + "'");
    }

    if (!context.mir()) {
        MIRLoweringPass loweringPass{};

        loweringPass.run(context);
    }

    backend->emit(context.requireMIR());
}

} // namespace novac::compiler