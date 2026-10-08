#include "novac/engine/foundation/Diagnostic.hpp"

#include <sstream>
#include <utility>

namespace novac::diagnostics {

/**
 * @brief Implements the `report` operation.
 *
 * @param diagnostic Value supplied for `diagnostic`.
 */
void DiagnosticEngine::report(Diagnostic diagnostic) {
    diagnostics_.push_back(std::move(diagnostic));
}

/**
 * @brief Implements the `error` operation.
 *
 * @param message Value supplied for `message`.
 */
void DiagnosticEngine::error(std::string message) {
    report({DiagnosticSeverity::Error, std::move(message), {}});
}

/**
 * @brief Implements the `error` operation.
 *
 * @param message Value supplied for `message`.
 * @param span Value supplied for `span`.
 */
void DiagnosticEngine::error(std::string message, SourceSpan span) {
    report({DiagnosticSeverity::Error, std::move(message), std::move(span)});
}

/**
 * @brief Implements the `warning` operation.
 *
 * @param message Value supplied for `message`.
 */
void DiagnosticEngine::warning(std::string message) {
    report({DiagnosticSeverity::Warning, std::move(message), {}});
}

/**
 * @brief Implements the `warning` operation.
 *
 * @param message Value supplied for `message`.
 * @param span Value supplied for `span`.
 */
void DiagnosticEngine::warning(std::string message, SourceSpan span) {
    report({DiagnosticSeverity::Warning, std::move(message), std::move(span)});
}

/**
 * @brief Implements the `note` operation.
 *
 * @param message Value supplied for `message`.
 */
void DiagnosticEngine::note(std::string message) {
    report({DiagnosticSeverity::Note, std::move(message), {}});
}

/**
 * @brief Implements the `note` operation.
 *
 * @param message Value supplied for `message`.
 * @param span Value supplied for `span`.
 */
void DiagnosticEngine::note(std::string message, SourceSpan span) {
    report({DiagnosticSeverity::Note, std::move(message), std::move(span)});
}

/**
 * @brief Checks the condition represented by `hasErrors`.
 *
 * @return Value produced by the operation.
 */
bool DiagnosticEngine::hasErrors() const {
    for (const Diagnostic &diagnostic : diagnostics_) {
        if (diagnostic.severity == DiagnosticSeverity::Error) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Checks the condition represented by `empty`.
 *
 * @return Value produced by the operation.
 */
bool DiagnosticEngine::empty() const {
    return diagnostics_.empty();
}

/**
 * @brief Resets state through `clear`.
 */
void DiagnosticEngine::clear() {
    diagnostics_.clear();
}

/**
 * @brief Implements the `diagnostics` operation.
 *
 * @return Value produced by the operation.
 */
const std::vector<Diagnostic> &DiagnosticEngine::diagnostics() const {
    return diagnostics_;
}

/**
 * @brief Implements the `format` operation.
 *
 * @return Value produced by the operation.
 */
std::string DiagnosticEngine::format() const {
    std::ostringstream output{};

    for (const Diagnostic &diagnostic : diagnostics_) {
        output << severityName(diagnostic.severity) << ": ";

        if (diagnostic.span.begin.line > 0 && diagnostic.span.begin.column > 0) {
            output << diagnostic.span.begin.line << ':' << diagnostic.span.begin.column << ": ";
        }

        output << diagnostic.message;
        if (!diagnostic.code.empty()) {
            output << " [" << diagnostic.code << ']';
        }
        output << '\n';

        for (const DiagnosticLabel &label : diagnostic.labels) {
            output << "  label: ";
            if (label.span.begin.line > 0 && label.span.begin.column > 0) {
                output << label.span.begin.line << ':' << label.span.begin.column << ": ";
            }
            output << label.message << '\n';
        }

        for (const DiagnosticNote &noteValue : diagnostic.notes) {
            output << "  note: ";
            if (noteValue.span.has_value() && noteValue.span->begin.line > 0 && noteValue.span->begin.column > 0) {
                output << noteValue.span->begin.line << ':' << noteValue.span->begin.column << ": ";
            }
            output << noteValue.message << '\n';
        }

        for (const DiagnosticFixIt &fix : diagnostic.fixes) {
            output << "  fix: ";
            if (!fix.message.empty()) {
                output << fix.message << " -> ";
            }
            output << fix.replacement << '\n';
        }
    }

    return output.str();
}

/**
 * @brief Implements the `severityName` operation.
 *
 * @param severity Value supplied for `severity`.
 * @return Value produced by the operation.
 */
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
