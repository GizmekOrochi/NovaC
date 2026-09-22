# Changelog

All notable changes to NovaC are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Added a generic `TypeController` registry for language-defined types, primitive layout helpers, Atomic literal typing, operation overload resolution, and custom semantic rules.
- Added generic type-to-type conversions with ranked direct implicit conversion resolution and forward type declarations.
- Added final type-system validation for unresolved declarations and dangling conversion endpoints.

### Changed

- Kept the Types asset compact: one controller implementation plus `model/Type.hpp` and `semantics/TypeSemantics.hpp`.
- Type semantics reuse Atomic `LiteralFeature` and `OperationFeature` identities instead of maintaining a parallel literal/operator hierarchy.
- Moved conversions out of `PrimitiveType`; conversions are now relations between arbitrary registered types.
- Made committed extension payloads const and made `TypeDefinition::freeze()` framework-controlled with an `onFreeze()` extension hook.
- Custom operation semantic rules now return declarative conversion requests; NovaC materializes conversion metadata from the registered conversion table.
- Separated primitive semantic bit width from physical storage width and kept alignment policy language/backend-defined.
- Hardened overload resolution with arity/type validation, explicit ambiguity states, direct-conversion-only semantics, and conversion-cost overflow protection.

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
