#include "novac/assets/language/features/operators/ArithmeticOperatorSyntaxFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace novac::language::features {

ArithmeticOperatorSyntaxFeature::ArithmeticOperatorSyntaxFeature(std::string expressionDomain, std::string binaryNodeKind)
    : expressionDomain_{std::move(expressionDomain)}, binaryNodeKind_{std::move(binaryNodeKind)} {
    if (expressionDomain_.empty() || binaryNodeKind_.empty()) {
        throw std::runtime_error("ArithmeticOperatorSyntaxFeature::ArithmeticOperatorSyntaxFeature: domain and binary node kind cannot be empty");
    }
}

LanguageFeatureInfo ArithmeticOperatorSyntaxFeature::info() const {
    return {"language.syntax.arithmetic-operators", "0.2.0", "Arithmetic infix operator syntax", {"syntax.operator.arithmetic"}, {"expression.binary"}, {"language.expression.binary"}, {}, {}};
}

void ArithmeticOperatorSyntaxFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{binaryNodeKind_};

    const auto builder{[kind, &language](parser::ParserContext &, ast::NodePtr left, const token::Token &op, ast::NodePtr right) {
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("op", op.text);
        node->set("left", left);
        node->set("right", right);
        return node;
    }};

    const std::array<std::string, 5> operators{"+", "-", "*", "/", "%"};

    for (const std::string &op : operators) {
        language.symbol(op);
    }

    language.infix(domain, "+", 10, builder);
    language.infix(domain, "-", 10, builder);
    language.infix(domain, "*", 20, builder);
    language.infix(domain, "/", 20, builder);
    language.infix(domain, "%", 20, builder);
}

} // namespace novac::language::features
