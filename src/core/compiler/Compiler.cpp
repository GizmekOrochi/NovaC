#include "../../include/compiler/Compiler.hpp"

#include "../../include/compiler/Pass.hpp"

#include <utility>

namespace novac::compiler {

Compiler::Compiler(const language::Language &language, std::string startDomain) : language_{language}, startDomain_{std::move(startDomain)} {}

ast::NodePtr Compiler::parse(const std::string &source) const {
    CompilationContext context{language_, startDomain_};
    context.setSource(source);

    PassManager passes{};
    passes.add<ParsePass>();
    passes.add<AstValidationPass>();
    passes.run(context);

    return context.ast();
}

ir::HIRModule Compiler::lowerToHIR(const ast::Node &root) const {
    language_.nodes.validate(root);

    ir::ASTLoweringPass pass{language_.lowering};

    return pass.lower(root);
}

ir::HIRModule Compiler::lowerToHIR(const std::string &source) const {
    CompilationContext context{language_, startDomain_};
    context.setSource(source);

    PassManager passes{};
    passes.add<ParsePass>();
    passes.add<AstValidationPass>();
    passes.add<HIRLoweringPass>();
    passes.run(context);

    return context.requireHIR();
}

ir::MIRModule Compiler::lowerToMIR(const ast::Node &root) const {
    language_.nodes.validate(root);

    ir::ASTLoweringPass astPass{language_.lowering};
    ir::HIRLoweringPass hirPass{language_.lowering};

    const ir::HIRModule hir{astPass.lower(root)};

    return hirPass.lower(hir);
}

ir::MIRModule Compiler::lowerToMIR(const std::string &source) const {
    CompilationContext context{language_, startDomain_};
    context.setSource(source);

    PassManager passes{};
    passes.add<ParsePass>();
    passes.add<AstValidationPass>();
    passes.add<HIRLoweringPass>();
    passes.add<MIRLoweringPass>();
    passes.run(context);

    return context.requireMIR();
}

runtime::Value Compiler::run(const ast::Node &root) const {
    language_.nodes.validate(root);

    runtime::Runtime runtime{language_.runtime};

    return runtime.eval(root);
}

runtime::Value Compiler::run(const std::string &source) const {
    CompilationContext context{language_, startDomain_};
    context.setSource(source);

    PassManager passes{};
    passes.add<ParsePass>();
    passes.add<AstValidationPass>();
    passes.add<RuntimePass>();
    passes.run(context);

    return context.requireRuntimeValue();
}

bool Compiler::emit(const ast::Node &root, const std::string &backendName) const {
    language_.nodes.validate(root);

    const ir::MIRModule mir{lowerToMIR(root)};
    std::unique_ptr<backend::Backend> backend{language_.backends.create(backendName)};

    if (!backend) {
        return false;
    }

    backend->emit(mir);

    return true;
}

bool Compiler::emit(const std::string &source, const std::string &backendName) const {
    CompilationContext context{language_, startDomain_};
    context.setSource(source);

    PassManager passes{};
    passes.add<ParsePass>();
    passes.add<AstValidationPass>();
    passes.add<HIRLoweringPass>();
    passes.add<MIRLoweringPass>();
    passes.add<BackendEmitPass>(backendName);
    passes.run(context);

    return true;
}

} // namespace novac::compiler