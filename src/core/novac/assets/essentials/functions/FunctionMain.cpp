#include "novac/assets/essentials/functions/FunctionMain.hpp"

#include "novac/assets/essentials/EssentialsController.hpp"


namespace novac::assets::essentials::functions {


EssentialInfo FunctionMainFeature::info() const {
    return {"essentials.functions.main","0.1.0", "Program entry point", {}, {}, {"function.entry"}, {}};
}



void FunctionMainFeature::install(EssentialsController &controller) const {
    const FunctionSyntaxOptions options{controller.functions()};

    controller.functionRegistry().entryPoint(options.mainFunctionName);
}



EssentialPack functionMain(){
    EssentialPack pack{};
    pack.add<FunctionMainFeature>();

    return pack;
}


}