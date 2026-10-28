#include "../tester.hpp"
#include "NovaC.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace novac;

TEST(Extensibility, MetadataStoreCarriesArbitraryNamedValues) {
    metadata::MetadataStore metadata;
    metadata.set<int>("example.count", 7);
    metadata.set<std::string>("example.name", "demo");

    CHECK(metadata.require<int>("example.count") == 7);
    CHECK(metadata.require<std::string>("example.name") == "demo");
    CHECK(metadata.get<double>("example.count") == nullptr);
}


TEST(Extensibility, AstSupportsOpaqueFieldsSpanAndMetadata) {
    ast::Node node{"CustomNode"};
    diagnostics::SourceSpan span{{"x.nc", 10, 2, 3}, {"x.nc", 14, 2, 7}};

    node.setOpaque<std::vector<int>>("payload", {1, 2, 3});
    node.setSpan(span);
    node.metadata().set<std::string>("example.role", "test");

    const auto *payload{node.opaque<std::vector<int>>("payload")};
    CHECK(payload != nullptr);
    CHECK(payload->size() == 3);
    CHECK(node.span().begin.offset == 10);
    CHECK(node.metadata().require<std::string>("example.role") == "test");

    ast::NodeRegistry registry;
    registry.registerNode({"CustomNode", {{"payload", ast::FieldKind::Opaque, true, {}, {}}}, {}, {}});
    registry.validate(node);
}


TEST(Extensibility, LexerCustomRulesProduceTaggedTokens) {
    lexer::LexerRegistry registry;
    registry.tokenRule("raw-name", 100, [](std::string_view remaining) -> std::optional<lexer::TokenRuleMatch> {
        if (remaining.size() < 3 || remaining.substr(0, 2) != "%%") {
            return std::nullopt;
        }
        std::size_t length{2};
        while (length < remaining.size() && remaining[length] >= 'a' && remaining[length] <= 'z') {
            ++length;
        }
        if (length == 2) {
            return std::nullopt;
        }
        return lexer::TokenRuleMatch{length, token::Kind::Identifier, std::string{remaining.substr(2, length - 2)}, {}, "$raw-name"};
    });

    lexer::Lexer lexer{registry};
    const auto tokens{lexer.tokenize("%%hello")};

    CHECK(tokens.size() == 2);
    CHECK(tokens[0].text == "hello");
    CHECK(tokens[0].tag == "$raw-name");
}


TEST(Extensibility, ParserDispatchesOnCustomTokenTag) {
    lexer::LexerRegistry lex;
    lex.tokenRule("tagged", 100, [](std::string_view remaining) -> std::optional<lexer::TokenRuleMatch> {
        if (remaining.empty() || remaining.front() != '#') {
            return std::nullopt;
        }
        return lexer::TokenRuleMatch{1, token::Kind::Symbol, "#", {}, "$tagged"};
    });
    parser::ParserRegistry registry;
    registry.rule("root", "$tagged", [](parser::ParserContext &context) {
        context.advance();
        return ast::Node::make("Tagged");
    });

    parser::Parser parser{registry, "root"};
    lexer::Lexer lexer{lex};
    CHECK(parser.parse(lexer.tokenize("#"))->kind() == "Tagged");
}


TEST(Extensibility, ScopeGraphAndSymbolTableResolveLexically) {
    ast::Node rootNode{"Root"};
    ast::Node childNode{"Child"};
    semantics::ScopeGraph scopes;
    const auto root{scopes.create(std::nullopt, &rootNode)};
    const auto child{scopes.create(root, &childNode)};

    semantics::SymbolTable symbols{scopes};
    const auto outer{symbols.declare("value", root, "binding", &rootNode)};
    CHECK(symbols.resolveFirst("value", child).has_value());
    CHECK(symbols.resolveFirst("value", child)->value == outer.value);
    CHECK(scopes.parentOf(child)->value == root.value);
    CHECK(scopes.scopeOf(childNode)->value == child.value);
}


TEST(Extensibility, DiagnosticsCarryLabelsNotesFixesAndMetadata) {
    diagnostics::Diagnostic diagnostic;
    diagnostic.severity = diagnostics::DiagnosticSeverity::Error;
    diagnostic.message = "invalid construct";
    diagnostic.code = "example.invalid";
    diagnostic.labels.push_back({{{"x.nc", 4, 1, 5}, {"x.nc", 5, 1, 6}}, "related location"});
    diagnostic.notes.push_back({"extra context", std::nullopt});
    diagnostic.fixes.push_back({{{"x.nc", 4, 1, 5}, {"x.nc", 5, 1, 6}}, "replacement", "use this form"});
    diagnostic.metadata.set<int>("example.rank", 3);

    diagnostics::DiagnosticEngine engine;
    engine.report(std::move(diagnostic));

    CHECK(engine.diagnostics()[0].code == "example.invalid");
    CHECK(engine.diagnostics()[0].metadata.require<int>("example.rank") == 3);
    CHECK(engine.format().find("extra context") != std::string::npos);
    CHECK(engine.format().find("replacement") != std::string::npos);
}


