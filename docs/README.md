# NovaC documentation

NovaC's official documentation combines hand-written Sphinx guides with a
Doxygen-generated C++ API reference.

- `config/source/` contains the hand-written RST documentation.
- `config/examples/` contains real C++ programs used by the documentation.
- `config/Doxyfile` extracts the public API from `src/include/novac`.
- `documentation/` is generated HTML output and is not the source of truth.

From the repository root, build the documentation with:

```bash
make doc
```

Open `docs/documentation/index.html`.

Validate the executable examples independently with:

```bash
make doc-examples
```

For release-quality validation, run:

```bash
make doc-strict
```

`doc-strict` requires all documentation examples to compile and run, then
builds Sphinx with warnings treated as errors.

The repository release gate is:

```bash
make release-check
```

It requires the normal test suite, ASan, UBSan, the installed-package smoke test, and strict documentation validation to pass. GitHub CI also runs GCC and Clang test/package jobs plus dedicated sanitizer and documentation jobs on clean runners.
