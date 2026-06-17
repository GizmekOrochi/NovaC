#include "../../../tester.hpp"
#include "novac/engine/foundation/registry/Registry.hpp"
#include "novac/engine/foundation/registry/RegistryHelpers.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

using novac::registry::DuplicatePolicy;
using novac::registry::RegisterStatus;
using novac::registry::registerEntry;

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

} // namespace

TEST(Registry, DuplicatePolicyValuesAreDistinct) {
    CHECK(static_cast<int>(DuplicatePolicy::Error) != static_cast<int>(DuplicatePolicy::Replace));
    CHECK(static_cast<int>(DuplicatePolicy::Replace) != static_cast<int>(DuplicatePolicy::Ignore));
    CHECK(static_cast<int>(DuplicatePolicy::Error) != static_cast<int>(DuplicatePolicy::Ignore));
}

TEST(Registry, RegisterStatusValuesAreDistinct) {
    CHECK(static_cast<int>(RegisterStatus::Inserted) != static_cast<int>(RegisterStatus::Replaced));
    CHECK(static_cast<int>(RegisterStatus::Replaced) != static_cast<int>(RegisterStatus::Ignored));
    CHECK(static_cast<int>(RegisterStatus::Inserted) != static_cast<int>(RegisterStatus::Ignored));
}

TEST(RegistryHelpers, InsertsNewEntry) {
    std::unordered_map<std::string, int> map{};

    const RegisterStatus status{registerEntry(map, "answer", 42, DuplicatePolicy::Error, "TestRegistry")};

    CHECK(status == RegisterStatus::Inserted);
    CHECK(map.size() == 1);
    CHECK(map.at("answer") == 42);
}

TEST(RegistryHelpers, RejectsDuplicateWhenPolicyIsError) {
    std::unordered_map<std::string, int> map{};
    registerEntry(map, "answer", 42, DuplicatePolicy::Error, "TestRegistry");

    CHECK(throwsRuntimeError([&]() { registerEntry(map, "answer", 99, DuplicatePolicy::Error, "TestRegistry"); }));
    CHECK(map.size() == 1);
    CHECK(map.at("answer") == 42);
}

TEST(RegistryHelpers, ReplacesDuplicateWhenPolicyIsReplace) {
    std::unordered_map<std::string, int> map{};
    registerEntry(map, "answer", 42, DuplicatePolicy::Error, "TestRegistry");

    const RegisterStatus status{registerEntry(map, "answer", 99, DuplicatePolicy::Replace, "TestRegistry")};

    CHECK(status == RegisterStatus::Replaced);
    CHECK(map.size() == 1);
    CHECK(map.at("answer") == 99);
}

TEST(RegistryHelpers, IgnoresDuplicateWhenPolicyIsIgnore) {
    std::unordered_map<std::string, int> map{};
    registerEntry(map, "answer", 42, DuplicatePolicy::Error, "TestRegistry");

    const RegisterStatus status{registerEntry(map, "answer", 99, DuplicatePolicy::Ignore, "TestRegistry")};

    CHECK(status == RegisterStatus::Ignored);
    CHECK(map.size() == 1);
    CHECK(map.at("answer") == 42);
}

TEST(RegistryHelpers, SupportsMoveOnlyValues) {
    std::unordered_map<std::string, std::unique_ptr<int>> map{};

    const RegisterStatus inserted{registerEntry(map, "value", std::make_unique<int>(10), DuplicatePolicy::Error, "TestRegistry")};

    CHECK(inserted == RegisterStatus::Inserted);
    CHECK(map.at("value") != nullptr);
    CHECK(*map.at("value") == 10);

    const RegisterStatus replaced{registerEntry(map, "value", std::make_unique<int>(20), DuplicatePolicy::Replace, "TestRegistry")};

    CHECK(replaced == RegisterStatus::Replaced);
    CHECK(map.at("value") != nullptr);
    CHECK(*map.at("value") == 20);
}
