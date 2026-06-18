#include "../../tester.hpp"
#include "novac/assets/atomic/OperationFeature.hpp"
#include "novac/assets/atomic/AtomicPattern.hpp"

namespace {

using novac::assets::atomic::OperationFeature;
using novac::assets::atomic::OperationInfo;
using novac::assets::atomic::OperationArity;
using novac::assets::atomic::TokenPattern;
using novac::assets::atomic::TokenPatternMode;
using novac::assets::atomic::AtomicController;
using novac::parser::Associativity;

namespace test {
    class AddOperationFeature : public OperationFeature {
    public:
        OperationInfo info() const override {
            return OperationInfo{
                "add_operation",
                "1.0.0",
                "Binary addition operator",
                OperationArity::Binary,
                TokenPattern::text("+"),
                10,
                Associativity::Left,
                {"arithmetic", "binary"},
                {"lexer", "parser"}
            };
        }
        void install(AtomicController &controller) const override { (void)controller; }
    };

    class NegateOperationFeature : public OperationFeature {
    public:
        OperationInfo info() const override {
            return OperationInfo{
                "negate_operation",
                "0.5.0",
                "Unary negation operator",
                OperationArity::Unary,
                TokenPattern::text("-"),
                20,
                Associativity::Right,
                {"arithmetic", "unary"},
                {"lexer"}
            };
        }
        void install(AtomicController &controller) const override { (void)controller; }
    };
} // namespace test

TEST(OperationFeature, InfoReturnsCorrectBinaryMetadata) {
    test::AddOperationFeature feature;
    OperationInfo info = feature.info();
    CHECK(info.id == "add_operation");
    CHECK(info.arity == OperationArity::Binary);
    CHECK(info.pattern.token == "+");
    CHECK(info.precedence == 10);
    CHECK(info.associativity == Associativity::Left);
}

TEST(OperationFeature, InfoHandlesUnaryOperation) {
    test::NegateOperationFeature feature;
    OperationInfo info = feature.info();
    CHECK(info.arity == OperationArity::Unary);
    CHECK(info.precedence == 20);
    CHECK(info.associativity == Associativity::Right);
}

} // namespace