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

This requires the normal tests, ASan, UBSan, the installed-package smoke test, and strict documentation validation to pass.

Continuous integration runs normal tests with GCC and Clang, sanitizer jobs, the installed CMake package smoke test, and strict documentation on clean Linux runners.

## Release

NovaC 1.0.0 is the first stable public release. Release notes are maintained in [`CHANGELOG.md`](CHANGELOG.md).

After committing all release changes and obtaining a green release gate, create a clean source archive from the committed `HEAD` with:

```bash
make dist
```

`make dist` refuses to package a dirty or untracked working tree and writes `dist/NovaC-1.0.0.zip` using `git archive`, so local build products, virtual environments, and `.git` metadata are excluded.

## License

NovaC is released under the [MIT License](LICENSE), copyright © 2026 GizmekOrochi.
