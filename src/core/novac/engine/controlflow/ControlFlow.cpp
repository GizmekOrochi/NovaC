#include "novac/engine/controlflow/ControlFlow.hpp"

#include <stdexcept>
#include <utility>

namespace novac::controlflow {

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

LoopGuard::LoopGuard(std::size_t maximumIterations)
    : maximumIterations_{maximumIterations}, iterations_{} {}

void LoopGuard::step() {
    if (maximumIterations_ != 0 && iterations_ >= maximumIterations_) {
        throw std::runtime_error("LoopGuard::step: maximum loop iteration count exceeded");
    }
    ++iterations_;
}

std::size_t LoopGuard::iterations() const noexcept {
    return iterations_;
}

std::size_t LoopGuard::maximumIterations() const noexcept {
    return maximumIterations_;
}

} // namespace novac::controlflow
