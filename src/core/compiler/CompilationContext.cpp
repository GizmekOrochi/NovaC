#include "../../include/compiler/CompilationContext.hpp"

namespace novac::compiler {

CompilationContext::CompilationContext(const language::Language &language, std::string startDomain)
    : language_{language}, startDomain_{std::move(startDomain)}, source_{}, artifacts_{}, diagnostics_{} {
}

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

diagnostics::DiagnosticEngine &CompilationContext::diagnostics() {
    return diagnostics_;
}

const diagnostics::DiagnosticEngine &
CompilationContext::diagnostics() const {
    return diagnostics_;
}

} // namespace novac::compiler