#include "../../include/module/Module.hpp"

#include <stdexcept>
#include <utility>

namespace novac::module {

bool ModuleCache::has(const std::string &name) const {
    return modules_.find(name) != modules_.end();
}

void ModuleCache::store(Module module) {
    const std::string name{module.name};

    modules_[name] = std::move(module);
}

const Module *ModuleCache::find(const std::string &name) const {
    const auto iter{modules_.find(name)};

    if (iter == modules_.end())
        return nullptr;

    return &iter->second;
}

void ImportResolver::registerVirtualFile(std::string moduleName, std::string source) {
    virtualFiles_[std::move(moduleName)] = std::move(source);
}

void ImportResolver::exportSymbol(const std::string &moduleName, std::string symbol, Visibility visibility) {
    manualExports_[moduleName].push_back({std::move(symbol), visibility});
}

const Module &ImportResolver::load(const std::string &name) {
    if (const Module *cached{cache_.find(name)})
        return *cached;

    if (loading_.find(name) != loading_.end())
        throw std::runtime_error("ImportResolver::load: circular import involving module '" + name + "'");

    const auto sourceIter{virtualFiles_.find(name)};

    if (sourceIter == virtualFiles_.end())
        throw std::runtime_error("ImportResolver::load: module not found '" + name + "'");

    loading_.insert(name);

    Module module{name, name, {}, manualExports_[name], sourceIter->second, Namespace{name, {}}};

    for (const Export &exportSymbol : module.exports)
        if (exportSymbol.visibility == Visibility::Public)
            module.names.symbols[exportSymbol.name] = { name, exportSymbol.name, exportSymbol.visibility };

    cache_.store(std::move(module));
    loading_.erase(name);

    const Module *loaded{cache_.find(name)};

    if (!loaded)
        throw std::runtime_error("ImportResolver::load: failed to cache module '" + name + "'");

    return *loaded;
}

ImportedSymbol ImportResolver::resolveQualified(const std::string &moduleName, const std::string &symbol) {
    const Module &module{load(moduleName)};
    const auto iter{module.names.symbols.find(symbol)};

    if (iter == module.names.symbols.end())
        throw std::runtime_error("ImportResolver::resolveQualified: module '" + moduleName + "' does not export symbol '" + symbol + "'");

    return iter->second;
}

} // namespace novac::module