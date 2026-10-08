#include "novac/assets/essentials/functions/FunctionRegistry.hpp"

#include <stdexcept>


namespace novac::assets::essentials::functions {



/**
 * @brief Implements the `native` operation.
 *
 * @param name Value supplied for `name`.
 * @param function Value supplied for `function`.
 */
void FunctionRegistry::native(const std::string &name, NativeFunction function) {
    if(name.empty()) throw std::runtime_error("FunctionRegistry::native: empty name");
    if(!function) throw std::runtime_error("FunctionRegistry::native: null function");
    if(natives_.contains(name)) throw std::runtime_error( "FunctionRegistry::native: duplicate native '" + name + "'");

    natives_.insert({name, std::move(function)});

}



/**
 * @brief Checks the condition represented by `hasNative`.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
bool FunctionRegistry::hasNative(const std::string &name) const {
    return natives_.contains(name);
}



/**
 * @brief Returns the value exposed by `getNative`.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
const FunctionRegistry::NativeFunction &FunctionRegistry::getNative(const std::string &name) const {
    auto it{natives_.find(name)};

    if(it == natives_.end()) {
        throw std::runtime_error("FunctionRegistry: unknown native '" + name + "'");
    }
    return it->second;
}



/**
 * @brief Implements the `entryPoint` operation.
 *
 * @param name Value supplied for `name`.
 */
void FunctionRegistry::entryPoint(const std::string &name) {
    if(name.empty()) {
        throw std::runtime_error("FunctionRegistry::entryPoint: empty name");
    }
    entryPoint_ = name;
}



/**
 * @brief Implements the `entryPoint` operation.
 *
 * @return Value produced by the operation.
 */
const std::string &FunctionRegistry::entryPoint() const{
    return entryPoint_;
}



}
