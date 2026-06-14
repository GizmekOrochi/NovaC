#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace novac::diagnostics {

/**
 * @brief Severity level associated with a diagnostic message.
 */
enum class DiagnosticSeverity {
    /**
     * @brief Informational diagnostic that does not indicate a problem.
     */
    Note,

    /**
     * @brief Non-fatal diagnostic indicating a potential issue.
     */
    Warning,

    /**
     * @brief Diagnostic indicating a compilation or processing error.
     */
    Error
};

/**
 * @brief Identifies a position in a source file.
 */
struct SourceLocation {
    std::string file{};
    std::size_t offset{};
    int line{1};
    int column{1};
};

/**
 * @brief Describes a half-open source range.
 */
struct SourceSpan {
    SourceLocation begin{};
    SourceLocation end{};
};

/**
 * @brief Stores a diagnostic message with severity and optional source span.
 */
struct Diagnostic {
    DiagnosticSeverity severity{DiagnosticSeverity::Error};
    std::string message{};
    SourceSpan span{};
};

/**
 * @brief Collects and formats diagnostics produced by the engine.
 */
class DiagnosticEngine {
public:
    /**
     * @brief Adds a diagnostic to the engine.
     *
     * @param diagnostic Diagnostic to store.
     */
    void report(Diagnostic diagnostic);

    /**
     * @brief Reports an error without source location.
     *
     * @param message Diagnostic message.
     */
    void error(std::string message);

    /**
     * @brief Reports an error with source location.
     *
     * @param message Diagnostic message.
     * @param span Source span associated with the diagnostic.
     */
    void error(std::string message, SourceSpan span);

    /**
     * @brief Reports a warning without source location.
     *
     * @param message Diagnostic message.
     */
    void warning(std::string message);

    /**
     * @brief Reports a warning with source location.
     *
     * @param message Diagnostic message.
     * @param span Source span associated with the diagnostic.
     */
    void warning(std::string message, SourceSpan span);

    /**
     * @brief Reports an informational note without source location.
     *
     * @param message Diagnostic message.
     */
    void note(std::string message);

    /**
     * @brief Reports an informational note with source location.
     *
     * @param message Diagnostic message.
     * @param span Source span associated with the diagnostic.
     */
    void note(std::string message, SourceSpan span);

    /**
     * @brief Checks whether any stored diagnostic is an error.
     *
     * @return True if at least one error diagnostic is present.
     */
    bool hasErrors() const;

    /**
     * @brief Checks whether no diagnostics are stored.
     *
     * @return True if the diagnostic list is empty.
     */
    bool empty() const;

    /**
     * @brief Removes all stored diagnostics.
     */
    void clear();

    /**
     * @brief Returns all stored diagnostics.
     *
     * @return Diagnostics in reporting order.
     */
    const std::vector<Diagnostic> &diagnostics() const;

    /**
     * @brief Formats all diagnostics as human-readable text.
     *
     * @return Formatted diagnostic output.
     */
    std::string format() const;

private:
    static std::string severityName(DiagnosticSeverity severity);

    std::vector<Diagnostic> diagnostics_;
};

} // namespace novac::diagnostics