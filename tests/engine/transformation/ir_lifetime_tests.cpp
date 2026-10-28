#include "../../tester.hpp"
#include "novac/engine/transformation/IR.hpp"
#include <type_traits>
using namespace novac;
template<typename Borrower, typename Dependency>
constexpr bool safeOneDependency =
    std::is_constructible_v<Borrower, Dependency &> &&
    std::is_constructible_v<Borrower, const Dependency &> &&
    !std::is_constructible_v<Borrower, Dependency &&> &&
    !std::is_constructible_v<Borrower, const Dependency &&>;
static_assert(safeOneDependency<ir::ASTLoweringPass, ir::LoweringRegistry>);
static_assert(safeOneDependency<ir::HIRLoweringPass, ir::LoweringRegistry>);
TEST(EngineIR, BorrowedDependenciesRemainSafe) { CHECK(true); }
