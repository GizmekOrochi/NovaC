#include "../../include/compiler/CompilationContext.hpp"

#include <stdexcept>
#include <utility>

namespace novac::compiler {

CompilationContext::CompilationContext(const language::Language &language, std::string startDomain)
    : language_{language}, startDomain_{std::move(startDomain)}, source_{}, ast_{}, hir_{}, mir_{}, runtimeValue_{}, diagnostics_{} {}

const language::Language &CompilationContext::language() const {
    return language_;
}

const std::string &CompilationContext::startDomain() const {
    return startDomain_;
}

void CompilationContext::setSource(std::string source) {
    source_ = std::move(source);
}

const std::string &CompilationContext::source() const {
    return source_;
}

void CompilationContext::setAst(ast::NodePtr ast) {
    ast_ = std::move(ast);
}

ast::NodePtr CompilationContext::ast() const {
    return ast_;
}

const ast::Node &CompilationContext::requireAst() const {
    if (!ast_) {
        throw std::runtime_error(
            "CompilationContext::requireAst: AST is not available");
    }

    return *ast_;
}

void CompilationContext::setHIR(ir::HIRModule hir) {
    hir_ = std::move(hir);
}

const std::optional<ir::HIRModule> &CompilationContext::hir() const {
    return hir_;
}

const ir::HIRModule &CompilationContext::requireHIR() const {
    if (!hir_) {
        throw std::runtime_error(
            "CompilationContext::requireHIR: HIR module is not available");
    }

    return *hir_;
}

void CompilationContext::setMIR(ir::MIRModule mir) {
    mir_ = std::move(mir);
}

const std::optional<ir::MIRModule> &CompilationContext::mir() const {
    return mir_;
}

const ir::MIRModule &CompilationContext::requireMIR() const {
    if (!mir_) {
        throw std::runtime_error(
            "CompilationContext::requireMIR: MIR module is not available");
    }

    return *mir_;
}

void CompilationContext::setRuntimeValue(runtime::Value value) {
    runtimeValue_ = std::move(value);
}

const std::optional<runtime::Value> &CompilationContext::runtimeValue() const {
    return runtimeValue_;
}

const runtime::Value &CompilationContext::requireRuntimeValue() const {
    if (!runtimeValue_) {
        throw std::runtime_error(
            "CompilationContext::requireRuntimeValue: runtime value is not available");
    }

    return *runtimeValue_;
}

diagnostics::DiagnosticEngine &CompilationContext::diagnostics() {
    return diagnostics_;
}

const diagnostics::DiagnosticEngine &CompilationContext::diagnostics() const {
    return diagnostics_;
}

} // namespace novac::compiler