#pragma once

#include "../ast/Node.hpp"
#include "../backend/Backend.hpp"
#include "../diagnostics/Diagnostic.hpp"
#include "../ir/IR.hpp"
#include "../language/Language.hpp"
#include "../runtime/Runtime.hpp"

#include <string>

namespace novac::compiler {

class Compiler {
public:
    Compiler(
        const language::Language &language,
        std::string startDomain);

    ast::NodePtr parse(const std::string &source) const;

    ir::HIRModule lowerToHIR(const ast::Node &root) const;
    ir::HIRModule lowerToHIR(const std::string &source) const;

    ir::MIRModule lowerToMIR(const ast::Node &root) const;
    ir::MIRModule lowerToMIR(const std::string &source) const;

    runtime::Value run(const ast::Node &root) const;
    runtime::Value run(const std::string &source) const;

    bool emit(const ast::Node &root, const std::string &backendName) const;
    bool emit(const std::string &source, const std::string &backendName) const;

private:
    const language::Language &language_;
    std::string startDomain_;
};

} // namespace novac::compiler