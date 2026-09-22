# Changelog

All notable changes to NovaC are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
