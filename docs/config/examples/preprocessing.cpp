#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

#include <NovaC.hpp>

int main() {
    using novac::source::Source;
    using novac::source::SourceRequest;

    novac::controllers::EngineController engine{};

    const std::unordered_map<std::string, Source> sources{
        {"config", {"pkg:config", "config.nova", "#define FEATURE\n"}},
        {"shared", {"pkg:shared", "shared.nova", "from_shared\n"}},
        {"module", {"pkg:module", "module.nova", "from_module\n"}},
        // A different specifier resolving to the same canonical source id.
        {"module-alias", {"pkg:module", "module.nova", "from_module\n"}}
    };

    engine.sources().resolver(
        "memory",
        [sources](const SourceRequest &request) -> std::optional<Source> {
            const auto found{sources.find(request.specifier)};
            if (found == sources.end()) {
                return std::nullopt;
            }
            return found->second;
        });

    engine.install(novac::source::standardPreprocessing());

    std::string dialect{};
    engine.pragma(
        "dialect",
        [&dialect](const novac::source::Directive &directive,
                   novac::source::PreprocessorContext &) {
            dialect = directive.arguments;
        });

    const Source mainSource{
        "app:main",
        "main.nova",
        "#pragma dialect strict\n"
        "#include \"config\"\n"
        "#if FEATURE\n"
        "#include \"shared\"\n"
        "#endif\n"
        "#import \"module\"\n"
        "#import \"module-alias\"\n"
        "root\n"};

    const auto tokens{engine.tokenizeSource(mainSource)};

    std::cout << "dialect=" << dialect << '\n';
    for (const auto &token : tokens) {
        if (token.kind == novac::token::Kind::End) {
            continue;
        }
        std::cout << token.text << " @ "
                  << token.span.begin.file << ':' << token.span.begin.line << '\n';
    }
}
