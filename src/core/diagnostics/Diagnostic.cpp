#include "../../include/diagnostics/Diagnostic.hpp"

#include <sstream>
#include <utility>

namespace novac::diagnostics {

void DiagnosticEngine::report(Diagnostic diagnostic) {
    diagnostics_.push_back(std::move(diagnostic));
}

void DiagnosticEngine::error(std::string message) {
    report({DiagnosticSeverity::Error, std::move(message), {}});
}

void DiagnosticEngine::warning(std::string message) {
    report({DiagnosticSeverity::Warning, std::move(message), {}});
}

void DiagnosticEngine::note(std::string message) {
    report({DiagnosticSeverity::Note, std::move(message), {}});
}

bool DiagnosticEngine::hasErrors() const {
    for (const Diagnostic &diagnostic : diagnostics_) {
        if (diagnostic.severity == DiagnosticSeverity::Error) {
            return true;
        }
    }

    return false;
}

bool DiagnosticEngine::empty() const {
    return diagnostics_.empty();
}

const std::vector<Diagnostic> &DiagnosticEngine::diagnostics() const {
    return diagnostics_;
}

std::string DiagnosticEngine::format() const {
    std::ostringstream output{};

    for (const Diagnostic &diagnostic : diagnostics_) {
        output
            << severityName(diagnostic.severity)
            << ": "
            << diagnostic.message
            << '\n';
    }

    return output.str();
}

std::string DiagnosticEngine::severityName(DiagnosticSeverity severity) {
    if (severity == DiagnosticSeverity::Note) {
        return "note";
    }

    if (severity == DiagnosticSeverity::Warning) {
        return "warning";
    }

    return "error";
}

} // namespace novac::diagnostics