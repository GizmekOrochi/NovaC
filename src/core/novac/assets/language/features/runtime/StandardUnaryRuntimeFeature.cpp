#include "novac/assets/language/features/runtime/StandardUnaryRuntimeFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

namespace {

bool isIntegerValue(const runtime::Value &value) {
    try {
        static_cast<void>(value.asInt());
        return true;
    } catch (const std::runtime_error &) {
        return false;
    }
}

runtime::Value negate(runtime::Value value) {
    if (isIntegerValue(value)) {
        return runtime::Value::integer(-value.asInt());
    }

    return runtime::Value::floating(-value.asFloat());
}

} // namespace

StandardUnaryRuntimeFeature::StandardUnaryRuntimeFeature(std::string unaryNodeKind)
    : unaryNodeKind_{std::move(unaryNodeKind)} {
    if (unaryNodeKind_.empty()) {
        throw std::runtime_error("StandardUnaryRuntimeFeature::StandardUnaryRuntimeFeature: unary node kind cannot be empty");
    }
}

LanguageFeatureInfo StandardUnaryRuntimeFeature::info() const {
    return {"language.runtime.unary-operators", "0.2.0", "Runtime behavior for standard unary operators", {"runtime.operator.unary"}, {"expression.unary"}, {"language.expression.unary"}, {}, {}};
}

void StandardUnaryRuntimeFeature::install(LanguageOptionsController &language) const {
    const std::string kind{unaryNodeKind_};

    language.expression(kind, [](const ast::Node &node, runtime::RuntimeContext &context) {
        const std::string op{node.str("op")};
        runtime::Value value{context.eval(*node.child("expr"))};

        if (op == "-") {
            return negate(std::move(value));
        }

        if (op == "!") {
            return runtime::Value::boolean(!value.truthy());
        }

        throw std::runtime_error("StandardUnaryRuntimeFeature::runtime: unsupported unary operator '" + op + "'");
    });
}

} // namespace novac::language::features
