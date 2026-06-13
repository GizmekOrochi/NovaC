#pragma once

#include "../../engine/EngineController.hpp"
#include "LanguageFeature.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace novac::language {

struct LanguageOptionsControllerOptions final {
    bool transactionalInstall{true};
};

class LanguageOptionsController final {
public:
    explicit LanguageOptionsController(
        controllers::EngineController &engine,
        LanguageOptionsControllerOptions options = {});

    LanguageOptionsController &use(const LanguageFeature &feature);
    LanguageOptionsController &use(std::unique_ptr<LanguageFeature> feature);

    controllers::EngineController &engine();
    const controllers::EngineController &engine() const;

    LanguageOptionsController &setStartDomain(std::string domain);
    const std::string &startDomain() const;

    bool hasFeature(const std::string &name) const;
    bool hasCapability(const std::string &capability) const;
    const LanguageFeatureInfo *feature(const std::string &name) const;
    const std::vector<LanguageFeatureInfo> &features() const;

    registry::RegisterStatus keyword(std::string keyword);
    registry::RegisterStatus symbol(std::string symbol);
    registry::RegisterStatus node(ast::NodeSchema schema);

    registry::RegisterStatus parseRule(std::string domain, std::string key, parser::ParseFn fn);
    registry::RegisterStatus fallback(std::string domain, parser::ParseFn fn);
    registry::RegisterStatus prefix(std::string domain, std::string key, parser::PrefixFn fn);
    registry::RegisterStatus infix(std::string domain, std::string op, int precedence, parser::InfixFn fn);
    registry::RegisterStatus infix(std::string domain, std::string op, int precedence, parser::Associativity associativity, parser::InfixFn fn);
    registry::RegisterStatus postfix(std::string domain, std::string op, int precedence, parser::PostfixFn fn);

    registry::RegisterStatus expression(std::string kind, runtime::ExprHandler handler);
    registry::RegisterStatus statement(std::string kind, runtime::StmtHandler handler);
    registry::RegisterStatus declaration(std::string kind, runtime::DeclHandler handler);
    registry::RegisterStatus binaryOperator(std::string op, runtime::BinaryHandler handler);
    void setBinaryNodeKind(std::string kind);

    registry::RegisterStatus hir(std::string nodeKind, ir::LoweringRegistry::HIRLowerer lowerer);
    registry::RegisterStatus mir(std::string instructionKind, ir::LoweringRegistry::MIRLowerer lowerer);

private:
    void validateFeature(const LanguageFeatureInfo &info) const;
    void rememberFeature(LanguageFeatureInfo info);
    void installTransactional(const LanguageFeature &feature, const LanguageFeatureInfo &info);
    void installDirect(const LanguageFeature &feature, const LanguageFeatureInfo &info);

    controllers::EngineController &engine_;
    LanguageOptionsControllerOptions options_;
    std::vector<LanguageFeatureInfo> features_;
    std::unordered_map<std::string, std::unique_ptr<LanguageFeature>> ownedFeatures_;
};

} // namespace novac::language
