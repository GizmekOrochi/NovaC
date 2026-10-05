# Changelog

All notable changes to NovaC are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

Target version: **1.3.0**.

### Added

- Added a bit-precise `MemoryController` with address spaces, ranges, regions, raw allocations, optional lifetimes, typed references, and provenance validation.
- Added pluggable `BitAccess` backends with a standard `BitStorageAccess` implementation for exact raw bit load/store.
- Added extensible `AllocationStrategy` placement with a default first-fit `LinearAllocationStrategy`.
- Added typed memory load/store that reuses `LayoutController` and `StorageController`, preserving custom type storage semantics without hard-coding heap/stack/pointer policy.
- Added Memory documentation, Doxygen API coverage, executable examples, and unit tests including exact two-bit packing.
- Added open-ended memory behavior dispatch through `MemoryCapabilitySet`, typed `MemoryOperationHandler<Operation>`, and `MemoryController::invoke<Operation>()`, allowing language-defined operations unknown to NovaC core.
- Added scoped behavior resolution from allocation to region to address space to controller-global capabilities, with built-in raw load/store using the same operation mechanism.
- Moved non-template Memory model, bit-access, allocation-strategy, capability-set, context, and standard-operation implementations into `.cpp` translation units.

## [1.2.0] - 2026-09-24

### Added

- Added generic type capabilities for composition, layout, and member behavior without special-casing complex types in `TypeController`.
- Added `LayoutController` with recursive layout resolution and cycle detection.
- Added `StructType` as the reference aggregate implementation with natural aligned layout and named member access.
- Added bit-granular layout descriptors with byte-view helpers, allowing sub-byte and packed custom types.
- Added per-resolution layout caching and preserved field metadata through generic member lookup.
- Added low-level bit-addressed `BitStorage`, `BitValue`, and `StorageController` APIs.
- Added `StorageCapability` so custom types can define exact bit-level load/store behavior instead of only descriptive layout.
- Hardened storage semantics with bounded bit access and shared layout-resolution sessions.
- Simplified primitive sizing to one exact `bits` width: the declared bits are the real bits occupied by the type, with no separate logical/storage width.

## [1.1.0] - 2026-09-23

### Added

- Added a generic `TypeController` registry for language-defined types, primitive layout helpers, Atomic literal typing, operation overload resolution, and custom semantic rules.
- Added generic type-to-type conversions with ranked direct implicit conversion resolution and forward type declarations.
- Added final type-system validation for unresolved declarations and dangling conversion endpoints.
- Added type aliases, canonical type identity, nominal alias equivalence, and cycle detection.
- Added structured overload-resolution diagnostics with ambiguous candidate/cost reporting.
- Added `TypeController::validate()` / `finalize()` for asset-local validation and freezing.


## [1.0.0] - 2026-09-22

### Added

- First stable public release of the NovaC C++20 language framework.
- Tokenization, Pratt parsing, AST/runtime execution, diagnostics, and HIR/MIR infrastructure.
- Installable language features for composing language behavior.
- Static-library installation with relocatable CMake package support through `NovaC::NovaC`.
- Normal, AddressSanitizer, and UndefinedBehaviorSanitizer test gates.
- Sphinx, Breathe, Doxygen, and executable-example documentation checks.
- Continuous integration coverage for GCC, Clang, sanitizers, package installation, and strict documentation.

### Reliability

- Release validation is available through `make release-check`.
- Source-release generation refuses a dirty Git working tree to prevent publishing uncommitted changes.
