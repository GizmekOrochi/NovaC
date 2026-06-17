#include "../../tester.hpp"
#include "novac/engine/foundation/Ids.hpp"

#include <string>

using novac::ids::FieldName;
using novac::ids::NodeKind;
using novac::ids::Operation;
using novac::ids::ParseDomain;

TEST(NodeKind, StoresValue) {
    NodeKind kind{"IntegerLiteral"};

    CHECK(kind.value == "IntegerLiteral");
}

TEST(NodeKind, AllowsEmptyValue) {
    NodeKind kind{""};

    CHECK(kind.value.empty());
}

TEST(NodeKind, SupportsMovedString) {
    std::string value{"BinaryExpression"};

    NodeKind kind{std::move(value)};

    CHECK(kind.value == "BinaryExpression");
}

TEST(FieldName, StoresValue) {
    FieldName name{"value"};

    CHECK(name.value == "value");
}

TEST(FieldName, AllowsEmptyValue) {
    FieldName name{""};

    CHECK(name.value.empty());
}

TEST(ParseDomain, StoresValue) {
    ParseDomain domain{"expression"};

    CHECK(domain.value == "expression");
}

TEST(ParseDomain, AllowsEmptyValue) {
    ParseDomain domain{""};

    CHECK(domain.value.empty());
}

TEST(Operation, StoresValue) {
    Operation operation{"add"};

    CHECK(operation.value == "add");
}

TEST(Operation, AllowsEmptyValue) {
    Operation operation{""};

    CHECK(operation.value.empty());
}
