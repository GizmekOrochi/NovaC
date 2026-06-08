#pragma once

#include "../syntax/Node.hpp"
#include "../transformation/IR.hpp"
#include "../syntax/Lexer.hpp"
#include "../transformation/Macro.hpp"
#include "Module.hpp"
#include "../semantic/Overload.hpp"
#include "../syntax/Parser.hpp"
#include "../execution/Runtime.hpp"
#include "../semantic/Semantic.hpp"
#include "../semantic/Templates.hpp"
#include "../semantic/Traits.hpp"
#include "../semantic/TypeSystem.hpp"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace novac::language {

struct VersionRequirement {
    std::string feature{};
    std::string minVersion{};
};

struct FeatureInfo {
    std::string name{};
    std::string version{"0.1.0"};
    std::vector<std::string> dependencies{};
    std::vector<std::string> capabilities{};
    std::vector<std::string> conflicts{};
    std::vector<std::string> requiredCapabilities{};
    std::vector<VersionRequirement> versionRequirements{};
};

class Language;

class LanguageFeature {
public:
    virtual ~LanguageFeature();

    virtual FeatureInfo info() const = 0;

    virtual void tokens(lexer::LexerRegistry &lexer) const;
    virtual void nodes(ast::NodeRegistry &nodes) const;
    virtual void parser(parser::ParserRegistry &parser) const;
    virtual void semantic(semantic::SemanticRegistry &semantic) const;
    virtual void types(types::TypeRegistry &types) const;
    virtual void operators(overload::OperatorRegistry &operators, types::TypeRegistry &types) const;
    virtual void traits(traits::TraitRegistry &traits) const;
    virtual void templates(templates::TemplateRegistry &templates) const;
    virtual void macros(macro::MacroRegistry &macros) const;
    virtual void runtime(runtime::RuntimeRegistry &runtime) const;
    virtual void lowering(ir::LoweringRegistry &lowering) const;
};

class Language {
public:
    bool has(const std::string &name) const;
    bool hasCapability(const std::string &capability) const;
    const FeatureInfo *feature(const std::string &name) const;

    lexer::LexerRegistry lexer{};
    ast::NodeRegistry nodes{};
    parser::ParserRegistry parser{};
    semantic::SemanticRegistry semantic{};
    types::TypeRegistry types{};
    overload::OperatorRegistry operators{};
    traits::TraitRegistry traitRegistry{};
    templates::TemplateRegistry templateRegistry{};
    templates::InstantiationCache instantiations{};
    templates::SpecializationRegistry specializations{};
    macro::MacroRegistry macros{};
    ir::LoweringRegistry lowering{};
    module::ImportResolver modules{};
    runtime::RuntimeRegistry runtime{};
    std::vector<FeatureInfo> features{};
};

class LanguageBuilder {
public:
    template<class F, class... Args>
    LanguageBuilder &use(Args &&...args) {
        F feature{std::forward<Args>(args)...};

        return useObject(feature);
    }

    LanguageBuilder &useObject(const LanguageFeature &feature);

    Language build();

private:
    void installFeature(const LanguageFeature &feature, const FeatureInfo &info);

    void validateDependencies() const;
    void validateCapabilities() const;
    void validateVersions() const;
    void validateCycles() const;

    Language lang_{};
    std::unordered_map<std::string, FeatureInfo> pending_{};
    std::vector<std::string> installOrder_{};
};

} // namespace novac::language