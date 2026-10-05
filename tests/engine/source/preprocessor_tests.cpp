#include "../../tester.hpp"
#include "novac/engine/EngineController.hpp"
#include "novac/engine/source/StandardPreprocessing.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace {

novac::controllers::EngineController makeEngine(
    std::unordered_map<std::string, novac::source::Source> files) {
    novac::controllers::EngineController engine{};
    engine.sources().resolver(
        "memory",
        [files = std::move(files)](const novac::source::SourceRequest &request) -> std::optional<novac::source::Source> {
            const auto iter{files.find(request.specifier)};
            if (iter == files.end()) {
                return std::nullopt;
            }
            return iter->second;
        });
    engine.install(novac::source::standardPreprocessing());
    return engine;
}

bool throwsRuntimeError(const std::function<void()> &fn) {
    try {
        fn();
    } catch (const std::runtime_error &) {
        return true;
    }
    return false;
}

} // namespace

TEST(Preprocessor, ConditionalCompilationRunsBeforeLanguageLexer) {
    auto engine{makeEngine({})};
    const novac::source::Source source{
        "main", "main.nova",
        "#if NEVER\n@@@ invalid language syntax @@@\n#else\nvisible\n#endif\n"};

    const std::vector<novac::token::Token> tokens{engine.tokenizeSource(source)};
    CHECK_EQ(tokens.size(), std::size_t{2});
    CHECK(tokens[0].text == "visible");
    CHECK(tokens[0].span.begin.file == "main.nova");
    CHECK_EQ(tokens[0].span.begin.line, 4);
}

TEST(Preprocessor, IncludePreservesIncludedSourceLocations) {
    auto engine{makeEngine({
        {"child", {"pkg:child", "child.nova", "inside\n"}}
    })};
    const novac::source::Source main{"main", "main.nova", "#include \"child\"\nafter\n"};

    const std::vector<novac::token::Token> tokens{engine.tokenizeSource(main)};
    CHECK_EQ(tokens.size(), std::size_t{3});
    CHECK(tokens[0].text == "inside");
    CHECK(tokens[0].span.begin.file == "child.nova");
    CHECK_EQ(tokens[0].span.begin.line, 1);
    CHECK(tokens[1].text == "after");
    CHECK(tokens[1].span.begin.file == "main.nova");
    CHECK_EQ(tokens[1].span.begin.line, 2);
}

TEST(Preprocessor, ImportProcessesCanonicalSourceOnlyOnce) {
    auto engine{makeEngine({
        {"one", {"canonical", "module.nova", "item\n"}},
        {"alias", {"canonical", "module.nova", "item\n"}}
    })};
    const novac::source::Source main{
        "main", "main.nova", "#import \"one\"\n#import \"alias\"\nend\n"};

    const std::vector<novac::token::Token> tokens{engine.tokenizeSource(main)};
    CHECK_EQ(tokens.size(), std::size_t{3});
    CHECK(tokens[0].text == "item");
    CHECK(tokens[1].text == "end");
}

TEST(Preprocessor, IncludeProcessesSourceEveryTime) {
    auto engine{makeEngine({
        {"child", {"canonical", "module.nova", "item\n"}}
    })};
    const novac::source::Source main{
        "main", "main.nova", "#include \"child\"\n#include \"child\"\n"};

    const std::vector<novac::token::Token> tokens{engine.tokenizeSource(main)};
    CHECK_EQ(tokens.size(), std::size_t{3});
    CHECK(tokens[0].text == "item");
    CHECK(tokens[1].text == "item");
}

TEST(Preprocessor, DetectsIncludeCycles) {
    auto engine{makeEngine({
        {"a", {"a", "a.nova", "#include \"b\"\n"}},
        {"b", {"b", "b.nova", "#include \"a\"\n"}}
    })};

    CHECK(throwsRuntimeError([&] {
        engine.tokenizeSource({"main", "main.nova", "#include \"a\"\n"});
    }));
}

