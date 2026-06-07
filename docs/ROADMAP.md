# NovaC V7 Roadmap

V7 stabilizes the lexer/parser/node/builder layer and adds the missing high-power architecture:

- Generic instantiation engine
- Type variables, substitutions, unification
- Trait registry and constraint solver
- Overload registry and operator overload resolution
- Macro expansion pipeline with recursion guard
- AST -> HIR -> MIR lowering
- Backend registry and backend pipeline
- Interpreter, bytecode, LLVM, and C backend placeholders

The smoke test still validates the stable surface:

```c
int main() { return 40 + 2; }
```

The project is now prepared for generic functions, generic structs, traits, modules, macros, and multiple backends without changing the core lexer/parser/builder API.
