#include "../../../tester.hpp"

#include "novac/assets/essentials/functions/FunctionRegistry.hpp"
#include "novac/engine/execution/Runtime.hpp"

namespace {

template <typename Fn>
bool throwsRuntimeError(Fn &&fn) {
    try {
        fn();
    } catch (const std::runtime_error &) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

using novac::assets::essentials::functions::FunctionRegistry;

TEST(FunctionRegistry, NativeRegistration) {
    FunctionRegistry registry{};

    registry.native("answer", [](const novac::ast::NodeList &, novac::runtime::RuntimeContext &) {return novac::runtime::Value::integer(42);});

    CHECK(registry.hasNative("answer"));
    CHECK(!registry.hasNative("missing"));
}

TEST(FunctionRegistry, EmptyNativeNameThrows) {
    FunctionRegistry registry{};

    CHECK(throwsRuntimeError([&]() {registry.native("", [](const novac::ast::NodeList &, novac::runtime::RuntimeContext &) {return novac::runtime::Value::voidValue();});}));
}

TEST(FunctionRegistry, EmptyNativeHandlerThrows) {
    FunctionRegistry registry{};

    CHECK(throwsRuntimeError([&]() {registry.native("empty", {});}));
}

TEST(FunctionRegistry, DuplicateNativeThrows) {
    FunctionRegistry registry{};

    auto handler{[](const novac::ast::NodeList &, novac::runtime::RuntimeContext &) {return novac::runtime::Value::voidValue();}};

    registry.native("duplicate", handler);

    CHECK(throwsRuntimeError([&]() {registry.native("duplicate", handler);}));
}

TEST(FunctionRegistry, UnknownNativeThrows) {
    FunctionRegistry registry{};

    CHECK(throwsRuntimeError([&]() {(void)registry.getNative("missing");}));
}

TEST(FunctionRegistry, EntryPointCanBeConfigured) {
    FunctionRegistry registry{};

    registry.entryPoint("start");

    CHECK(registry.entryPoint() == "start");
}

TEST(FunctionRegistry, EmptyEntryPointThrows) {
    FunctionRegistry registry{};

    CHECK(throwsRuntimeError([&]() {registry.entryPoint("");}));
}

} // namespace
