#include "../../../tester.hpp"
#include "NovaC.hpp"

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace novac;

template <class Borrower, class Dependency, class... OtherArgs>
constexpr bool safeOneDependency =
    std::is_constructible_v<Borrower, Dependency &, OtherArgs...> &&
    std::is_constructible_v<Borrower, const Dependency &, OtherArgs...> &&
    !std::is_constructible_v<Borrower, Dependency &&, OtherArgs...> &&
    !std::is_constructible_v<Borrower, const Dependency &&, OtherArgs...>;

static_assert(safeOneDependency<lexer::Lexer, lexer::LexerRegistry>);
static_assert(std::is_constructible_v<parser::ParserContext, std::vector<token::Token>, parser::ParserRegistry &>);
static_assert(std::is_constructible_v<parser::ParserContext, std::vector<token::Token>, const parser::ParserRegistry &>);
static_assert(!std::is_constructible_v<parser::ParserContext, std::vector<token::Token>, parser::ParserRegistry &&>);
static_assert(!std::is_constructible_v<parser::ParserContext, std::vector<token::Token>, const parser::ParserRegistry &&>);

// Check both parser entry-point overloads: textual and typed parse domains.
static_assert(safeOneDependency<parser::Parser, parser::ParserRegistry, std::string>);
static_assert(safeOneDependency<parser::Parser, parser::ParserRegistry, const ids::ParseDomain &>);
static_assert(safeOneDependency<runtime::Runtime, runtime::RuntimeRegistry>);
static_assert(safeOneDependency<runtime::RuntimeContext, runtime::RuntimeRegistry>);
static_assert(safeOneDependency<semantics::SymbolTable, semantics::ScopeGraph>);
static_assert(safeOneDependency<assets::types::LayoutController, assets::types::TypeController>);
static_assert(safeOneDependency<assets::types::LayoutResolutionSession, assets::types::TypeController>);

using Types = assets::types::TypeController;
using Layouts = assets::types::LayoutController;
using Storage = assets::types::StorageController;
using Memory = assets::memory::MemoryController;

static_assert(std::is_constructible_v<Storage, Types &, Layouts &>);
static_assert(std::is_constructible_v<Storage, const Types &, const Layouts &>);
static_assert(!std::is_constructible_v<Storage, Types &&, Layouts &>);
static_assert(!std::is_constructible_v<Storage, Types &, Layouts &&>);
static_assert(!std::is_constructible_v<Storage, Types &&, Layouts &&>);
static_assert(!std::is_constructible_v<Storage, const Types &&, const Layouts &&>);

static_assert(std::is_constructible_v<Memory, Types &, Layouts &, Storage &>);
static_assert(std::is_constructible_v<Memory, const Types &, const Layouts &, const Storage &>);
static_assert(!std::is_constructible_v<Memory, Types &&, Layouts &, Storage &>);
static_assert(!std::is_constructible_v<Memory, Types &, Layouts &&, Storage &>);
static_assert(!std::is_constructible_v<Memory, Types &, Layouts &, Storage &&>);
static_assert(!std::is_constructible_v<Memory, Types &&, Layouts &&, Storage &&>);
static_assert(!std::is_constructible_v<Memory, const Types &&, const Layouts &&, const Storage &&>);

TEST(Lifetime, BorrowedDependenciesRejectTemporaryObjectsAtCompileTime) {
    // The static_asserts above are the regression test. This entry ensures the
    // compilation-only coverage is visible in the normal test report.
    CHECK(true);
}

} // namespace
