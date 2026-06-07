#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace novac::module {

enum class Visibility {
    Private,
    Public
};

struct Export {
    std::string name{};
    Visibility visibility{Visibility::Public};
};

struct Import {
    std::string module{};
    std::string alias{};
};

struct ImportedSymbol {
    std::string module{};
    std::string name{};
    Visibility visibility{Visibility::Public};
};

struct Namespace {
    std::string name{};
    std::unordered_map<std::string, ImportedSymbol> symbols{};
};

struct Module {
    std::string name{};
    std::string path{};
    std::vector<Import> imports{};
    std::vector<Export> exports{};
    std::string source{};
    Namespace names{};
};

class ModuleCache {
public:
    bool has(const std::string &name) const;

    void store(Module module);

    const Module *find(const std::string &name) const;

private:
    std::unordered_map<std::string, Module> modules_;
};

class ImportResolver {
public:
    void registerVirtualFile(std::string moduleName, std::string source);

    void exportSymbol(
        const std::string &moduleName,
        std::string symbol,
        Visibility visibility = Visibility::Public);

    const Module &load(const std::string &name);

    ImportedSymbol resolveQualified(
        const std::string &moduleName,
        const std::string &symbol);

private:
    std::unordered_map<std::string, std::string> virtualFiles_;
    std::unordered_map<std::string, std::vector<Export>> manualExports_;
    std::unordered_set<std::string> loading_;
    ModuleCache cache_;
};

} // namespace novac::module