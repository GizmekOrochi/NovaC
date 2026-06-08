#include "../../include/language/Language.hpp"

#include <functional>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace novac::language {

LanguageFeature::~LanguageFeature() = default;

void LanguageFeature::tokens(lexer::LexerRegistry &lexer) const {
    static_cast<void>(lexer);
}

void LanguageFeature::nodes(ast::NodeRegistry &nodes) const {
    static_cast<void>(nodes);
}

void LanguageFeature::parser(parser::ParserRegistry &parser) const {
    static_cast<void>(parser);
}

void LanguageFeature::semantic(semantic::SemanticRegistry &semantic) const {
    static_cast<void>(semantic);
}

void LanguageFeature::types(types::TypeRegistry &types) const {
    static_cast<void>(types);
}

void LanguageFeature::operators(overload::OperatorRegistry &operators, types::TypeRegistry &types) const {
    static_cast<void>(operators);static_cast<void>(types);
}

void LanguageFeature::traits(traits::TraitRegistry &traits) const {
    static_cast<void>(traits);
}

void LanguageFeature::templates(templates::TemplateRegistry &templates) const {
    static_cast<void>(templates);
}

void LanguageFeature::macros(macro::MacroRegistry &macros) const {
    static_cast<void>(macros);
}

void LanguageFeature::runtime(runtime::RuntimeRegistry &runtime) const {
    static_cast<void>(runtime);
}

void LanguageFeature::lowering(ir::LoweringRegistry &lowering) const {
    static_cast<void>(lowering);
}

bool Language::has(const std::string &name) const {
    for (const FeatureInfo &featureInfo : features)
        if (featureInfo.name == name)
            return true;

    return false;
}

bool Language::hasCapability(const std::string &capability) const {
    for (const FeatureInfo &featureInfo : features)
        for (const std::string &featureCapability : featureInfo.capabilities)
            if (featureCapability == capability)
                return true;

    return false;
}

const FeatureInfo *Language::feature(const std::string &name) const {
    for (const FeatureInfo &featureInfo : features)
        if (featureInfo.name == name)
            return &featureInfo;

    return nullptr;
}

LanguageBuilder &LanguageBuilder::useObject(const LanguageFeature &feature) {
    const FeatureInfo info{feature.info()};

    if (pending_.find(info.name) != pending_.end())
        throw std::runtime_error("LanguageBuilder::useObject: duplicate feature '" + info.name + "'");

    for (const std::string &conflict : info.conflicts)
        if (pending_.find(conflict) != pending_.end())
            throw std::runtime_error("LanguageBuilder::useObject: feature '" + info.name + "' conflicts with '" + conflict + "'");

    pending_[info.name] = info;
    installOrder_.push_back(info.name);

    installFeature(feature, info);

    return *this;
}

void LanguageBuilder::validateDependencies() const {
    for (const auto &[name, info] : pending_)
        for (const std::string &dependency : info.dependencies)
            if (pending_.find(dependency) == pending_.end())
                throw std::runtime_error("LanguageBuilder::validateDependencies: feature '" + name + "' requires missing feature '" + dependency + "'");
}

void LanguageBuilder::validateCapabilities() const {
    std::unordered_set<std::string> capabilities{};

    for (const auto &[name, info] : pending_) {
        static_cast<void>(name);

        for (const std::string &capability : info.capabilities)
            capabilities.insert(capability);
    }

    for (const auto &[name, info] : pending_)
        for (const std::string &capability : info.requiredCapabilities)
            if (capabilities.find(capability) == capabilities.end())
                throw std::runtime_error("LanguageBuilder::validateCapabilities: feature '" + name + "' requires missing capability '" + capability + "'");
}

void LanguageBuilder::validateCycles() const {
    std::unordered_map<std::string, int> state{};

    std::function<void(const std::string &)> visit{
        [&](const std::string &name) {
            state[name] = 1;

            const auto iter{pending_.find(name)};

            if (iter != pending_.end()) {
                for (const std::string &dependency : iter->second.dependencies) {
                    if (pending_.find(dependency) == pending_.end())
                        continue;

                    if (state[dependency] == 1)
                        throw std::runtime_error("LanguageBuilder::validateCycles: feature dependency cycle involving '" + dependency + "'");

                    if (state[dependency] == 0)
                        visit(dependency);
                }
            }

            state[name] = 2;
        }
    };

    for (const auto &[name, info] : pending_) {
        static_cast<void>(info);

        if (state[name] == 0)
            visit(name);
    }
}

void LanguageBuilder::validateVersions() const {
    for (const auto &[name, info] : pending_) {
        for (const VersionRequirement &requirement : info.versionRequirements) {
            const auto iter{pending_.find(requirement.feature)};

            if (iter == pending_.end())
                throw std::runtime_error("LanguageBuilder::validateVersions: feature '" + name + "' requires versioned missing feature '" + requirement.feature + "'");

            if (!requirement.minVersion.empty() && iter->second.version < requirement.minVersion)
                throw std::runtime_error("LanguageBuilder::validateVersions: feature '" + name + "' requires '" + requirement.feature + "' >= " + requirement.minVersion + ", got " + iter->second.version);
        }
    }
}

Language LanguageBuilder::build() {
    validateDependencies();
    validateCycles();
    validateCapabilities();
    validateVersions();

    return std::move(lang_);
}

void LanguageBuilder::installFeature(const LanguageFeature &feature, const FeatureInfo &info) {
    feature.tokens(lang_.lexer);
    feature.nodes(lang_.nodes);
    feature.types(lang_.types);
    feature.parser(lang_.parser);
    feature.semantic(lang_.semantic);
    feature.operators(lang_.operators, lang_.types);
    feature.traits(lang_.traitRegistry);
    feature.templates(lang_.templateRegistry);
    feature.macros(lang_.macros);
    feature.runtime(lang_.runtime);
    feature.lowering(lang_.lowering);

    lang_.features.push_back(info);
}

} // namespace novac::language