#pragma once

#include "execution/Runtime.hpp"
#include "foundation/Diagnostic.hpp"
#include "foundation/Ids.hpp"
#include "foundation/registry/Registry.hpp"
#include "syntax/Lexer.hpp"
#include "syntax/Node.hpp"
#include "syntax/Parser.hpp"
#include "syntax/Token.hpp"
#include "transformation/IR.hpp"

#include <functional>
#include <string>
#include <vector>

namespace novac::controllers {

class EngineController;

class EngineFeature final {
public:
    using Installer = std::function<void(EngineController &)>;

    explicit EngineFeature(std::string name);

    EngineFeature &version(std::string value);
    EngineFeature &description(std::string value);
    EngineFeature &provides(std::string capability);
    EngineFeature &requires(std::string capability);
    EngineFeature &conflictsWith(std::string featureName);
    EngineFeature &onInstall(Installer installer);

    const std::string &name() const;
    const std::string &version() const;
    const std::string &description() const;
    const std::vector<std::string> &capabilities() const;
    const std::vector<std::string> &requiredCapabilities() const;
    const std::vector<std::string> &conflicts() const;

    void install(EngineController &engine) const;

private:
    std::string name_;
    std::string version_;
    std::string description_;
    std::vector<std::string> capabilities_;
    std::vector<std::string> requiredCapabilities_;
    std::vector<std::string> conflicts_;
    std::vector<Installer> installers_;
};

struct EngineControllerOptions final {
    registry::DuplicatePolicy duplicatePolicy{registry::DuplicatePolicy::Error};
    std::string startDomain{};
};

class EngineController final {
public:
    explicit EngineController(EngineControllerOptions options = {});

    registry::RegisterStatus keyword(std::string keyword);
    registry::RegisterStatus symbol(std::string symbol);

    registry::RegisterStatus node(ast::NodeSchema schema);

    registry::RegisterStatus parseRule(std::string domain, std::string key, parser::ParseFn fn);
    registry::RegisterStatus parseRule(const ids::ParseDomain &domain, std::string key, parser::ParseFn fn);

    registry::RegisterStatus fallback(std::string domain, parser::ParseFn fn);
    registry::RegisterStatus fallback(const ids::ParseDomain &domain, parser::ParseFn fn);

    registry::RegisterStatus prefix(std::string domain, std::string key, parser::PrefixFn fn);
    registry::RegisterStatus prefix(const ids::ParseDomain &domain, std::string key, parser::PrefixFn fn);

    registry::RegisterStatus infix(std::string domain, std::string op, int precedence, parser::InfixFn fn);
    registry::RegisterStatus infix(const ids::ParseDomain &domain, std::string op, int precedence, parser::InfixFn fn);

    registry::RegisterStatus postfix(std::string domain, std::string op, int precedence, parser::PostfixFn fn);
    registry::RegisterStatus postfix(const ids::ParseDomain &domain, std::string op, int precedence, parser::PostfixFn fn);

    registry::RegisterStatus expression(std::string kind, runtime::ExprHandler handler);
    registry::RegisterStatus expression(const ids::NodeKind &kind, runtime::ExprHandler handler);

    registry::RegisterStatus statement(std::string kind, runtime::StmtHandler handler);
    registry::RegisterStatus statement(const ids::NodeKind &kind, runtime::StmtHandler handler);

    registry::RegisterStatus declaration(std::string kind, runtime::DeclHandler handler);
    registry::RegisterStatus declaration(const ids::NodeKind &kind, runtime::DeclHandler handler);

    registry::RegisterStatus binaryOperator(std::string op, runtime::BinaryHandler handler);
    registry::RegisterStatus binaryOperator(const ids::Operation &op, runtime::BinaryHandler handler);

    registry::RegisterStatus hir(std::string nodeKind, ir::LoweringRegistry::HIRLowerer lowerer);
    registry::RegisterStatus hir(const ids::NodeKind &nodeKind, ir::LoweringRegistry::HIRLowerer lowerer);

    registry::RegisterStatus mir(std::string instructionKind, ir::LoweringRegistry::MIRLowerer lowerer);
    registry::RegisterStatus mir(const ids::Operation &instructionKind, ir::LoweringRegistry::MIRLowerer lowerer);

    ast::NodePtr makeNode(std::string kind) const;
    ast::NodePtr makeNode(const ids::NodeKind &kind) const;

    std::vector<token::Token> tokenize(const std::string &source) const;
    ast::NodePtr parse(const std::string &source) const;
    ast::NodePtr parse(const std::string &source, std::string startDomain) const;
    ast::NodePtr parseTokens(std::vector<token::Token> tokens) const;
    ast::NodePtr parseTokens(std::vector<token::Token> tokens, std::string startDomain) const;

    void validate(const ast::Node &node) const;

    runtime::Value eval(const ast::Node &node) const;
    void exec(const ast::Node &node) const;

    ir::HIRModule lowerToHIR(const ast::Node &node) const;
    ir::MIRModule lowerToMIR(const ir::HIRModule &hir) const;
    ir::MIRModule lowerToMIR(const ast::Node &node) const;

    void install(const EngineFeature &feature);
    bool hasFeature(const std::string &name) const;
    bool hasCapability(const std::string &capability) const;

    void setStartDomain(std::string startDomain);
    const std::string &startDomain() const;

    diagnostics::DiagnosticEngine &diagnostics();
    const diagnostics::DiagnosticEngine &diagnostics() const;

    lexer::LexerRegistry &lexer();
    const lexer::LexerRegistry &lexer() const;

    ast::NodeRegistry &nodes();
    const ast::NodeRegistry &nodes() const;

    parser::ParserRegistry &parser();
    const parser::ParserRegistry &parser() const;

    runtime::RuntimeRegistry &runtime();
    const runtime::RuntimeRegistry &runtime() const;

    ir::LoweringRegistry &lowering();
    const ir::LoweringRegistry &lowering() const;

private:
    struct InstalledFeature final {
        std::string name{};
        std::string version{};
        std::vector<std::string> capabilities{};
        std::vector<std::string> conflicts{};
    };

    void validateFeatureInstall(const EngineFeature &feature) const;
    void rememberFeature(const EngineFeature &feature);
    void requireStartDomain(const std::string &owner) const;

    lexer::LexerRegistry lexer_;
    ast::NodeRegistry nodes_;
    parser::ParserRegistry parser_;
    runtime::RuntimeRegistry runtime_;
    ir::LoweringRegistry lowering_;
    diagnostics::DiagnosticEngine diagnostics_;
    std::string startDomain_;
    std::vector<InstalledFeature> features_;
};

} // namespace novac::controllers
