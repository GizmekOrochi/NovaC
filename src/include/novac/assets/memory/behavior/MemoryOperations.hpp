#pragma once

#include "novac/assets/memory/behavior/MemoryCapabilities.hpp"
#include "novac/assets/memory/model/Memory.hpp"
#include "novac/assets/types/model/Storage.hpp"

#include <cstddef>

namespace novac::assets::memory {

/** Standard raw-load operation. Custom handlers may override it per scope. */
struct LoadBitsOperation {
    struct Request {
        Address address{};
        std::size_t bitSize{0};
    };
    using Result = types::BitValue;
};

/** Standard raw-store operation. Custom handlers may override it per scope. */
struct StoreBitsOperation {
    struct Request {
        Address address{};
        types::BitValue value{};
    };
    using Result = void;
};

/** Default raw-load implementation delegating to the address-space BitAccess. */
class DirectLoadBitsHandler final : public MemoryOperationHandler<LoadBitsOperation> {
public:
    /**
     * @brief Executes the behavior handled by `execute`.
     *
     * @param context Value supplied for `context`.
     * @param request Value supplied for `request`.
     * @return Value produced by the operation.
     */
    types::BitValue execute(MemoryContext &context, const Request &request) const override;
};

/** Default raw-store implementation delegating to the address-space BitAccess. */
class DirectStoreBitsHandler final : public MemoryOperationHandler<StoreBitsOperation> {
public:
    /**
     * @brief Executes the behavior handled by `execute`.
     *
     * @param context Value supplied for `context`.
     * @param request Value supplied for `request`.
     */
    void execute(MemoryContext &context, const Request &request) const override;
};

} // namespace novac::assets::memory
