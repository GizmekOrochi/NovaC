# Changelog

All notable changes to NovaC are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
