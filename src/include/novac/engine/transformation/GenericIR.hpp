#pragma once

#include "novac/engine/foundation/Metadata.hpp"
#include "novac/engine/foundation/registry/Registry.hpp"

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace novac::ir::generic {

struct ValueId {
    std::uint64_t value{};
    friend bool operator==(const ValueId &, const ValueId &) = default;
};

struct BlockId {
    std::uint64_t value{};
    friend bool operator==(const BlockId &, const BlockId &) = default;
};

/** Opaque type identity. Empty means unspecified. */
struct TypeRef {
    std::string id{};
    /**
     * @brief Performs the `valid` operation.
     *
     * @return Value produced by the operation.
     */
    bool valid() const noexcept { return !id.empty(); }
};

struct Region;

struct BlockArgument {
    ValueId value{};
    TypeRef type{};
    metadata::MetadataStore metadata{};
};

/**
 * Generic n-result operation. Operation names and traits are registered by
 * extensions; nested regions and block successors are optional.
 */
struct Operation {
    std::string name{};
    std::vector<ValueId> operands{};
    std::vector<ValueId> results{};
    std::vector<TypeRef> resultTypes{};
    std::vector<BlockId> successors{};
    std::vector<std::shared_ptr<Region>> regions{};
    metadata::MetadataStore metadata{};
};

struct Block {
    BlockId id{};
    std::string name{};
    std::vector<BlockArgument> arguments{};
    std::vector<Operation> operations{};
    metadata::MetadataStore metadata{};
};

struct Region {
    std::vector<Block> blocks{};
    metadata::MetadataStore metadata{};
};

struct Module {
    std::vector<std::shared_ptr<Region>> regions{};
    metadata::MetadataStore metadata{};
};

struct OperationSchema {
    std::string name{};
    std::size_t minOperands{};
    std::size_t maxOperands{std::numeric_limits<std::size_t>::max()};
    std::size_t minResults{};
    std::size_t maxResults{std::numeric_limits<std::size_t>::max()};
    std::vector<std::string> traits{};
    metadata::MetadataStore metadata{};
};

/** Registry used to document and validate extension-defined IR operations. */
class OperationRegistry final {
public:
    /**
     * @brief Constructs a `OperationRegistry` instance.
     *
     * @param duplicatePolicy Value supplied for `duplicatePolicy`.
     */
    explicit OperationRegistry(registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error)
        : duplicatePolicy_{duplicatePolicy} {}

    /**
     * @brief Adds data through `add`.
     *
     * @param schema Value supplied for `schema`.
     * @return Value produced by the operation.
     */
    registry::RegisterStatus add(OperationSchema schema) {
        if (schema.name.empty()) {
            throw std::runtime_error("OperationRegistry::add: operation name cannot be empty");
        }
        if (schema.minOperands > schema.maxOperands || schema.minResults > schema.maxResults) {
            throw std::runtime_error("OperationRegistry::add: invalid arity range");
        }
        const auto it{schemas_.find(schema.name)};
        if (it != schemas_.end()) {
            if (duplicatePolicy_ == registry::DuplicatePolicy::Ignore) {
                return registry::RegisterStatus::Ignored;
            }
            if (duplicatePolicy_ == registry::DuplicatePolicy::Error) {
                throw std::runtime_error("OperationRegistry::add: duplicate operation '" + schema.name + "'");
            }
            it->second = std::move(schema);
            return registry::RegisterStatus::Replaced;
        }
        schemas_.emplace(schema.name, std::move(schema));
        return registry::RegisterStatus::Inserted;
    }

    /**
     * @brief Finds the value requested by `find`.
     *
     * @param name Value supplied for `name`.
     * @return Value produced by the operation.
     */
    const OperationSchema *find(const std::string &name) const noexcept {
        const auto it{schemas_.find(name)};
        return it == schemas_.end() ? nullptr : &it->second;
    }

    /**
     * @brief Validates data through `validate`.
     *
     * @param operation Value supplied for `operation`.
     */
    void validate(const Operation &operation) const {
        const OperationSchema *schema{find(operation.name)};
        if (!schema) {
            throw std::runtime_error("OperationRegistry::validate: unknown operation '" + operation.name + "'");
        }
        if (operation.operands.size() < schema->minOperands || operation.operands.size() > schema->maxOperands) {
            throw std::runtime_error("OperationRegistry::validate: invalid operand count for '" + operation.name + "'");
        }
        if (operation.results.size() < schema->minResults || operation.results.size() > schema->maxResults) {
            throw std::runtime_error("OperationRegistry::validate: invalid result count for '" + operation.name + "'");
        }
        if (!operation.resultTypes.empty() && operation.resultTypes.size() != operation.results.size()) {
            throw std::runtime_error("OperationRegistry::validate: result type count does not match result count for '" + operation.name + "'");
        }
    }

private:
    std::unordered_map<std::string, OperationSchema> schemas_{};
    registry::DuplicatePolicy duplicatePolicy_;
};

} // namespace novac::ir::generic
