#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"
#include "novac/assets/essentials/functions/FunctionRegistry.hpp"

namespace novac::assets::essentials::functions {

/**
 * @brief Installs function declarations and function-call expressions.
 *
 * FunctionsFeature registers the syntax, AST schemas and runtime behavior
 * required for user-defined and native functions.
 *
 * Function declarations are stored in the active RuntimeContext as bound AST
 * nodes. Function calls first check FunctionRegistry for a native function and
 * otherwise resolve a previously bound user-defined declaration.
 *
 * User-defined calls create a nested runtime scope, evaluate arguments, bind
 * them to parameter names, execute the function body and collect an optional
 * return value before restoring the previous scope.
 */
class FunctionsFeature final : public EssentialFeature {
public:
    /**
     * @brief Returns metadata describing this feature.
     *
     * The feature exposes function declaration, function call and parameter
     * node kinds together with declaration, expression and callable traits.
     *
     * @return Feature registration and dependency information.
     */
    EssentialInfo info() const override;

    /**
     * @brief Installs function support into an Essentials controller.
     *
     * Installation registers the function keyword and punctuation, creates AST
     * schemas for parameters, declarations and calls, and installs an independent
     * transactional identifier-call prefix parser plus runtime handlers.
     *
     * User-defined calls validate argument count, evaluate arguments inside the
     * caller context, create a child scope for parameters and ensure that scope
     * is removed even when function execution throws.
     *
     * @param controller Controller receiving parser, schema, and runtime registrations.
     */
    void install(EssentialsController &controller) const override;
};

/**
 * @brief Creates a pack containing function declaration and call support.
 *
 * The returned pack contains one FunctionsFeature instance.
 *
 * @return Ownable feature pack.
 */
EssentialPack functions();

/**
 * @brief Creates the standard function feature pack.
 *
 * The standard pack combines return statements, function declarations and
 * calls, and program entry-point configuration.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::functions