TEST(Preprocessor, DetectsImportCyclesBeforeDeduplication) {
    auto engine{makeEngine({
        {"a", {"a", "a.nova", "#import \"b\"\n"}},
        {"b", {"b", "b.nova", "#import \"a\"\n"}}
    })};

    CHECK(throwsRuntimeError([&] {
        engine.tokenizeSource({"main", "main.nova", "#import \"a\"\n"});
    }));
}

TEST(Preprocessor, PragmasAreLanguageExtensible) {
    auto engine{makeEngine({})};
    std::string received{};
    engine.pragma("dialect", [&](const novac::source::Directive &directive, novac::source::PreprocessorContext &) {
        received = directive.arguments;
    });

    const novac::source::Source main{"main", "main.nova", "#pragma dialect strict\nvalue\n"};
    const auto tokens{engine.tokenizeSource(main)};

    CHECK(received == "strict");
    CHECK(tokens[0].text == "value");
}

TEST(Preprocessor, DirectivesInsideCommentsAreIgnored) {
    auto engine{makeEngine({})};
    const novac::source::Source main{
        "main", "main.nova",
        "/*\n#include \"missing\"\n*/\n// #include \"missing\"\nvalue\n"};

    const auto tokens{engine.tokenizeSource(main)};
    CHECK_EQ(tokens.size(), std::size_t{2});
    CHECK(tokens[0].text == "value");
}

TEST(Preprocessor, DirectiveAfterClosingBlockCommentKeepsLexicalCommentBalanced) {
    auto engine{makeEngine({})};
    const novac::source::Source main{
        "main", "main.nova",
        "/* comment\n*/ #define READY\n#if READY\nvalue\n#endif\n"};

    const auto tokens{engine.tokenizeSource(main)};
    CHECK_EQ(tokens.size(), std::size_t{2});
    CHECK(tokens[0].text == "value");
    CHECK_EQ(tokens[0].span.begin.line, 4);
}

TEST(Preprocessor, StandardDirectivesAllowTrailingLineComments) {
    auto engine{makeEngine({
        {"child", {"child", "child.nova", "item\n"}}
    })};
    const novac::source::Source main{
        "main", "main.nova",
        "#define ENABLED // comment\n#if ENABLED // comment\n#include \"child\" // comment\n#endif\n"};

    const auto tokens{engine.tokenizeSource(main)};
    CHECK_EQ(tokens.size(), std::size_t{2});
    CHECK(tokens[0].text == "item");
}

TEST(Preprocessor, BlockCommentStartedAfterDirectiveSuppressesNestedDirectives) {
    auto engine{makeEngine({})};
    const novac::source::Source main{
        "main", "main.nova",
        "#define READY /* comment\n"
        "#include \"missing\"\n"
        "*/\n"
        "#if READY\n"
        "value\n"
        "#endif\n"};

    const auto tokens{engine.tokenizeSource(main)};
    CHECK_EQ(tokens.size(), std::size_t{2});
    CHECK(tokens[0].text == "value");
    CHECK_EQ(tokens[0].span.begin.line, 5);
}

TEST(Preprocessor, PragmaArgumentsSpanMatchesDispatchedArguments) {
    auto engine{makeEngine({})};
    novac::diagnostics::SourceSpan received{};
    std::string arguments{};
    engine.pragma("dialect", [&](const novac::source::Directive &directive, novac::source::PreprocessorContext &) {
        arguments = directive.arguments;
        received = directive.argumentsSpan;
    });

    engine.tokenizeSource({"main", "main.nova", "#pragma dialect strict\n"});

    CHECK(arguments == "strict");
    CHECK(received.begin.file == "main.nova");
    CHECK_EQ(received.begin.line, 1);
    CHECK_EQ(received.begin.column, 17);
    CHECK_EQ(received.end.column, 23);
}

TEST(Preprocessor, EndTokenStaysAtRootSourceEndAfterInclude) {
    auto engine{makeEngine({
        {"child", {"child", "child.nova", "item\n"}}
    })};
    const novac::source::Source main{"main", "main.nova", "#include \"child\"\n"};

    const auto tokens{engine.tokenizeSource(main)};
    CHECK_EQ(tokens.size(), std::size_t{2});
    CHECK(tokens.back().kind == novac::token::Kind::End);
    CHECK(tokens.back().span.begin.file == "main.nova");
    CHECK_EQ(tokens.back().span.begin.offset, main.text.size());
    CHECK_EQ(tokens.back().span.begin.line, 2);
    CHECK_EQ(tokens.back().span.begin.column, 1);
}

