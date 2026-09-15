#pragma once

#include "novac/engine/execution/Runtime.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace novac::assets::essentials::functions {


/**
 * @brief Stores native functions and the configured program entry point.
 *
 * FunctionRegistry is shared by function-related Essentials features. Native
 * functions receive already-parsed argument nodes together with the active
 * runtime context and return a runtime Value.
 */
class FunctionRegistry {
public:
    /** @brief Callable signature used by registered native functions. */
    using NativeFunction = std::function<runtime::Value(const ast::NodeList &, runtime::RuntimeContext &)>;

    /**
     * @brief Registers a native function by name.
     *
     * @param name Language-level function name.
     * @param function Native implementation to invoke.
     * @throws std::runtime_error If the name is empty, the callable is invalid,
     * or the name is already registered.
     */
    void native(const std::string &name, NativeFunction function);
    /**
     * @brief Selects the language-level entry-point function name.
     *
     * @param name Function name to use as the program entry point.
     */
    void entryPoint(const std::string &name);
    /**
     * @brief Tests whether a native function is registered.
     *
     * @param name Function name to query.
     * @return true when a native implementation exists.
     */
    bool hasNative(const std::string &name) const;

    /**
     * @brief Returns a registered native function.
     *
     * @param name Function name to resolve.
     * @return Registered native callable.
     * @throws std::runtime_error If no native function uses the supplied name.
     */
    const NativeFunction &getNative(const std::string &name) const;
    /** @brief Returns the configured entry-point function name. */
    const std::string &entryPoint() const;

private:
    std::unordered_map<std::string, NativeFunction> natives_{};
    std::string entryPoint_{};
};


}