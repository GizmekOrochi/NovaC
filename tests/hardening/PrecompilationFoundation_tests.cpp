#include "../tester.hpp"
#include "novac/engine/EngineController.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace {
using namespace novac;

template <class Fn> bool fails(Fn &&fn) {
    try { fn(); } catch (const std::runtime_error &) { return true; }
    return false;
}

} // namespace

TEST(PrecompilationRegistries, StrictKeywordAndSymbolDuplicatesAreRejected) {
    lexer::LexerRegistry registry{registry::DuplicatePolicy::Error};
    CHECK(registry.keyword("if") == registry::RegisterStatus::Inserted);
    CHECK(registry.symbol(";") == registry::RegisterStatus::Inserted);
    CHECK(fails([&] { registry.keyword("if"); }));
    CHECK(fails([&] { registry.symbol(";"); }));
    CHECK(registry.symbols().size() == 1);
}

TEST(PrecompilationRegistries, IgnorePolicyKeepsExistingTokens) {
    lexer::LexerRegistry registry{registry::DuplicatePolicy::Ignore};
    CHECK(registry.keyword("if") == registry::RegisterStatus::Inserted);
    CHECK(registry.keyword("if") == registry::RegisterStatus::Ignored);
    CHECK(registry.symbol(";") == registry::RegisterStatus::Inserted);
    CHECK(registry.symbol(";") == registry::RegisterStatus::Ignored);
    CHECK(registry.symbols().size() == 1);
}

TEST(PrecompilationRegistries, ReplacePolicyIsObservableAndIdempotentForSetEntries) {
    lexer::LexerRegistry registry{registry::DuplicatePolicy::Replace};
    CHECK(registry.keyword("if") == registry::RegisterStatus::Inserted);
    CHECK(registry.keyword("if") == registry::RegisterStatus::Replaced);
    CHECK(registry.symbol("==") == registry::RegisterStatus::Inserted);
    CHECK(registry.symbol("=") == registry::RegisterStatus::Inserted);
    CHECK(registry.symbol("==") == registry::RegisterStatus::Replaced);
    CHECK(registry.symbols().size() == 2);
    CHECK(registry.symbols()[0] == "==");
}

TEST(PrecompilationRegistries, EngineSharedTokensRequireExplicitIntent) {
    controllers::EngineController engine{};
    CHECK(engine.ensureKeyword("while") == registry::RegisterStatus::Inserted);
    CHECK(engine.ensureKeyword("while") == registry::RegisterStatus::Ignored);
    CHECK(fails([&] { engine.keyword("while"); }));
    CHECK(engine.ensureSymbol(";") == registry::RegisterStatus::Inserted);
    CHECK(engine.ensureSymbol(";") == registry::RegisterStatus::Ignored);
    CHECK(fails([&] { engine.symbol(";"); }));
    CHECK(fails([&] { engine.ensureSymbol(""); }));
}

TEST(PrecompilationRegistries, FeatureInstallationRollsBackOnConflictingToken) {
    controllers::EngineController engine{};
    engine.symbol(";");
    controllers::EngineFeature feature{"broken"};
    feature.provides("uncommitted").onInstall([](controllers::EngineController &e) {
        e.keyword("temporary");
        e.symbol(";"); // strict duplicate; must roll back temporary keyword
    });
    CHECK(fails([&] { engine.install(feature); }));
    CHECK(!engine.hasFeature("broken"));
    CHECK(!engine.hasCapability("uncommitted"));
    CHECK(!engine.lexer().isKeyword("temporary"));
    CHECK(engine.lexer().symbols().size() == 1);
}

TEST(PrecompilationDiagnostics, FormatsAllSourceLocationsAndCodes) {
    diagnostics::DiagnosticEngine engine{};
    diagnostics::Diagnostic diagnostic{};
    diagnostic.code = "parser.expected-token";
    diagnostic.message = "expected ';'";
    diagnostic.span.begin = {"main.nova", 10, 3, 7};
    diagnostic.span.end = {"main.nova", 11, 3, 8};
    diagnostic.labels.push_back({{{"lib.nova", 4, 1, 5}, {"lib.nova", 5, 1, 6}}, "declared here"});
    diagnostic.notes.push_back({"from included source", diagnostics::SourceSpan{{"lib.nova", 4, 1, 5}, {"lib.nova", 5, 1, 6}}});
    diagnostic.fixes.push_back({{{"main.nova", 10, 3, 7}, {"main.nova", 11, 3, 8}}, ";", "insert punctuation"});
    engine.report(std::move(diagnostic));
    engine.warning("warning without location");
    const auto text = engine.format();
    CHECK(text.find("error: main.nova:3:7: expected ';' [parser.expected-token]") != std::string::npos);
    CHECK(text.find("label: lib.nova:1:5: declared here") != std::string::npos);
    CHECK(text.find("note: lib.nova:1:5: from included source") != std::string::npos);
    CHECK(text.find("fix: main.nova:3:7: insert punctuation -> ;") != std::string::npos);
    CHECK(text.find("warning: warning without location") != std::string::npos);
}

TEST(PrecompilationDiagnostics, LexerExceptionKeepsFileAndStableCode) {
    controllers::EngineController engine{};
    const source::Source bad{"main.nova", "main.nova", "ok @\n"};
    CHECK(fails([&] { engine.tokenizeSource(bad); }));
    CHECK(engine.diagnostics().diagnostics().size() == 1);
    const auto &d = engine.diagnostics().diagnostics()[0];
    CHECK(d.code == "lexer.unexpected-character");
    CHECK(d.span.begin.file == "main.nova");
    CHECK(d.span.begin.line == 1);
    CHECK(d.span.begin.column == 4);
    CHECK(engine.diagnostics().format().find("main.nova:1:4") != std::string::npos);
}

TEST(PrecompilationDiagnostics, ParserFailureCarriesTokenSourceSpan) {
    controllers::EngineController engine{};
    engine.parseRule("expr", "never", [](parser::ParserContext &) -> ast::NodePtr { return nullptr; });
    lexer::Lexer scan{engine.lexer()};
    CHECK(fails([&] { engine.parseTokens(scan.tokenize("abc", "included.nova"), "expr"); }));
    CHECK(engine.diagnostics().diagnostics().size() == 1);
    const auto &d = engine.diagnostics().diagnostics()[0];
    CHECK(d.code == "parser.no-rule");
    CHECK(d.span.begin.file == "included.nova");
    CHECK(d.span.begin.line == 1);
    CHECK(d.span.begin.column == 1);
}

