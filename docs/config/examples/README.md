# Examples: embedded source programs

Each complete C++ example contains a `const std::string source{R"(... )"};`
(or an equivalent raw literal) showing the input language or illustrative
language syntax for the feature being demonstrated.

- `custom_language.cpp` contains the full `fn factorial` / `fn start` program
  with recursion, a `for` loop, and `print` calls. It parses and evaluates it.
- `minimal.cpp`, `calculator.cpp`, `custom_control_flow.cpp`,
  `cardinal_direction.cpp`, `typed_minilang.cpp`, and `preprocessing.cpp`
  parse/tokenize the embedded source that their registered features support.
- `ir_builder.cpp`, `type_system.cpp`, `complex_types.cpp`, and
  `memory_model.cpp` intentionally demonstrate compiler infrastructure
  programmatically. Their `source` is an **illustration, not parsed/executed**:
  these standalone examples do not implement a complete frontend.

This distinction is intentional: the infrastructure examples must not
pretend to support full function declarations, control flow, or recursion.

Run `make doc-examples` from the repository root to compile and execute all
11 complete examples against the installed NovaC package.
