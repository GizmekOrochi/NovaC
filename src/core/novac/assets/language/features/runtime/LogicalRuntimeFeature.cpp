#include "novac/assets/language/features/runtime/LogicalRuntimeFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

LogicalRuntimeFeature::LogicalRuntimeFeature(std::string binaryNodeKind)
    : binaryNodeKind_{std::move(binaryNodeKind)} {
    if (binaryNodeKind_.empty()) {
        throw std::runtime_error("LogicalRuntimeFeature::LogicalRuntimeFeature: binary node kind cannot be empty");
    }
}

LanguageFeatureInfo LogicalRuntimeFeature::info() const {
    return {"language.runtime.logical-operators", "0.2.0", "Runtime short-circuit logical operators", {"runtime.operator.logical"}, {"expression.binary"}, {"language.expression.binary"}, {}, {}};
}

void LogicalRuntimeFeature::install(LanguageOptionsController &language) const {
    static_cast<void>(binaryNodeKind_);

    language.binaryOperator("&&", [](const ast::Node &node, runtime::RuntimeContext &context) {
        const bool left{context.eval(*node.child("left")).truthy()};

        if (!left) {
            return runtime::Value::boolean(false);
        }

        return runtime::Value::boolean(context.eval(*node.child("right")).truthy());
    });

    language.binaryOperator("||", [](const ast::Node &node, runtime::RuntimeContext &context) {
        const bool left{context.eval(*node.child("left")).truthy()};

        if (left) {
            return runtime::Value::boolean(true);
        }

        return runtime::Value::boolean(context.eval(*node.child("right")).truthy());
    });
}

} // namespace novac::language::features
