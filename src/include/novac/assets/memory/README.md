# NovaC Memory asset

`novac::assets::memory` is NovaC's low-level, bit-precise memory model.

It deliberately separates four responsibilities:

- **Types** describe what a value is.
- **Layout** describes how many bits a value occupies and its alignment.
- **Storage** describes how those exact bits are encoded/decoded for a type.
- **Memory** describes where bits live, how they are addressed, reserved, and checked.

The core memory model does not hard-code heap, stack, RAM, ROM, MMIO, GC,
borrowing, C pointers, paging, or byte-only addressing.

## Core concepts

- `Address`: `(AddressSpaceId, bitOffset)`.
- `AddressRange`: exact continuous bit range inside one address space.
- `AddressSpaceDefinition`: named addressable space with an exact bit width.
- `BitAccess`: pluggable raw backend for an address space.
- `MemoryRegion`: named view over an address range; regions may overlap.
- `Allocation`: raw reserved range. It intentionally stores no `TypeId`.
- `AllocationStrategy`: proposes placements; `MemoryController` owns allocation identity and lifecycle.
- `Lifetime`: optional alive/dead metadata for managed accesses.
- `MemoryReference`: typed interpretation of an address with optional allocation provenance.

`BitStorageAccess` is the standard in-memory backend. A language may replace it
with any custom `BitAccess` implementation.

## Exact-bit allocation

If a type has a two-bit layout with one-bit alignment, four typed allocations
can occupy one eight-bit address space at offsets 0, 2, 4, and 6. No byte-sized
placeholder is introduced by the memory model.

## Raw versus managed access

`loadBits` / `storeBits` operate directly on addresses and intentionally ignore
allocation lifetime/provenance. This is required for systems-language use cases.

Typed `load` / `store` validate a `MemoryReference`. When provenance is present,
they also require an active allocation, range containment, and a live lifetime.
They then reuse `StorageController`, preserving custom `StorageCapability`
semantics.

Ending a lifetime does not release the allocation, and releasing an allocation
does not end a lifetime. NovaC exposes mechanisms; a language defines policy.


## Open-ended behavior

Memory behavior is not limited to the operations known when NovaC was built.
`MemoryCapabilitySet` stores typed behavior interfaces and
`MemoryOperationHandler<Operation>` lets a language define a completely new
operation with its own `Request` and `Result` types. `MemoryController::invoke`
resolves handlers from the most specific scope to the broadest:

`Allocation -> MemoryRegion -> AddressSpaceDefinition -> controller-global`.

For example, paging, bank switching, transactions, cache operations, snapshots,
or a domain-specific device command can be introduced outside NovaC core.
Built-in raw `loadBits` / `storeBits` are themselves dispatched through
`LoadBitsOperation` / `StoreBitsOperation`, so an address space can replace
those semantics without a special branch in `MemoryController`.

Capability topology is frozen when address spaces, regions, and allocations are
committed. Implementations may still maintain runtime state. Controller-global
capabilities remain configurable for cross-cutting or fallback behaviors.

`MemoryContext` is the controlled façade given to operation handlers. It exposes
types, layout, storage, registered memory objects, and raw address-space access
without exposing `MemoryController`'s internal registries.
