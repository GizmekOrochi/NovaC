#pragma once

#include "../../engine/syntax/Node.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace novac::macro {

struct SourceSpan {
    std::string file{};
    int line{1};
    int column{1};
    int length{};
};

struct HygieneMark {
    std::string macroName{};
    int expansionId{};
};

struct TokenMapping {
    SourceSpan generated{};
    SourceSpan original{};
};

struct ExpansionRecord {
    std::string macroName{};
    SourceSpan callSite{};
    SourceSpan definitionSite{};
    std::vector<TokenMapping> mappings{};
};

using MacroFn = std::function<ast::NodePtr(const ast::Node &)>;

class MacroRegistry {
public:
    void add(std::string name, MacroFn fn, SourceSpan definition = {});

    const MacroFn *find(const std::string &name) const;

    SourceSpan definition(const std::string &name) const;

private:
    std::unordered_map<std::string, MacroFn> macros_;
    std::unordered_map<std::string, SourceSpan> definitions_;
};

class ExpansionContext {
public:
    bool enter(const std::string &name);
    void leave(const std::string &name);

    std::string hygienicName(const std::string &base);
    SourceSpan spanFor(const ast::Node &node) const;

    void map(SourceSpan generated, SourceSpan original);
    void record(std::string name, SourceSpan call, SourceSpan def);

    const std::vector<ExpansionRecord> &records() const;

private:
    std::unordered_set<std::string> active_;
    std::vector<TokenMapping> currentMappings_;
    std::vector<ExpansionRecord> records_;
    int nextId_{};
    int depth_{};
    int recursionLimit_{128};
};

class MacroExpansionPass {
public:
    explicit MacroExpansionPass(const MacroRegistry &registry);

    ast::NodePtr expand(ast::NodePtr node) const;

private:
    ast::NodePtr expandNode(ast::NodePtr node, ExpansionContext &context) const;

    const MacroRegistry &registry_;
};

} // namespace novac::macro