#include "novac/assets/essentials/functions/FunctionMain.hpp"

#include "novac/assets/essentials/EssentialsController.hpp"


namespace novac::assets::essentials::functions {


/**
 * @brief Implements the `info` operation.
 *
 * @return Value produced by the operation.
 */
EssentialInfo FunctionMainFeature::info() const {
    return {"essentials.functions.main","0.1.0", "Program entry point", {}, {}, {"function.entry"}, {}};
}



/**
 * @brief Installs the behavior provided by `install`.
 *
 * @param controller Value supplied for `controller`.
 */
void FunctionMainFeature::install(EssentialsController &controller) const {
    const FunctionSyntaxOptions options{controller.functions()};

    controller.functionRegistry().entryPoint(options.mainFunctionName);
}



/**
 * @brief Implements the `functionMain` operation.
 *
 * @return Value produced by the operation.
 */
EssentialPack functionMain(){
    EssentialPack pack{};
    pack.add<FunctionMainFeature>();

    return pack;
}


}
