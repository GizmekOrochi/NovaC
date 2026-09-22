# NovaC Types asset

`Types` provides reusable type semantics while staying small and consistent with
Atomic and Essentials.

```text
types/
├── TypeController.hpp
├── README.md
├── model/
│   └── Type.hpp
└── semantics/
    └── TypeSemantics.hpp
```

The implementation stays in one `TypeController.cpp`.

## Generic type registry

`TypeController` owns a generic `TypeId -> TypeDefinition` registry. Primitives
are only one built-in descriptor. Languages may register their own types without
adding another controller API.

```cpp
class VectorType final : public TypeDefinition {
public:
    VectorType(TypeId id, std::size_t lanes)
        : TypeDefinition(std::move(id)), lanes(lanes) {}

    std::size_t lanes;
};

auto vec = std::make_unique<VectorType>(TypeId{"vec4"}, 4);
types.registerType(std::move(vec));
```

`TypeDefinition::freeze()` is controlled by NovaC. A custom descriptor may
implement the protected `onFreeze()` hook, but cannot bypass freezing of the
base extension metadata.

## Primitive layout

Semantic width and storage width are separate:

```cpp
types.definePrimitive("u24")
    .bits(24)
    .storageBits(32)
    .alignment(4)
    .unsignedType()
    .commit();
```

`bitWidth` is semantic precision. `storageBits` is physical storage and defaults
to `bitWidth`. Alignment only has to be non-zero; stricter ABI policies belong
to a language/backend.

Extensions are typed C++ objects. Once committed, an `ExtensionSet` contains
`const` objects and cannot be mutated through retained references.

## Conversions are relations between types

Conversions belong to `TypeController`, not to `PrimitiveType`. They therefore
work for primitives, structs, pointers, arrays, or any future custom type.

```cpp
types.registerConversion(
    TypeId{"Meters"},
    TypeId{"Feet"},
    ConversionKind::Explicit
);
```

The primitive builder keeps `conversionTo()` as convenience syntax:

```cpp
types.declareType("u16");
types.definePrimitive("u8")
    .bits(8)
    .conversionTo(TypeId{"u16"}, ConversionKind::Implicit, 1)
    .commit();
types.definePrimitive("u16").bits(16).commit();
```

Call `types.validate()` after assembling a language. It rejects unresolved
forward type declarations and dangling conversion endpoints.

Only one direct implicit conversion per operand participates in overload
resolution. NovaC deliberately does not chain implicit conversions. Ranks are
additive costs and arithmetic is overflow-checked during overload resolution.

## Literal typing reuses Atomic literals

```cpp
atomic::literals::IntegerLiteralAtomic integerLiteral;
types.bindLiteral(integerLiteral, TypeId{"i32"});
auto type = types.resolveLiteral(integerLiteral, node);
```

A custom `LiteralFeature` may use a resolver when the type depends on the parsed
literal value. Literal feature identity is preserved, so two custom literal
features may intentionally share an AST node kind without overwriting each
other.

## Operation typing reuses Atomic operations

```cpp
atomic::operations::AddOperationAtomic add;
types.registerOperation(add, {TypeId{"u16"}, TypeId{"u16"}}, TypeId{"u16"});
```

Registration validates operation arity and every operand/result type. Concrete
signatures are resolved before semantic rules.

Custom semantic rules are intentionally declarative. They return a result type
and requested operand conversions:

```cpp
class Promote final : public OperationSemanticRule {
public:
    std::optional<SemanticOperationResolution> resolve(
        const TypeController &,
        std::span<const TypeId>
    ) const override {
        SemanticOperationResolution out;
        out.result = TypeId{"u16"};
        out.conversions.push_back({0, TypeId{"u16"}});
        return out;
    }
};
```

The rule cannot forge conversion rank or metadata. `TypeController` materializes
each request from the registered direct implicit conversion.

`resolveOperationDetailed()` distinguishes `Resolved`, `NoMatch`, `Ambiguous`,
`UnknownOperand`, and `InvalidArity`.
