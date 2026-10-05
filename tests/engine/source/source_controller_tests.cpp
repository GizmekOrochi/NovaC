#include "../../tester.hpp"
#include "novac/engine/source/SourceController.hpp"

#include <optional>
#include <string>

TEST(SourceController, ResolvesThroughOrderedCallbacks) {
    novac::source::SourceController sources{};
    std::string trace{};

    sources.resolver("first", [&](const novac::source::SourceRequest &) -> std::optional<novac::source::Source> {
        trace += "A";
        return std::nullopt;
    });
    sources.resolver("second", [&](const novac::source::SourceRequest &request) -> std::optional<novac::source::Source> {
        trace += "B";
        return novac::source::Source{"id:" + request.specifier, {}, "body"};
    });

    const novac::source::Source source{sources.resolve({"module", "main"})};
    CHECK(trace == "AB");
    CHECK(source.id == "id:module");
    CHECK(source.name == "id:module");
    CHECK(source.text == "body");
}
