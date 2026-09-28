#include "novac/assets/memory/behavior/MemoryOperations.hpp"

#include "novac/assets/memory/MemoryContext.hpp"
#include "novac/assets/memory/access/BitAccess.hpp"

namespace novac::assets::memory {

types::BitValue DirectLoadBitsHandler::execute(MemoryContext &context, const Request &request) const {
    return static_cast<const MemoryContext &>(context).access(request.address.space)
        .loadBits(request.address.bitOffset, request.bitSize);
}

void DirectStoreBitsHandler::execute(MemoryContext &context, const Request &request) const {
    context.access(request.address.space).storeBits(request.address.bitOffset, request.value);
}

} // namespace novac::assets::memory
