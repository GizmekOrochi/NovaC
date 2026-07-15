#pragma once

#include "novac/engine/execution/Runtime.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace novac::assets::essentials::functions {


class FunctionRegistry {
public:
    using NativeFunction = std::function<runtime::Value(const ast::NodeList &, runtime::RuntimeContext &)>;

    void native(const std::string &name, NativeFunction function);
    void entryPoint(const std::string &name);
    bool hasNative(const std::string &name) const;

    const NativeFunction &getNative(const std::string &name) const;
    const std::string &entryPoint() const;

private:
    std::unordered_map<std::string, NativeFunction> natives_{};
    std::string entryPoint_{};
};


}