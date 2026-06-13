#include "novac/assets/language/features/operators/UnaryOperatorSyntaxFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

UnaryOperatorSyntaxFeature::UnaryOperatorSyntaxFeature(std::string expressionDomain, std::string unaryNodeKind)
    : expressionDomain_{std::move(expressionDomain)}, unaryNodeKind_{std::move(unaryNodeKind)} {
    if (expressionDomain_.empty() || unaryNodeKind_.empty()) {
        throw std::runtime_error("UnaryOperatorSyntaxFeature::UnaryOperatorSyntaxFeature: domain and unary node kind cannot be empty");
    }
}

LanguageFeatureInfo UnaryOperatorSyntaxFeature::info() const {
    return {"language.syntax.unary-operators", "0.2.0", "Unary operator syntax", {"syntax.operator.unary"}, {"expression.unary"}, {"language.expression.unary"}, {}, {}};
}

void UnaryOperatorSyntaxFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{unaryNodeKind_};

    const auto builder{[domain, kind, &language](parser::ParserContext &context) {
        const token::Token op{context.advance()};
        ast::NodePtr operand{context.parse(domain, 30)};
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("op", op.text);
        node->set("expr", operand);
        return node;
    }};

    language.symbol("-");
    language.symbol("!");
    language.prefix(domain, "-", builder);
    language.prefix(domain, "!", builder);
}

} // namespace novac::language::features
