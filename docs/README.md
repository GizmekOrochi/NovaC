# NovaC documentation

- `config/` contains Sphinx/Doxygen configuration, hand-written RST sources, and compile-tested examples.
- `documentation/` is generated output.

From the repository root:

```bash
make doc
```

Open `docs/documentation/index.html`.

For release validation, compile/run the documentation examples and make Sphinx warnings fatal:

```bash
make doc-strict
```
