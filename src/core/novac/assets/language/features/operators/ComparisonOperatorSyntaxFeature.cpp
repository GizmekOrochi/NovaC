#include "novac/assets/language/features/operators/ComparisonOperatorSyntaxFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace novac::language::features {

ComparisonOperatorSyntaxFeature::ComparisonOperatorSyntaxFeature(std::string expressionDomain, std::string binaryNodeKind)
    : expressionDomain_{std::move(expressionDomain)}, binaryNodeKind_{std::move(binaryNodeKind)} {
    if (expressionDomain_.empty() || binaryNodeKind_.empty()) {
        throw std::runtime_error("ComparisonOperatorSyntaxFeature::ComparisonOperatorSyntaxFeature: domain and binary node kind cannot be empty");
    }
}

LanguageFeatureInfo ComparisonOperatorSyntaxFeature::info() const {
    return {"language.syntax.comparison-operators", "0.2.0", "Comparison infix operator syntax", {"syntax.operator.comparison"}, {"expression.binary"}, {"language.expression.binary"}, {}, {}};
}

void ComparisonOperatorSyntaxFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{binaryNodeKind_};

    const auto builder{[kind, &language](parser::ParserContext &, ast::NodePtr left, const token::Token &op, ast::NodePtr right) {
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("op", op.text);
        node->set("left", left);
        node->set("right", right);
        return node;
    }};

    const std::array<std::string, 6> operators{"==", "!=", "<=", ">=", "<", ">"};

    for (const std::string &op : operators) {
        language.symbol(op);
        language.infix(domain, op, 7, parser::Associativity::None, builder);
    }
}

} // namespace novac::language::features