TEST(Preprocessor, EndTokenStaysAtRootEndWhenAllTextIsFiltered) {
    auto engine{makeEngine({})};
    const novac::source::Source main{
        "main", "main.nova", "#if NEVER\nhidden\n#endif\n"};

    const auto tokens{engine.tokenizeSource(main)};
    CHECK_EQ(tokens.size(), std::size_t{1});
    CHECK(tokens[0].kind == novac::token::Kind::End);
    CHECK(tokens[0].span.begin.file == "main.nova");
    CHECK_EQ(tokens[0].span.begin.offset, main.text.size());
    CHECK_EQ(tokens[0].span.begin.line, 4);
    CHECK_EQ(tokens[0].span.begin.column, 1);
}

TEST(Preprocessor, IncludeDepthCountsLevelsBelowRoot) {
    auto engine{makeEngine({
        {"child", {"child", "child.nova", "value\n"}},
        {"middle", {"middle", "middle.nova", "#include \"child\"\n"}}
    })};

    novac::source::PreprocessOptions options{};
    options.maxIncludeDepth = 1;

    const auto oneLevel{engine.tokenizeSource(
        {"main", "main.nova", "#include \"child\"\n"}, options)};
    CHECK_EQ(oneLevel.size(), std::size_t{2});
    CHECK(oneLevel[0].text == "value");

    CHECK(throwsRuntimeError([&] {
        engine.tokenizeSource(
            {"main", "main.nova", "#include \"middle\"\n"}, options);
    }));
}

TEST(Preprocessor, UnterminatedDirectiveBlockCommentIsRejected) {
    auto engine{makeEngine({})};
    CHECK(throwsRuntimeError([&] {
        engine.tokenizeSource({"main", "main.nova", "#define READY /* never closes\n"});
    }));
}

TEST(Preprocessor, BlockCommentsInsideDirectiveArgumentsBehaveAsWhitespace) {
    auto engine{makeEngine({})};
    std::string received{};
    engine.pragma("dialect", [&](const novac::source::Directive &directive, novac::source::PreprocessorContext &) {
        received = directive.arguments;
    });

    engine.tokenizeSource({
        "main",
        "main.nova",
        "#pragma dialect before /* note */ after\n"});

    CHECK(received.find("before") != std::string::npos);
    CHECK(received.find("after") != std::string::npos);
}

TEST(Preprocessor, StandardDirectiveDoesNotSilentlyDropTextAfterBlockComment) {
    auto engine{makeEngine({})};

    CHECK(throwsRuntimeError([&] {
        engine.tokenizeSource({
            "main",
            "main.nova",
            "#define READY /* note */ garbage\n"});
    }));
}

TEST(Preprocessor, ElseAndEndifRejectArguments) {
    auto engine{makeEngine({})};

    CHECK(throwsRuntimeError([&] {
        engine.tokenizeSource({
            "main",
            "main.nova",
            "#if NEVER\n#else unexpected\n#endif\n"});
    }));

    CHECK(throwsRuntimeError([&] {
        engine.tokenizeSource({
            "main",
            "main.nova",
            "#if NEVER\n#endif unexpected\n"});
    }));
}

TEST(Preprocessor, IncludeAndImportRequireQuotedOrBracketedSpecifiers) {
    auto engine{makeEngine({
        {"child", {"child", "child.nova", "item\n"}}
    })};

    CHECK(throwsRuntimeError([&] {
        engine.tokenizeSource({"main", "main.nova", "#include child\n"});
    }));
    CHECK(throwsRuntimeError([&] {
        engine.tokenizeSource({"main", "main.nova", "#import child\n"});
    }));

    const auto tokens{engine.tokenizeSource({"main", "main.nova", "#include <child>\n"})};
    CHECK_EQ(tokens.size(), std::size_t{2});
    CHECK(tokens[0].text == "item");
}
