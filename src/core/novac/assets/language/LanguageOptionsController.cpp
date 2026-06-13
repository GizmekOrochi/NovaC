#include "novac/assets/language/LanguageOptionsController.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace novac::language {

LanguageOptionsController::LanguageOptionsController(
    controllers::EngineController &engine,
    LanguageOptionsControllerOptions options)
    : engine_{engine},
      options_{options},
      features_{},
      ownedFeatures_{} {
}

LanguageOptionsController &LanguageOptionsController::use(const LanguageFeature &feature) {
    const LanguageFeatureInfo info{feature.info()};

    validateFeature(info);

    if (options_.transactionalInstall) {
        installTransactional(feature, info);
    } else {
        installDirect(feature, info);
    }

    return *this;
}

LanguageOptionsController &LanguageOptionsController::use(std::unique_ptr<LanguageFeature> feature) {
    if (!feature) {
        throw std::runtime_error("LanguageOptionsController::use: feature cannot be null");
    }

    const LanguageFeatureInfo info{feature->info()};
    const std::string name{info.name};

    use(*feature);
    ownedFeatures_[name] = std::move(feature);

    return *this;
}

controllers::EngineController &LanguageOptionsController::engine() {
    return engine_;
}

const controllers::EngineController &LanguageOptionsController::engine() const {
    return engine_;
}

LanguageOptionsController &LanguageOptionsController::setStartDomain(std::string domain) {
    engine_.setStartDomain(std::move(domain));

    return *this;
}

const std::string &LanguageOptionsController::startDomain() const {
    return engine_.startDomain();
}

bool LanguageOptionsController::hasFeature(const std::string &name) const {
    return feature(name) != nullptr;
}

bool LanguageOptionsController::hasCapability(const std::string &capability) const {
    for (const LanguageFeatureInfo &info : features_) {
        const auto iter{std::find(info.capabilities.begin(), info.capabilities.end(), capability)};

        if (iter != info.capabilities.end()) {
            return true;
        }
    }

    return false;
}

const LanguageFeatureInfo *LanguageOptionsController::feature(const std::string &name) const {
    for (const LanguageFeatureInfo &info : features_) {
        if (info.name == name) {
            return &info;
        }
    }

    return nullptr;
}

const std::vector<LanguageFeatureInfo> &LanguageOptionsController::features() const {
    return features_;
}

registry::RegisterStatus LanguageOptionsController::keyword(std::string keyword) {
    return engine_.keyword(std::move(keyword));
}

registry::RegisterStatus LanguageOptionsController::symbol(std::string symbol) {
    return engine_.symbol(std::move(symbol));
}

registry::RegisterStatus LanguageOptionsController::node(ast::NodeSchema schema) {
    return engine_.node(std::move(schema));
}

registry::RegisterStatus LanguageOptionsController::parseRule(std::string domain, std::string key, parser::ParseFn fn) {
    return engine_.parseRule(std::move(domain), std::move(key), std::move(fn));
}

registry::RegisterStatus LanguageOptionsController::fallback(std::string domain, parser::ParseFn fn) {
    return engine_.fallback(std::move(domain), std::move(fn));
}

registry::RegisterStatus LanguageOptionsController::prefix(std::string domain, std::string key, parser::PrefixFn fn) {
    return engine_.prefix(std::move(domain), std::move(key), std::move(fn));
}

registry::RegisterStatus LanguageOptionsController::infix(std::string domain, std::string op, int precedence, parser::InfixFn fn) {
    return engine_.infix(std::move(domain), std::move(op), precedence, std::move(fn));
}

registry::RegisterStatus LanguageOptionsController::infix(
    std::string domain,
    std::string op,
    int precedence,
    parser::Associativity associativity,
    parser::InfixFn fn) {
    return engine_.infix(std::move(domain), std::move(op), precedence, associativity, std::move(fn));
}

