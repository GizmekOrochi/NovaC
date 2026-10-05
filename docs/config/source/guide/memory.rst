Memory
======

NovaC's Memory asset is a **bit-precise memory workbench**, not a built-in
``malloc`` model. Its purpose is to let a language define where values live and
how memory policy should work without forcing C, Rust, a VM, or a particular
machine architecture onto the framework.

Responsibility split
--------------------

Keep these layers distinct:

.. code-block:: text

   TypeController       what is the value?
          |
   LayoutController     how many bits / what alignment?
          |
   MemoryController     where are those bits and who reserved them?
          |
   BitAccess            how are raw bits physically reached?

   StorageController    how does this type encode/decode its exact bits?

Typed Memory operations resolve both sides: Memory validates the location and
provenance while Storage remains the authority for type-specific bit encoding.

Addresses and address spaces
----------------------------

An ``Address`` is not a host pointer. It is the pair
``(AddressSpaceId, bitOffset)``. Two offset-zero addresses in different spaces
are therefore distinct.

``AddressSpaceDefinition`` gives a space a name and exact size in bits.
``BitAccess`` supplies the actual raw behavior. The standard
``BitStorageAccess`` keeps bits in process memory, while a custom backend may
implement device, transformed, mirrored, simulated, or otherwise unusual
storage.

Regions
-------

A ``MemoryRegion`` is a named view over an ``AddressRange``. It owns no separate
storage and imposes no parent/child hierarchy. Regions are allowed to overlap;
that makes aliases, overlays, and hardware-oriented views possible without
special cases in the core.

Allocations and strategies
--------------------------

An ``Allocation`` reserves a raw range. It deliberately has no ``TypeId``.
Typed allocation is just convenience syntax:

.. code-block:: text

   layout(type)
      -> bitSize + alignmentBits
      -> allocateBits(...)
      -> Allocation
      -> MemoryReference(address, type, allocation-id)

Placement is delegated to ``AllocationStrategy``. A strategy proposes a
region-relative offset; ``MemoryController`` still validates bounds and
alignment and owns allocation identity/lifecycle. ``LinearAllocationStrategy``
is the default first-fit implementation. A custom strategy may deliberately
return overlapping placements.

Lifetimes and provenance
------------------------

``Lifetime`` intentionally contains only an id, an alive/dead state, and typed
extensions. NovaC does not impose lifetime trees, borrowing, RAII, GC, or arena
semantics.

``MemoryReference`` combines an address and a ``TypeId`` with optional
``AllocationId`` provenance. With provenance, typed access checks:

* the allocation exists and is still active;
* the typed range remains inside the allocation;
* an attached lifetime is still alive.

Without provenance, a typed reference behaves as a raw typed address and only
address-space/type bounds are checked.

``dereference(reference)`` performs those checks explicitly and returns the exact
``AddressRange`` occupied by the referenced type; it does not create a new
ownership or view object.

Raw versus typed access
-----------------------

``loadBits`` / ``storeBits`` are deliberately raw. They do not consult
allocations or lifetimes. This is important for boot code, device access,
debuggers, binary tooling, kernels, and language runtimes that need controlled
unsafe operations.

``load`` / ``store`` operate on ``MemoryReference`` and perform managed checks
before reusing ``StorageController``. Ending a lifetime does not release an
allocation; releasing an allocation does not end a lifetime.


Open-ended behavior
-------------------

Raw backends and allocation strategies are not the end of Memory's extension
model. ``MemoryCapabilitySet`` can hold a typed
``MemoryOperationHandler<Operation>`` for any operation type defined by a
language, including operation types NovaC core has never seen.

An operation supplies its own ``Request`` and ``Result`` types. It is invoked
through ``MemoryController::invoke<Operation>()``. Handler lookup is scoped and
uses the most specific available behavior:

.. code-block:: text

   Allocation
       -> Region
       -> AddressSpace
       -> controller-global capabilities

This makes it possible to add address translation, bank switching,
transactions, snapshots, protection, device commands, or other future semantics
without adding a new method or enum to ``MemoryController``.

The standard raw operations use this mechanism too. ``loadBits`` dispatches
``LoadBitsOperation`` and ``storeBits`` dispatches ``StoreBitsOperation``.
An address space can therefore override even the standard raw semantics by
registering a handler before the space is committed.

Handlers receive ``MemoryContext`` rather than ``MemoryController`` itself.
The context is a controlled façade over Types, Layout, Storage, registered
memory objects, and the underlying ``BitAccess`` backend.


What the core does not impose
-----------------------------

The Memory asset has no hard-coded concept of:

* heap or stack;
* RAM, ROM, MMIO, VRAM, or pages;
* C pointers or host pointers;
* garbage collection or borrow checking;
* byte-only addressing;
* volatile/atomic/cache semantics;
* ABI-specific allocation policy.

Those can be built from the low-level mechanisms when a language actually needs
them.
