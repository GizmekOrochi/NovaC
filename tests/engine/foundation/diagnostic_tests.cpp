#include "../../tester.hpp"
#include "novac/engine/foundation/Diagnostic.hpp"

#include <string>

namespace {

using novac::diagnostics::Diagnostic;
using novac::diagnostics::DiagnosticEngine;
using novac::diagnostics::DiagnosticSeverity;
using novac::diagnostics::SourceLocation;
using novac::diagnostics::SourceSpan;

SourceSpan span(std::string file = "sample.novac", std::size_t beginOffset = 2, int beginLine = 3, int beginColumn = 4, std::size_t endOffset = 8, int endLine = 3, int endColumn = 10) {
    return SourceSpan{
        SourceLocation{file, beginOffset, beginLine, beginColumn},
        SourceLocation{file, endOffset, endLine, endColumn}
    };
}

} // namespace

TEST(SourceLocation, DefaultConstruction) {
    SourceLocation location{};

    CHECK(location.file.empty());
    CHECK(location.offset == 0);
    CHECK(location.line == 1);
    CHECK(location.column == 1);
}

TEST(SourceLocation, StoresValues) {
    SourceLocation location{"main.novac", 12, 3, 9};

    CHECK(location.file == "main.novac");
    CHECK(location.offset == 12);
    CHECK(location.line == 3);
    CHECK(location.column == 9);
}

TEST(SourceSpan, DefaultConstruction) {
    SourceSpan sourceSpan{};

    CHECK(sourceSpan.begin.line == 1);
    CHECK(sourceSpan.begin.column == 1);
    CHECK(sourceSpan.end.line == 1);
    CHECK(sourceSpan.end.column == 1);
}

TEST(SourceSpan, StoresBeginAndEndLocations) {
    SourceSpan sourceSpan{
        SourceLocation{"main.novac", 1, 2, 3},
        SourceLocation{"main.novac", 4, 2, 6}
    };

    CHECK(sourceSpan.begin.file == "main.novac");
    CHECK(sourceSpan.begin.offset == 1);
    CHECK(sourceSpan.begin.line == 2);
    CHECK(sourceSpan.begin.column == 3);

    CHECK(sourceSpan.end.file == "main.novac");
    CHECK(sourceSpan.end.offset == 4);
    CHECK(sourceSpan.end.line == 2);
    CHECK(sourceSpan.end.column == 6);
}

TEST(Diagnostic, DefaultConstruction) {
    Diagnostic diagnostic{};

    CHECK(diagnostic.severity == DiagnosticSeverity::Error);
    CHECK(diagnostic.message.empty());
    CHECK(diagnostic.span.begin.line == 1);
    CHECK(diagnostic.span.begin.column == 1);
}

TEST(Diagnostic, StoresValues) {
    Diagnostic diagnostic{
        DiagnosticSeverity::Warning,
        "careful",
        span("main.novac", 0, 1, 2, 5, 1, 7)
    };

    CHECK(diagnostic.severity == DiagnosticSeverity::Warning);
    CHECK(diagnostic.message == "careful");
    CHECK(diagnostic.span.begin.file == "main.novac");
    CHECK(diagnostic.span.begin.line == 1);
    CHECK(diagnostic.span.begin.column == 2);
    CHECK(diagnostic.span.end.column == 7);
}

TEST(DiagnosticEngine, StartsEmpty) {
    DiagnosticEngine engine{};

    CHECK(engine.empty());
    CHECK(!engine.hasErrors());
    CHECK(engine.diagnostics().empty());
    CHECK(engine.format().empty());
}

TEST(DiagnosticEngine, ReportsDiagnosticDirectly) {
    DiagnosticEngine engine{};

    engine.report(Diagnostic{
        DiagnosticSeverity::Note,
        "hello",
        span()
    });

    CHECK(!engine.empty());
    CHECK(!engine.hasErrors());
    CHECK(engine.diagnostics().size() == 1);
    CHECK(engine.diagnostics()[0].severity == DiagnosticSeverity::Note);
    CHECK(engine.diagnostics()[0].message == "hello");
}

TEST(DiagnosticEngine, ReportsErrorWithoutSpan) {
    DiagnosticEngine engine{};

    engine.error("boom");

    CHECK(!engine.empty());
    CHECK(engine.hasErrors());
    CHECK(engine.diagnostics().size() == 1);
    CHECK(engine.diagnostics()[0].severity == DiagnosticSeverity::Error);
    CHECK(engine.diagnostics()[0].message == "boom");
}