registry::RegisterStatus LanguageOptionsController::postfix(std::string domain, std::string op, int precedence, parser::PostfixFn fn) {
    return engine_.postfix(std::move(domain), std::move(op), precedence, std::move(fn));
}

registry::RegisterStatus LanguageOptionsController::expression(std::string kind, runtime::ExprHandler handler) {
    return engine_.expression(std::move(kind), std::move(handler));
}

registry::RegisterStatus LanguageOptionsController::statement(std::string kind, runtime::StmtHandler handler) {
    return engine_.statement(std::move(kind), std::move(handler));
}

registry::RegisterStatus LanguageOptionsController::declaration(std::string kind, runtime::DeclHandler handler) {
    return engine_.declaration(std::move(kind), std::move(handler));
}

registry::RegisterStatus LanguageOptionsController::binaryOperator(std::string op, runtime::BinaryHandler handler) {
    return engine_.binaryOperator(std::move(op), std::move(handler));
}

void LanguageOptionsController::setBinaryNodeKind(std::string kind) {
    engine_.setBinaryNodeKind(std::move(kind));
}

registry::RegisterStatus LanguageOptionsController::hir(std::string nodeKind, ir::LoweringRegistry::HIRLowerer lowerer) {
    return engine_.hir(std::move(nodeKind), std::move(lowerer));
}

registry::RegisterStatus LanguageOptionsController::mir(std::string instructionKind, ir::LoweringRegistry::MIRLowerer lowerer) {
    return engine_.mir(std::move(instructionKind), std::move(lowerer));
}

void LanguageOptionsController::validateFeature(const LanguageFeatureInfo &info) const {
    if (info.name.empty()) {
        throw std::runtime_error("LanguageOptionsController::validateFeature: feature name cannot be empty");
    }

    if (hasFeature(info.name)) {
        throw std::runtime_error("LanguageOptionsController::validateFeature: duplicate feature '" + info.name + "'");
    }

    for (const std::string &conflict : info.conflicts) {
        if (hasFeature(conflict)) {
            throw std::runtime_error("LanguageOptionsController::validateFeature: feature '" + info.name + "' conflicts with '" + conflict + "'");
        }
    }

    for (const std::string &dependency : info.dependencies) {
        if (!hasFeature(dependency)) {
            throw std::runtime_error("LanguageOptionsController::validateFeature: feature '" + info.name + "' requires feature '" + dependency + "'");
        }
    }

    for (const std::string &capability : info.requiredCapabilities) {
        if (!hasCapability(capability)) {
            throw std::runtime_error("LanguageOptionsController::validateFeature: feature '" + info.name + "' requires capability '" + capability + "'");
        }
    }

    for (const VersionRequirement &requirement : info.versionRequirements) {
        const LanguageFeatureInfo *required{feature(requirement.feature)};

        if (!required) {
            throw std::runtime_error("LanguageOptionsController::validateFeature: feature '" + info.name + "' requires versioned feature '" + requirement.feature + "'");
        }

        if (!requirement.minVersion.empty() && required->version < requirement.minVersion) {
            throw std::runtime_error(
                "LanguageOptionsController::validateFeature: feature '" + info.name + "' requires '" + requirement.feature
                + "' >= " + requirement.minVersion + ", got " + required->version);
        }
    }
}

void LanguageOptionsController::rememberFeature(LanguageFeatureInfo info) {
    features_.push_back(std::move(info));
}

void LanguageOptionsController::installTransactional(const LanguageFeature &feature, const LanguageFeatureInfo &info) {
    controllers::EngineController snapshot{engine_.snapshot()};

    try {
        feature.install(*this);
        rememberFeature(info);
    } catch (...) {
        engine_.restore(snapshot);
        throw;
    }
}

void LanguageOptionsController::installDirect(const LanguageFeature &feature, const LanguageFeatureInfo &info) {
    feature.install(*this);
    rememberFeature(info);
}

} // namespace novac::language
