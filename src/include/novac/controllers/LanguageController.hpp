#pragma once

#include <memory>

namespace novac::ast {
class NodeRegistry;
} // namespace novac::ast

namespace novac::ir {
class LoweringRegistry;
} // namespace novac::ir

namespace novac::language {
class Language;
class LanguageBuilder;
} // namespace novac::language

namespace novac::lexer {
class LexerRegistry;
} // namespace novac::lexer

namespace novac::macro {
class MacroRegistry;
} // namespace novac::macro

namespace novac::module {
class ImportResolver;
} // namespace novac::module

namespace novac::overload {
class OperatorRegistry;
} // namespace novac::overload

namespace novac::parser {
class ParserRegistry;
} // namespace novac::parser

namespace novac::runtime {
class RuntimeRegistry;
} // namespace novac::runtime

namespace novac::semantic {
class SemanticRegistry;
} // namespace novac::semantic

namespace novac::templates {
class InstantiationCache;
class SpecializationRegistry;
class TemplateRegistry;
} // namespace novac::templates

namespace novac::traits {
class TraitRegistry;
} // namespace novac::traits

namespace novac::types {
class TypeRegistry;
} // namespace novac::types

namespace novac::controllers {

class LanguageController final {
public:
    LanguageController();
    explicit LanguageController(std::unique_ptr<language::Language> language);
    explicit LanguageController(language::LanguageBuilder &builder);

    ~LanguageController();

    LanguageController(const LanguageController &) = delete;
    LanguageController &operator=(const LanguageController &) = delete;

    LanguageController(LanguageController &&other) noexcept;
    LanguageController &operator=(LanguageController &&other) noexcept;

    language::Language &language();
    const language::Language &language() const;

    lexer::LexerRegistry &lexer();
    ast::NodeRegistry &nodes();
    parser::ParserRegistry &parser();
    semantic::SemanticRegistry &semantic();
    types::TypeRegistry &types();
    overload::OperatorRegistry &operators();
    traits::TraitRegistry &traits();
    templates::TemplateRegistry &templates();
    templates::InstantiationCache &instantiations();
    templates::SpecializationRegistry &specializations();
    macro::MacroRegistry &macros();
    ir::LoweringRegistry &lowering();
    module::ImportResolver &modules();
    runtime::RuntimeRegistry &runtime();

    const lexer::LexerRegistry &lexer() const;
    const ast::NodeRegistry &nodes() const;
    const parser::ParserRegistry &parser() const;
    const semantic::SemanticRegistry &semantic() const;
    const types::TypeRegistry &types() const;
    const overload::OperatorRegistry &operators() const;
    const traits::TraitRegistry &traits() const;
    const templates::TemplateRegistry &templates() const;
    const templates::InstantiationCache &instantiations() const;
    const templates::SpecializationRegistry &specializations() const;
    const macro::MacroRegistry &macros() const;
    const ir::LoweringRegistry &lowering() const;
    const module::ImportResolver &modules() const;
    const runtime::RuntimeRegistry &runtime() const;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

} // namespace novac::controllers
