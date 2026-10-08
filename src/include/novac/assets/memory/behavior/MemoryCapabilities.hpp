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
    /**
     * @brief Destroys the `MemoryCapability` instance.
     */
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
    /**
     * @brief Constructs a `MemoryCapabilitySet` instance.
     */
    MemoryCapabilitySet() = default;

    /**
     * @brief Performs the `emplace` operation.
     *
     * @param args Value supplied for `args`.
     * @return Value produced by the operation.
     */
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

    /**
     * @brief Checks the condition represented by `has`.
     *
     * @return Value produced by the operation.
     */
    template <typename Capability>
    bool has() const noexcept {
        static_assert(std::is_base_of_v<MemoryCapability, Capability>, "Capability must derive from MemoryCapability");
        return values_.find(std::type_index(typeid(Capability))) != values_.end();
    }

    /**
     * @brief Returns the value exposed by `get`.
     *
     * @return Value produced by the operation.
     */
    template <typename Capability>
    Capability *get() noexcept {
        static_assert(std::is_base_of_v<MemoryCapability, Capability>, "Capability must derive from MemoryCapability");
        const auto it{values_.find(std::type_index(typeid(Capability)))};
        return it == values_.end() ? nullptr : static_cast<Capability *>(it->second.get());
    }

    /**
     * @brief Returns the value exposed by `get`.
     *
     * @return Value produced by the operation.
     */
    template <typename Capability>
    const Capability *get() const noexcept {
        static_assert(std::is_base_of_v<MemoryCapability, Capability>, "Capability must derive from MemoryCapability");
        const auto it{values_.find(std::type_index(typeid(Capability)))};
        return it == values_.end() ? nullptr : static_cast<const Capability *>(it->second.get());
    }

    /**
     * @brief Performs the `erase` operation.
     */
    template <typename Capability>
    void erase() {
        static_assert(std::is_base_of_v<MemoryCapability, Capability>, "Capability must derive from MemoryCapability");
        ensureMutable();
        values_.erase(std::type_index(typeid(Capability)));
    }

    /**
     * @brief Resets state through `clear`.
     */
    void clear();
    /**
     * @brief Performs the `freeze` operation.
     */
    void freeze() noexcept;
    /**
     * @brief Performs the `frozen` operation.
     *
     * @return Value produced by the operation.
     */
    bool frozen() const noexcept;
    /**
     * @brief Performs the `size` operation.
     *
     * @return Value produced by the operation.
     */
    std::size_t size() const noexcept;
    /**
     * @brief Checks the condition represented by `empty`.
     *
     * @return Value produced by the operation.
     */
    bool empty() const noexcept;

private:
    /**
     * @brief Ensures the invariant required by `ensureMutable`.
     */
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

    /**
     * @brief Destroys the `MemoryOperationHandler<Operation>` instance.
     */
    ~MemoryOperationHandler() override = default;
    /**
     * @brief Executes the behavior handled by `execute`.
     *
     * @param context Value supplied for `context`.
     * @param request Value supplied for `request`.
     * @return Value produced by the operation.
     */
    virtual Result execute(MemoryContext &context, const Request &request) const = 0;
};

} // namespace novac::assets::memory
