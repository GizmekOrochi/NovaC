#include "../../tester.hpp"
#include "novac/engine/transformation/IR.hpp"

#include <string>

using novac::ir::Literal;

TEST(Literal, DefaultConstruction) {
    Literal literal{};

    CHECK(literal.toString() == "none");
}

TEST(Literal, IntegerToString) {
    CHECK(Literal::integer(42).toString() == "42");
    CHECK(Literal::integer(-7).toString() == "-7");
}

TEST(Literal, FloatingToString) {
    const std::string text{Literal::floating(3.5).toString()};

    CHECK(text.find("3.500000") != std::string::npos);
}

TEST(Literal, BooleanToString) {
    CHECK(Literal::boolean(true).toString() == "true");
    CHECK(Literal::boolean(false).toString() == "false");
}

TEST(Literal, StringToString) {
    CHECK(Literal::string("hello").toString() == "hello");
}

TEST(Literal, RawVariantConstruction) {
    Literal literal{Literal::Data{std::string{"raw"}}};

    CHECK(literal.toString() == "raw");
}
