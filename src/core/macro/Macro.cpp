#include "../../include/macro/Macro.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::macro {

void MacroRegistry::add(std::string name, MacroFn fn, SourceSpan definition) {
    const std::string macroName{name};

    definitions_[macroName] = std::move(definition);
    macros_[std::move(name)] = std::move(fn);
}

const MacroFn *MacroRegistry::find(const std::string &name) const {
    const auto iter{macros_.find(name)};

    if (iter == macros_.end()) {
        return nullptr;
    }

    return &iter->second;
}

SourceSpan MacroRegistry::definition(const std::string &name) const {
    const auto iter{definitions_.find(name)};

    if (iter == definitions_.end()) {
        return {};
    }

    return iter->second;
}

bool ExpansionContext::enter(const std::string &name) {
    if (depth_++ > recursionLimit_) {
        throw std::runtime_error(
            "ExpansionContext::enter: macro recursion limit reached");
    }

    return active_.insert(name).second;
}

void ExpansionContext::leave(const std::string &name) {
    active_.erase(name);

    if (depth_ > 0) {
        --depth_;
    }
}

std::string ExpansionContext::hygienicName(const std::string &base) {
    return "__macro_" + std::to_string(nextId_++) + "_" + base;
}

SourceSpan ExpansionContext::spanFor(const ast::Node &node) const {
    static_cast<void>(node);

    return {};
}

void ExpansionContext::map(SourceSpan generated, SourceSpan original) {
    currentMappings_.push_back({std::move(generated), std::move(original)});
}

void ExpansionContext::record(std::string name, SourceSpan call, SourceSpan def) {
    records_.push_back({
        std::move(name),
        std::move(call),
        std::move(def),
        currentMappings_
    });

    currentMappings_.clear();
}

const std::vector<ExpansionRecord> &ExpansionContext::records() const {
    return records_;
}

MacroExpansionPass::MacroExpansionPass(const MacroRegistry &registry)
    : registry_{registry} {}

ast::NodePtr MacroExpansionPass::expand(ast::NodePtr node) const {
    ExpansionContext context{};

    return expandNode(std::move(node), context);
}

ast::NodePtr MacroExpansionPass::expandNode(ast::NodePtr node, ExpansionContext &context) const {
    if (!node) {
        return nullptr;
    }

    if (node->kind() == "macro.call") {
        const std::string name{node->str("name")};

        if (!context.enter(name)) {
            throw std::runtime_error(
                "MacroExpansionPass::expandNode: recursive macro expansion '" + name + "'");
        }

        const MacroFn *fn{registry_.find(name)};
        ast::NodePtr expanded{fn ? (*fn)(*node) : node};

        if (expanded && expanded->has("generatedName")) {
            expanded->set(
                "generatedName",
                context.hygienicName(expanded->str("generatedName")));
        }

        context.record(name, context.spanFor(*node), registry_.definition(name));
        context.leave(name);

        return expanded;
    }

    auto &fields{
        const_cast<std::unordered_map<std::string, ast::Field> &>(node->fields())
    };

    for (auto &[name, field] : fields) {
        static_cast<void>(name);

        if (auto *child{std::get_if<ast::NodePtr>(&field)}) {
            *child = expandNode(*child, context);
        }

        if (auto *list{std::get_if<ast::NodeList>(&field)}) {
            for (ast::NodePtr &child : *list) {
                child = expandNode(child, context);
            }
        }
    }

    return node;
}

} // namespace novac::macro