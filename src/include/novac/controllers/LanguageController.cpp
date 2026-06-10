#include "novac/engine/controllers/LanguageController.hpp"

#include "novac/assets/language/Language.hpp"

#include <stdexcept>
#include <utility>

namespace novac::controllers {

struct LanguageController::Impl final {
    explicit Impl(std::unique_ptr<language::Language> language)
        : language{std::move(language)}
    {
        if (!this->language) {
            throw std::runtime_error{"LanguageController::Impl: language cannot be null"};
        }
    }

    std::unique_ptr<language::Language> language;
};

LanguageController::LanguageController()
    : impl_{std::make_unique<Impl>(std::make_unique<language::Language>())}
{
}

LanguageController::LanguageController(std::unique_ptr<language::Language> language)
    : impl_{std::make_unique<Impl>(std::move(language))}
{
}

LanguageController::LanguageController(language::LanguageBuilder &builder)
    : impl_{std::make_unique<Impl>(std::make_unique<language::Language>(builder.build()))}
{
}

LanguageController::~LanguageController() = default;

LanguageController::LanguageController(LanguageController &&other) noexcept = default;

LanguageController &LanguageController::operator=(LanguageController &&other) noexcept = default;

language::Language &LanguageController::language()
{
    return *impl_->language;
}

const language::Language &LanguageController::language() const
{
    return *impl_->language;
}

lexer::LexerRegistry &LanguageController::lexer()
{
    return impl_->language->lexer;
}

ast::NodeRegistry &LanguageController::nodes()
{
    return impl_->language->nodes;
}

parser::ParserRegistry &LanguageController::parser()
{
    return impl_->language->parser;
}

semantic::SemanticRegistry &LanguageController::semantic()
{
    return impl_->language->semantic;
}

types::TypeRegistry &LanguageController::types()
{
    return impl_->language->types;
}

overload::OperatorRegistry &LanguageController::operators()
{
    return impl_->language->operators;
}

traits::TraitRegistry &LanguageController::traits()
{
    return impl_->language->traitRegistry;
}

templates::TemplateRegistry &LanguageController::templates()
{
    return impl_->language->templateRegistry;
}

templates::InstantiationCache &LanguageController::instantiations()
{
    return impl_->language->instantiations;
}

templates::SpecializationRegistry &LanguageController::specializations()
{
    return impl_->language->specializations;
}

macro::MacroRegistry &LanguageController::macros()
{
    return impl_->language->macros;
}

ir::LoweringRegistry &LanguageController::lowering()
{
    return impl_->language->lowering;
}

module::ImportResolver &LanguageController::modules()
{
    return impl_->language->modules;
}

runtime::RuntimeRegistry &LanguageController::runtime()
{
    return impl_->language->runtime;
}

const lexer::LexerRegistry &LanguageController::lexer() const
{
    return impl_->language->lexer;
}

const ast::NodeRegistry &LanguageController::nodes() const
{
    return impl_->language->nodes;
}

const parser::ParserRegistry &LanguageController::parser() const
{
    return impl_->language->parser;
}

const semantic::SemanticRegistry &LanguageController::semantic() const
{
    return impl_->language->semantic;
}

const types::TypeRegistry &LanguageController::types() const
{
    return impl_->language->types;
}

const overload::OperatorRegistry &LanguageController::operators() const
{
    return impl_->language->operators;
}

const traits::TraitRegistry &LanguageController::traits() const
{
    return impl_->language->traitRegistry;
}

const templates::TemplateRegistry &LanguageController::templates() const
{
    return impl_->language->templateRegistry;
}

const templates::InstantiationCache &LanguageController::instantiations() const
{
    return impl_->language->instantiations;
}

const templates::SpecializationRegistry &LanguageController::specializations() const
{
    return impl_->language->specializations;
}

const macro::MacroRegistry &LanguageController::macros() const
{
    return impl_->language->macros;
}

const ir::LoweringRegistry &LanguageController::lowering() const
{
    return impl_->language->lowering;
}

const module::ImportResolver &LanguageController::modules() const
{
    return impl_->language->modules;
}

const runtime::RuntimeRegistry &LanguageController::runtime() const
{
    return impl_->language->runtime;
}

} // namespace novac::controllers
