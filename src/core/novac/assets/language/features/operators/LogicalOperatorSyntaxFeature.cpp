#include "novac/assets/language/features/operators/LogicalOperatorSyntaxFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

LogicalOperatorSyntaxFeature::LogicalOperatorSyntaxFeature(std::string expressionDomain, std::string binaryNodeKind)
    : expressionDomain_{std::move(expressionDomain)}, binaryNodeKind_{std::move(binaryNodeKind)} {
    if (expressionDomain_.empty() || binaryNodeKind_.empty()) {
        throw std::runtime_error("LogicalOperatorSyntaxFeature::LogicalOperatorSyntaxFeature: domain and binary node kind cannot be empty");
    }
}

LanguageFeatureInfo LogicalOperatorSyntaxFeature::info() const {
    return {"language.syntax.logical-operators", "0.2.0", "Logical infix operator syntax", {"syntax.operator.logical"}, {"expression.binary"}, {"language.expression.binary"}, {}, {}};
}

void LogicalOperatorSyntaxFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{binaryNodeKind_};

    const auto builder{[kind, &language](parser::ParserContext &, ast::NodePtr left, const token::Token &op, ast::NodePtr right) {
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("op", op.text);
        node->set("left", left);
        node->set("right", right);
        return node;
    }};

    language.symbol("&&");
    language.symbol("||");
    language.infix(domain, "&&", 4, builder);
    language.infix(domain, "||", 3, builder);
}

} // namespace novac::language::features
