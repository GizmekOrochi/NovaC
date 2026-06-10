#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace novac::diagnostics {

enum class DiagnosticSeverity {
    Note,
    Warning,
    Error
};

struct SourceLocation {
    std::string file{};
    std::size_t offset{};
    int line{1};
    int column{1};
};

struct SourceSpan {
    SourceLocation begin{};
    SourceLocation end{};
};

struct Diagnostic {
    DiagnosticSeverity severity{DiagnosticSeverity::Error};
    std::string message{};
    SourceSpan span{};
};

class DiagnosticEngine {
public:
    void report(Diagnostic diagnostic);

    void error(std::string message);
    void error(std::string message, SourceSpan span);

    void warning(std::string message);
    void warning(std::string message, SourceSpan span);

    void note(std::string message);
    void note(std::string message, SourceSpan span);

    bool hasErrors() const;
    bool empty() const;
    void clear();

    const std::vector<Diagnostic> &diagnostics() const;

    std::string format() const;

private:
    static std::string severityName(DiagnosticSeverity severity);

    std::vector<Diagnostic> diagnostics_;
};

} // namespace novac::diagnostics