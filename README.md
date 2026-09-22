# NovaC

NovaC is a modular C++20 framework for building programming languages. Its engine provides tokenization, Pratt parsing, AST/runtime execution, diagnostics, and HIR/MIR infrastructure, while language behavior is composed through installable features instead of being hard-coded into the core.

## Quick start

Requirements: a C++20 compiler, `make`, and `ar`.

```bash
git clone https://github.com/GizmekOrochi/NovaC.git
cd NovaC
make -j
```

The default build produces the static library:

```text
lib/libNovaC.a
```

Public headers live under `src/include`. The umbrella header is `src/include/NovaC.hpp`, and version constants are exposed through `novac/Version.hpp`.

A minimal external build can link NovaC directly:

```bash
g++ -std=c++20 -I/path/to/NovaC/src/include main.cpp \
    /path/to/NovaC/lib/libNovaC.a -o my-language
```

## Architecture

NovaC is organized in three main layers:

- **Engine** — registries, lexer, Pratt parser, AST nodes, diagnostics, runtime, HIR and MIR.
- **Atomic assets** — literals and operators that provide the basic expression vocabulary of a language.
- **Essentials assets** — variables, functions, scopes, expression statements and control-flow features.

Features advertise requirements/capabilities and are installed through controllers. This keeps the engine generic while allowing language configurations to opt into only the syntax and runtime behavior they need.

More architecture notes are available in [`src/include/novac/engine/README.md`](src/include/novac/engine/README.md), [`src/include/novac/assets/atomic/README.md`](src/include/novac/assets/atomic/README.md), and [`src/include/novac/assets/essentials/README.md`](src/include/novac/assets/essentials/README.md).

## Build and installation

Build the static library:

```bash
make -j
```

Install headers, the static library, and the CMake package files:

```bash
sudo make install PREFIX=/usr/local
```

For packaging or local staging, use `DESTDIR`:

```bash
make install DESTDIR="$PWD/stage" PREFIX=/usr
```

Installed CMake consumers can use:

```cmake
find_package(NovaC 1.0 CONFIG REQUIRED)
target_link_libraries(my_target PRIVATE NovaC::NovaC)
```

The public umbrella header and version API are available after installation:

```cpp
#include <NovaC.hpp>

static_assert(NOVAC_VERSION_MAJOR == 1);
static_assert(novac::Version == "1.0.0");
```

Remove an installation made with the same prefix using:

```bash
sudo make uninstall PREFIX=/usr/local
```

## Tests and reliability

Run the normal test suite:

```bash
make test
```

Run the sanitizer suites:

```bash
make test-asan
make test-ubsan
```

ASan enables leak detection and aborts on the first memory error. UBSan aborts on the first detected undefined behavior and prints a stack trace.

## Documentation

Generate the documentation:

```bash
make doc
```

Compile and run documentation examples only:

```bash
make doc-examples
```

For release validation, warnings in the documentation are errors and examples are mandatory:

```bash
make doc-strict
```

The generated HTML is written to `docs/documentation/index.html`. Documentation setup details are in [`docs/README.md`](docs/README.md) and [`docs/config/README.md`](docs/config/README.md).

A complete local release gate is available as:

```bash
make release-check
```

This requires the normal tests, ASan, UBSan, and strict documentation validation to pass.

## Release

NovaC 1.0.0 is the first stable public release.

## License

NovaC is released under the [MIT License](LICENSE), copyright © 2026 GizmekOrochi.