TEST(DiagnosticEngine, ReportsErrorWithSpan) {
    DiagnosticEngine engine{};

    engine.error("boom", span("main.novac", 0, 7, 11, 3, 7, 14));

    CHECK(engine.hasErrors());
    CHECK(engine.diagnostics().size() == 1);
    CHECK(engine.diagnostics()[0].severity == DiagnosticSeverity::Error);
    CHECK(engine.diagnostics()[0].span.begin.line == 7);
    CHECK(engine.diagnostics()[0].span.begin.column == 11);
}

TEST(DiagnosticEngine, ReportsWarningWithoutSpan) {
    DiagnosticEngine engine{};

    engine.warning("careful");

    CHECK(!engine.empty());
    CHECK(!engine.hasErrors());
    CHECK(engine.diagnostics().size() == 1);
    CHECK(engine.diagnostics()[0].severity == DiagnosticSeverity::Warning);
    CHECK(engine.diagnostics()[0].message == "careful");
}

TEST(DiagnosticEngine, ReportsWarningWithSpan) {
    DiagnosticEngine engine{};

    engine.warning("careful", span("main.novac", 0, 2, 5, 4, 2, 9));

    CHECK(!engine.hasErrors());
    CHECK(engine.diagnostics().size() == 1);
    CHECK(engine.diagnostics()[0].severity == DiagnosticSeverity::Warning);
    CHECK(engine.diagnostics()[0].span.begin.line == 2);
    CHECK(engine.diagnostics()[0].span.begin.column == 5);
}

TEST(DiagnosticEngine, ReportsNoteWithoutSpan) {
    DiagnosticEngine engine{};

    engine.note("info");

    CHECK(!engine.empty());
    CHECK(!engine.hasErrors());
    CHECK(engine.diagnostics().size() == 1);
    CHECK(engine.diagnostics()[0].severity == DiagnosticSeverity::Note);
    CHECK(engine.diagnostics()[0].message == "info");
}

TEST(DiagnosticEngine, ReportsNoteWithSpan) {
    DiagnosticEngine engine{};

    engine.note("info", span("main.novac", 0, 4, 6, 8, 4, 14));

    CHECK(!engine.hasErrors());
    CHECK(engine.diagnostics().size() == 1);
    CHECK(engine.diagnostics()[0].severity == DiagnosticSeverity::Note);
    CHECK(engine.diagnostics()[0].span.begin.line == 4);
    CHECK(engine.diagnostics()[0].span.begin.column == 6);
}

TEST(DiagnosticEngine, HasErrorsOnlyWhenErrorExists) {
    DiagnosticEngine engine{};

    engine.note("info");
    engine.warning("careful");

    CHECK(!engine.hasErrors());

    engine.error("boom");

    CHECK(engine.hasErrors());
}

TEST(DiagnosticEngine, KeepsDiagnosticsInReportOrder) {
    DiagnosticEngine engine{};

    engine.note("first");
    engine.warning("second");
    engine.error("third");

    CHECK(engine.diagnostics().size() == 3);
    CHECK(engine.diagnostics()[0].message == "first");
    CHECK(engine.diagnostics()[1].message == "second");
    CHECK(engine.diagnostics()[2].message == "third");
}

TEST(DiagnosticEngine, ClearsDiagnostics) {
    DiagnosticEngine engine{};

    engine.error("boom");
    engine.warning("careful");

    CHECK(!engine.empty());
    CHECK(engine.hasErrors());

    engine.clear();

    CHECK(engine.empty());
    CHECK(!engine.hasErrors());
    CHECK(engine.diagnostics().empty());
}

TEST(DiagnosticEngine, FormatsSeveritiesAndMessages) {
    DiagnosticEngine engine{};

    engine.note("info");
    engine.warning("careful");
    engine.error("boom");

    const std::string formatted{engine.format()};

    CHECK(formatted.find("note:") != std::string::npos);
    CHECK(formatted.find("info") != std::string::npos);

    CHECK(formatted.find("warning:") != std::string::npos);
    CHECK(formatted.find("careful") != std::string::npos);

    CHECK(formatted.find("error:") != std::string::npos);
    CHECK(formatted.find("boom") != std::string::npos);
}

TEST(DiagnosticEngine, FormatsSourceLocationWhenPresent) {
    DiagnosticEngine engine{};

    engine.error("boom", span("main.novac", 0, 12, 34, 1, 12, 35));

    const std::string formatted{engine.format()};

    CHECK(formatted.find("error: 12:34: boom") != std::string::npos);
}

TEST(DiagnosticEngine, DoesNotFormatLocationWhenLineOrColumnIsZero) {
    DiagnosticEngine engine{};

    SourceSpan noLocation{};
    noLocation.begin.line = 0;
    noLocation.begin.column = 0;

    engine.error("boom", noLocation);

    const std::string formatted{engine.format()};

    CHECK(formatted.find("error: boom") != std::string::npos);
    CHECK(formatted.find("0:0") == std::string::npos);
}
