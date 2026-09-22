# NovaC documentation source

The official NovaC documentation is built with **Doxygen + Breathe + Sphinx + Furo**.

- `source/` contains the hand-written RST guides.
- `examples/` contains real C++ examples included by the guides with `literalinclude`.
- `Doxyfile` extracts the public C++ API from `src/include/novac`.
- `conf.py` configures Sphinx and Breathe.
- `_build/` is temporary generated data.
- `../documentation/` is the generated HTML site.

From the repository root:

```bash
make doc
```

Compile and execute the documentation examples only:

```bash
make doc-examples
```

For release checks:

```bash
make doc-strict
make release-check
```

`doc-strict` includes `doc-examples` and treats Sphinx warnings as errors.
`release-check` requires the normal NovaC test suite and `doc-strict` to pass.

Then open:

```text
docs/documentation/index.html
```

API facts belong in Doxygen comments in public headers. Concepts, tutorials,
architecture, workflows, installation/package guidance, and longer examples
belong in the Sphinx guides.
