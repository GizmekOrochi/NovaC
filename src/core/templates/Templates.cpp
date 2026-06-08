#include "../../include/templates/Templates.hpp"

#include <cctype>
#include <sstream>
#include <utility>
#include <variant>

namespace novac::templates {

void TemplateRegistry::add(GenericDeclaration declaration) {
    const std::string name{declaration.name};

    generics_[name] = std::move(declaration);
}

const GenericDeclaration *TemplateRegistry::find(const std::string &name) const {
    const auto iter{generics_.find(name)};

    if (iter == generics_.end())
        return nullptr;

    return &iter->second;
}

bool SpecializationRegistry::add(SpecializationSymbol symbol) {
    const std::string mangledName{symbol.mangledName};

    return symbols_.emplace(mangledName, std::move(symbol)).second;
}

const SpecializationSymbol *SpecializationRegistry::findMangled(const std::string &mangled) const {
    const auto iter{symbols_.find(mangled)};

    if (iter == symbols_.end())
        return nullptr;

    return &iter->second;
}

const std::unordered_map<std::string, SpecializationSymbol> &SpecializationRegistry::all() const {
    return symbols_;
}

ast::NodePtr InstantiationCache::find(const std::string &key) const {
    const auto iter{cache_.find(key)};

    if (iter == cache_.end())
        return nullptr;

    return iter->second;
}

void InstantiationCache::remember(std::string key, ast::NodePtr node) {
    cache_[std::move(key)] = std::move(node);
}

ast::NodePtr ASTCloner::clone(const ast::NodePtr &node, const types::Substitution &substitution) const {
    if (!node)
        return nullptr;

    ast::NodePtr output{ast::Node::make(node->kind())};

    for (const auto &[name, field] : node->fields())
        output->set(name, cloneField(name, field, substitution));

    return output;
}

ast::Field ASTCloner::cloneField(const std::string &name, const ast::Field &field, const types::Substitution &substitution) const {
    if (const auto *stringValue{std::get_if<std::string>(&field)}) {
        const auto iter{substitution.find(*stringValue)};

        if ((name == "type" || name == "returnType") && iter != substitution.end())
            return iter->second->display();

        return *stringValue;
    }

    if (const auto *child{std::get_if<ast::NodePtr>(&field)})
        return clone(*child, substitution);

    if (const auto *list{std::get_if<ast::NodeList>(&field)}) {
        ast::NodeList output{};

        for (const ast::NodePtr &item : *list)
            output.push_back(clone(item, substitution));

        return output;
    }

    return field;
}

InstantiationEngine::InstantiationEngine(const TemplateRegistry &registry, InstantiationCache &cache, SpecializationRegistry &specializations, const traits::TraitRegistry &traits)
    : registry_{registry}, cache_{cache}, specializations_{specializations}, traits_{traits} {}

ast::NodePtr InstantiationEngine::instantiate(const std::string &name, const std::vector<types::TypeRef> &args) { const std::string key{makeKey(name, args)};

    if (ast::NodePtr cached{cache_.find(key)})
        return cached;

    const GenericDeclaration *declaration{registry_.find(name)};

    if (!declaration) return nullptr;
    if (declaration->parameters.size() != args.size()) return nullptr;

    types::Substitution substitution{};

    for (std::size_t index{}; index < args.size(); ++index)
        substitution[declaration->parameters[index].name] = args[index];

    traits::ConstraintSolver solver{traits_};

    for (const TemplateParameter &parameter : declaration->parameters)
        if (!solver.solve(parameter.constraints, substitution))
            return nullptr;

    ast::NodePtr body{ASTCloner{}.clone(declaration->body, substitution)};
    ast::NodePtr specialized{ast::Node::make("specialization")};
    const std::string mangledName{mangle(name, args)};

    specialized->set("template", name).set("key", key).set("mangledName", mangledName).set("body", body);
    specializations_.add({name, mangledName, args, body});
    cache_.remember(key, specialized);

    return specialized;
}

std::string InstantiationEngine::makeKey(const std::string &name, const std::vector<types::TypeRef> &args) {
    std::ostringstream output{};

    output << name << '<';

    for (std::size_t index{}; index < args.size(); ++index) {
        if (index != 0)
            output << ',';

        output << args[index]->display();
    }

    output << '>';

    return output.str();
}

std::string InstantiationEngine::mangle(const std::string &name, const std::vector<types::TypeRef> &args) {
    std::ostringstream output{};

    output << name;

    for (const types::TypeRef &arg : args) {
        output << "__";

        for (const char character : arg->display())
            output << (std::isalnum(static_cast<unsigned char>(character)) ? character : '_');
    }

    return output.str();
}

} // namespace novac::templates