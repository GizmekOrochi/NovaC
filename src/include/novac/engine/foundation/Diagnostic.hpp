#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace novac::diagnostics {

/**
 * @brief Severity level associated with a diagnostic message.
 *
 * Severity indicates whether a diagnostic is informational, potentially
 * problematic, or represents an actual processing error.
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
 *
 * A location stores both the absolute character offset and the human-readable
 * line and column position used by diagnostics.
 */
struct SourceLocation {
    /** Logical source file name. */
    std::string file{};

    /** Zero-based character offset from the beginning of the source. */
    std::size_t offset{};

    /** One-based source line. */
    int line{1};

    /** One-based source column. */
    int column{1};
};

/**
 * @brief Describes a half-open source range.
 *
 * The range starts at begin and extends up to, but does not include, end.
 * This is useful for associating diagnostics with complete tokens or syntax
 * regions instead of a single position.
 */
struct SourceSpan {
    /** First source position included in the range. */
    SourceLocation begin{};

    /** First source position after the range. */
    SourceLocation end{};
};

/**
 * @brief Stores one diagnostic message.
 *
 * A diagnostic contains its severity, human-readable message and an optional
 * source span describing where the issue occurred.
 */
struct Diagnostic {
    /** Severity of the diagnostic. */
    DiagnosticSeverity severity{DiagnosticSeverity::Error};

    /** Human-readable diagnostic message. */
    std::string message{};

    /** Source range associated with the diagnostic, if available. */
    SourceSpan span{};
};

/**
 * @brief Collects and formats diagnostics produced by the engine.
 *
 * DiagnosticEngine stores diagnostics in the order they are reported.
 * Convenience functions are provided for errors, warnings and notes, while
 * report() can be used when a complete Diagnostic object is already available.
 *
 * Reporting a diagnostic does not throw or stop processing by itself. Callers
 * can use hasErrors() to decide whether processing should continue.
 */
class DiagnosticEngine {
public:
    /**
     * @brief Adds a diagnostic to the engine.
     *
     * The diagnostic is appended to the internal list and keeps reporting
     * order.
     *
     * @param diagnostic Diagnostic to store.
     */
    void report(Diagnostic diagnostic);

    /**
     * @brief Reports an error without source location.
     *
     * This is a convenience wrapper around report() using Error severity.
     *
     * @param message Diagnostic message.
     */
    void error(std::string message);

    /**
     * @brief Reports an error with source location.
     *
     * This is a convenience wrapper around report() using Error severity.
     *
     * @param message Diagnostic message.
     * @param span Source span associated with the diagnostic.
     */
    void error(std::string message, SourceSpan span);

    /**
     * @brief Reports a warning without source location.
     *
     * This is a convenience wrapper around report() using Warning severity.
     *
     * @param message Diagnostic message.
     */
    void warning(std::string message);

    /**
     * @brief Reports a warning with source location.
     *
     * This is a convenience wrapper around report() using Warning severity.
     *
     * @param message Diagnostic message.
     * @param span Source span associated with the diagnostic.
     */
    void warning(std::string message, SourceSpan span);

    /**
     * @brief Reports an informational note without source location.
     *
     * This is a convenience wrapper around report() using Note severity.
     *
     * @param message Diagnostic message.
     */
    void note(std::string message);

    /**
     * @brief Reports an informational note with source location.
     *
     * This is a convenience wrapper around report() using Note severity.
     *
     * @param message Diagnostic message.
     * @param span Source span associated with the diagnostic.
     */
    void note(std::string message, SourceSpan span);

    /**
     * @brief Checks whether any stored diagnostic is an error.
     *
     * The diagnostic list is scanned until the first Error severity entry is
     * found.
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
     *
     * After this call, empty() returns true and previous diagnostics are no
     * longer available through diagnostics().
     */
    void clear();

    /**
     * @brief Returns all stored diagnostics.
     *
     * Diagnostics are returned in the same order in which they were reported.
     *
     * @return Diagnostics in reporting order.
     */
    const std::vector<Diagnostic> &diagnostics() const;

    /**
     * @brief Formats all diagnostics as human-readable text.
     *
     * Each diagnostic is written on its own line. When a valid line and column
     * are present, the output follows the form:
     *
     * severity: line:column: message
     *
     * Otherwise only the severity and message are printed.
     *
     * @return Formatted diagnostic output.
     */
    std::string format() const;

private:
    /**
     * @brief Converts a diagnostic severity to its display name.
     *
     * @param severity Severity to convert.
     * @return "note", "warning" or "error".
     */
    static std::string severityName(DiagnosticSeverity severity);

    std::vector<Diagnostic> diagnostics_;
};

} // namespace novac::diagnostics