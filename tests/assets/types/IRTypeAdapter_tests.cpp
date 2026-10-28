#include "../../tester.hpp"
#include "novac/assets/types/IRTypeAdapter.hpp"
#include <stdexcept>
using namespace novac::assets::types;
TEST(TypesIRAdapter, CanonicalAliasesShareOneIRIdentity) {
    TypeController types;
    types.definePrimitive("i32").bits(32).commit();
    types.definePrimitive("u8").bits(8).commit();
    types.registerAlias("word",TypeId{"i32"});
    IRTypeAdapter adapter{types,"some.language"};
    auto canonical=adapter.toIR(TypeId{"i32"});
    CHECK(adapter.toIR(TypeId{"word"})==canonical);
    CHECK(adapter.toIR(TypeId{"u8"})!=canonical);
    CHECK(adapter.space().contains(canonical));
    bool rejected=false;
    try{(void)adapter.toIR(TypeId{"undefined"});}catch(const std::runtime_error &){rejected=true;}
    CHECK(rejected);
}
