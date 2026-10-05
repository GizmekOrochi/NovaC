#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace novac::assets::memory {

class MemoryContext;

/** Base class for behavior attached to memory scopes. */
class MemoryCapability {
public:
    virtual ~MemoryCapability() = default;
};

/**
 * Type-indexed behavior registry for memory scopes.
 *
 * Capabilities are indexed by their public interface type. Exactly one active
 * implementation exists per interface in a given scope. Freezing prevents the
 * capability topology from changing; capability implementations may still keep
 * their own mutable runtime state.
 */
class MemoryCapabilitySet final {
public:
    MemoryCapabilitySet() = default;

    template <typename Capability, typename Implementation = Capability, typename... Args>
    const Implementation &emplace(Args &&...args) {
        static_assert(std::is_base_of_v<MemoryCapability, Capability>, "Capability must derive from MemoryCapability");
        static_assert(std::is_base_of_v<Capability, Implementation>, "Implementation must derive from Capability");
        ensureMutable();
        auto value{std::make_shared<Implementation>(std::forward<Args>(args)...)};
        const Implementation &reference{*value};
        values_.insert_or_assign(std::type_index(typeid(Capability)), std::move(value));
        return reference;
    }

    template <typename Capability>
    bool has() const noexcept {
        static_assert(std::is_base_of_v<MemoryCapability, Capability>, "Capability must derive from MemoryCapability");
        return values_.find(std::type_index(typeid(Capability))) != values_.end();
    }

    template <typename Capability>
    Capability *get() noexcept {
        static_assert(std::is_base_of_v<MemoryCapability, Capability>, "Capability must derive from MemoryCapability");
        const auto it{values_.find(std::type_index(typeid(Capability)))};
        return it == values_.end() ? nullptr : static_cast<Capability *>(it->second.get());
    }

    template <typename Capability>
    const Capability *get() const noexcept {
        static_assert(std::is_base_of_v<MemoryCapability, Capability>, "Capability must derive from MemoryCapability");
        const auto it{values_.find(std::type_index(typeid(Capability)))};
        return it == values_.end() ? nullptr : static_cast<const Capability *>(it->second.get());
    }

    template <typename Capability>
    void erase() {
        static_assert(std::is_base_of_v<MemoryCapability, Capability>, "Capability must derive from MemoryCapability");
        ensureMutable();
        values_.erase(std::type_index(typeid(Capability)));
    }

    void clear();
    void freeze() noexcept;
    bool frozen() const noexcept;
    std::size_t size() const noexcept;
    bool empty() const noexcept;

private:
    void ensureMutable() const;

    std::unordered_map<std::type_index, std::shared_ptr<MemoryCapability>> values_{};
    bool frozen_{false};
};

/** Typed handler interface for a memory operation unknown to NovaC core. */
template <typename Operation>
class MemoryOperationHandler : public MemoryCapability {
public:
    using Request = typename Operation::Request;
    using Result = typename Operation::Result;

    ~MemoryOperationHandler() override = default;
    virtual Result execute(MemoryContext &context, const Request &request) const = 0;
};

} // namespace novac::assets::memory
