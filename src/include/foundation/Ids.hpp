#pragma once

#include <string>
#include <utility>

namespace novac::ids {

struct NodeKind {
    std::string value{};

    explicit NodeKind(std::string name) : value{std::move(name)} {}
};

struct FieldName {
    std::string value{};

    explicit FieldName(std::string name) : value{std::move(name)} {}
};

struct ParseDomain {
    std::string value{};

    explicit ParseDomain(std::string name) : value{std::move(name)} {}
};

struct Operation {
    std::string value{};

    explicit Operation(std::string name) : value{std::move(name)} {}
};

} // namespace novac::ids