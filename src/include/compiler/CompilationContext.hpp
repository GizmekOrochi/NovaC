#pragma once

#include "../ast/Node.hpp"
#include "../diagnostics/Diagnostic.hpp"
#include "../ir/IR.hpp"
#include "../language/Language.hpp"
#include "../runtime/Runtime.hpp"

#include <optional>
#include <string>

namespace novac::compiler {

class CompilationContext {
public:
    CompilationContext(
        const language::Language &language,
        std::string startDomain);

    const language::Language &language() const;

    const std::string &startDomain() const;

    void setSource(std::string source);
    const std::string &source() const;

    void setAst(ast::NodePtr ast);
    ast::NodePtr ast() const;
    const ast::Node &requireAst() const;

    void setHIR(ir::HIRModule hir);
    const std::optional<ir::HIRModule> &hir() const;
    const ir::HIRModule &requireHIR() const;

    void setMIR(ir::MIRModule mir);
    const std::optional<ir::MIRModule> &mir() const;
    const ir::MIRModule &requireMIR() const;

    void setRuntimeValue(runtime::Value value);
    const std::optional<runtime::Value> &runtimeValue() const;
    const runtime::Value &requireRuntimeValue() const;

    diagnostics::DiagnosticEngine &diagnostics();
    const diagnostics::DiagnosticEngine &diagnostics() const;

private:
    const language::Language &language_;
    std::string startDomain_;
    std::string source_;
    ast::NodePtr ast_;
    std::optional<ir::HIRModule> hir_;
    std::optional<ir::MIRModule> mir_;
    std::optional<runtime::Value> runtimeValue_;
    diagnostics::DiagnosticEngine diagnostics_;
};

} // namespace novac::compiler