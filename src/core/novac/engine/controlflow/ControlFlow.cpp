#include "novac/engine/controlflow/ControlFlow.hpp"

#include <stdexcept>
#include <utility>

namespace novac::controlflow {

/**
 * @brief Implements the `statement` operation.
 *
 * @param spec Value supplied for `spec`.
 * @return Value produced by the operation.
 */
controllers::EngineFeature::Installer statement(StatementSpec spec) {
    if (spec.domain.value.empty()) {
        throw std::runtime_error("controlflow::statement: parse domain cannot be empty");
    }
    if (spec.trigger.empty()) {
        throw std::runtime_error("controlflow::statement: trigger cannot be empty");
    }
    if (spec.schema.kind.empty()) {
        throw std::runtime_error("controlflow::statement: AST schema kind cannot be empty");
    }
    if (!spec.parse) {
        throw std::runtime_error("controlflow::statement: parse callback cannot be empty");
    }

    return [spec = std::move(spec)](controllers::EngineController &engine) {
        switch (spec.triggerRegistration) {
            case TriggerRegistration::Keyword:
                engine.keyword(spec.trigger);
                break;
            case TriggerRegistration::Symbol:
                engine.symbol(spec.trigger);
                break;
            case TriggerRegistration::Existing:
                break;
        }

        engine.node(spec.schema);
        engine.parseRule(spec.domain, spec.trigger, spec.parse);

        if (spec.runtime) {
            engine.statement(spec.schema.kind, *spec.runtime);
        }
        if (spec.hir) {
            engine.hir(spec.schema.kind, *spec.hir);
        }
    };
}

/**
 * @brief Constructs a `LoopGuard` instance.
 *
 * @param maximumIterations Value supplied for `maximumIterations`.
 */
LoopGuard::LoopGuard(std::size_t maximumIterations)
    : maximumIterations_{maximumIterations}, iterations_{} {}

/**
 * @brief Implements the `step` operation.
 */
void LoopGuard::step() {
    if (maximumIterations_ != 0 && iterations_ >= maximumIterations_) {
        throw std::runtime_error("LoopGuard::step: maximum loop iteration count exceeded");
    }
    ++iterations_;
}

/**
 * @brief Implements the `iterations` operation.
 *
 * @return Value produced by the operation.
 */
std::size_t LoopGuard::iterations() const noexcept {
    return iterations_;
}

/**
 * @brief Implements the `maximumIterations` operation.
 *
 * @return Value produced by the operation.
 */
std::size_t LoopGuard::maximumIterations() const noexcept {
    return maximumIterations_;
}

} // namespace novac::controlflow
