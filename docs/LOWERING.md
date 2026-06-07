# LoweringRegistry

`GrammarRegistry` is metadata. `ParserRegistry` is executable parsing. `LoweringRegistry` is executable lowering.

Features own their own lowering behavior:

```cpp
void ReturnFeature::lowering(LoweringRegistry& l) const {
    l.hir("return", [](const Node& n, HIRBuilder& out, const LoweringRegistry& reg) {
        reg.lowerHIR(*n.child("value"), out);
        out.emit("hir.return");
    });
}
```

The core lowering pass no longer needs to know all node kinds. It asks the registry.
