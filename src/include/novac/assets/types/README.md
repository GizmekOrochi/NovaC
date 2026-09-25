# NovaC Types asset

`Types` is NovaC's small reusable type-semantic layer. It deliberately answers
three questions and stops there:

1. what types exist and which names refer to the same canonical type;
2. how types relate through explicit/implicit conversions;
3. what type a literal or operation produces.

It does **not** own parsing, literals, operators, memory, or ownership.
Literal/operator identity comes from Atomic. Optional complex-type behavior and
layout are exposed through capabilities and `LayoutController`, without turning
`TypeController` into an ABI-specific controller.

```text
types/
├── TypeController.hpp
├── LayoutController.hpp
├── StorageController.hpp
├── README.md
├── model/
│   ├── Type.hpp
│   └── Capabilities.hpp
├── aggregate/
│   └── StructType.hpp
└── semantics/
    └── TypeSemantics.hpp
```

The public surface stays compact while layout and the reference aggregate
implementation remain separate from `TypeController`.

## Independent asset lifecycle

`TypeController` is an asset-level controller, not a subsystem owned by
`EngineController`. Create it explicitly when the language needs static type
semantics:

```cpp
controllers::EngineController engine;
atomic::AtomicController atomic{engine};
types::TypeController types;
```

This keeps the engine unaware of optional type semantics. A dynamic language can
use the engine and Atomic without constructing a `TypeController` at all. Types
can still reuse Atomic feature descriptors for literal and operation typing.


## Generic nominal type registry

`TypeController` owns a generic `TypeId -> TypeDefinition` registry.
`PrimitiveType` is only one built-in descriptor. A language can add a custom
descriptor without changing the controller:

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

There is no parallel `TypeKind` enum. Runtime type queries use the actual C++
descriptor type, so the registry cannot disagree with a second category tag.

`TypeDefinition::freeze()` is framework-owned. A custom descriptor can extend
freeze behavior with protected `onFreeze()`, but cannot bypass freezing the base
typed metadata.

## Aliases and canonical identity

Aliases are names for an existing (or forward-declared) type. They are **not**
conversions:

```cpp
types.definePrimitive("u64").bits(64).commit();
types.registerAlias("size_t", TypeId{"u64"});

assert(types.canonical(TypeId{"size_t"}) == TypeId{"u64"});
assert(types.equivalent(TypeId{"size_t"}, TypeId{"u64"}));
```

Alias chains are supported and cycles are rejected. Operation signatures,
literal bindings and conversions are canonicalized when registered, so aliases
do not create duplicate semantic universes.

`equivalent(a, b)` is nominal equivalence after alias canonicalization. NovaC
does not silently infer structural equivalence.

## Primitive storage description

Primitive bit width is exact:

```cpp
types.definePrimitive("u24")
    .bits(24)
    .alignmentBits(32)
    .unsignedType()
    .commit();
```

``bits`` is the exact number of bits occupied by the type. `alignmentBits` is the exact storage alignment in bits and only has to be non-zero. `alignmentBytes()` is available as convenience syntax for byte-oriented targets.
Target/ABI-specific aggregate layout policy intentionally remains outside this
asset.

`IntegerRepresentation`, `FloatingPointRepresentation` and
`FixedPointRepresentation` are convenience descriptors, not universal format
models. A language can derive its own `PrimitiveRepresentation`.

Extensions are typed C++ values stored as `const` objects. Once a type,
conversion, signature, or semantic result is committed, its `ExtensionSet`
cannot be mutated.

## Conversions are relations between types

Conversions are owned globally by `TypeController`, not by `PrimitiveType`:

```cpp
types.registerConversion(
    TypeId{"Meters"},
    TypeId{"Feet"},
    ConversionKind::Explicit
);
```

The primitive builder keeps `conversionTo()` only as convenience syntax:

```cpp
types.declareType("u16");
types.definePrimitive("u8")
    .bits(8)
    .conversionTo(TypeId{"u16"}, ConversionKind::Implicit, 1)
    .commit();
types.definePrimitive("u16").bits(16).commit();
```

Only **one direct implicit conversion per operand** participates in overload
resolution. NovaC deliberately does not chain `u8 -> u16 -> u32`. Conversion
ranks are language-defined additive costs and cost arithmetic is overflow-safe.
Replacing a type descriptor does not silently delete conversion relations.

## Literal typing reuses Atomic literals

```cpp
atomic::literals::IntegerLiteralAtomic integerLiteral;
types.bindLiteral(integerLiteral, TypeId{"i32"});
auto type = types.resolveLiteral(integerLiteral, node);
```

A custom `LiteralFeature` can provide a resolver when type depends on the parsed
value. Resolver results are canonicalized, and returning an unknown type is a
configuration error. Feature identity is retained even if multiple literal
features produce the same AST node kind.

## Operation typing reuses Atomic operations

```cpp
atomic::operations::AddOperationAtomic add;
types.registerOperation(add, {TypeId{"u16"}, TypeId{"u16"}}, TypeId{"u16"});
```

Registration validates arity and all operand/result types. Concrete signatures
are resolved before semantic rules.

Custom rules are intentionally declarative:

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

Rules cannot forge conversion rank or metadata. The controller materializes
each request from the registered direct implicit conversion. Rule registration
uses ``std::unique_ptr`` ownership transfer and the controller exposes stored rules
read-only.

`resolveOperationDetailed()` returns structured failure information:

- `Resolved`
- `NoMatch`
- `Ambiguous`
- `UnknownOperand`
- `InvalidArity`
- `InvalidConfiguration`

The result also carries a human-readable diagnostic message. Ambiguous concrete
overloads include their operand/result signatures and conversion costs in
`diagnostic.candidates`.

## Validation and finalization

Forward declarations allow mutually dependent feature installation:

```cpp
types.declareType("Future");
// ... register relations referring to Future ...
types.definePrimitive("Future").bits(32).commit();
```

Call `validate()` to check that every declaration, alias, and conversion endpoint
is complete. `finalize()` performs the same validation and then rejects all
further type-system mutation:

```cpp
types.finalize();
assert(types.finalized());
```

Validation belongs to the Types asset itself. The engine does not own or
implicitly validate a `TypeController`; call `validate()` or `finalize()` on the
controller you created. This keeps optional type semantics out of the engine core.

## Complex types through capabilities

Complex types do not require a new branch in `TypeController`. Every
`TypeDefinition` can expose optional behavior through `TypeCapabilities`.
Capabilities are registered by interface type and become immutable when the
containing type is registered. There is one active implementation per interface;
registering another implementation replaces the previous behavior.

```cpp
class MatrixLayout final : public LayoutCapability {
public:
    TypeLayout compute(
        const LayoutContext &context,
        const TypeDefinition &type
    ) const override;
};

auto matrix = std::make_unique<MyMatrixType>(TypeId{"mat4"});
matrix->capabilities.emplace<LayoutCapability, MatrixLayout>();
types.registerType(std::move(matrix));
```

`ExtensionSet` remains metadata; `TypeCapabilities` represents behavior.
NovaC currently provides three generic capability interfaces:
`CompositionCapability`, `LayoutCapability`, and `MemberCapability`.

## Layout is a separate concern

`LayoutController` resolves layout without adding ABI methods to
`TypeDefinition` or special cases for structs to `TypeController`. Layouts are
expressed in bits so sub-byte and packed representations remain representable:

```cpp
LayoutController layouts{types};
TypeLayout layout = layouts.compute(TypeId{"Point"});
```

Primitive descriptors use their exact `bits` and exact bit alignment by
default. Any descriptor may install `LayoutCapability`; for primitives, a custom
layout may change alignment or attach layout metadata but must preserve the
declared exact bit size. `TypeLayout::sizeBytes()` and related helpers provide
rounded byte views when needed. Recursive by-value layouts are rejected instead
of recursing indefinitely, and repeated nested layouts are cached during one
resolution.

## Struct is the reference aggregate implementation

`StructType` demonstrates the generic capability model; it is not built into
`TypeController`:

```cpp
using namespace novac::assets::types::aggregate;

defineStruct(types, "Point")
    .field("x", TypeId{"i32"})
    .field("y", TypeId{"i32"})
    .commit();

LayoutController layouts{types};
auto point = layouts.compute(TypeId{"Point"});
```

The default struct installs `CompositionCapability`, `NaturalStructLayout`, and
`NamedMemberAccess`. A language can replace a capability in the builder or
register a completely unrelated `TypeDefinition` with its own behavior.


## Low-level bit storage

`StorageController` materializes layouts through a bit-addressed `BitStorage`.
Types may attach `StorageCapability` to override direct bit-for-bit load/store
behavior. The capability may validate or transform the type bits during load/store, but the type has only one width: `bits`. Raw storage accesses issued through `StorageContext` are bounded to the current value's `TypeLayout` range, and repeated layout queries share one resolution session/cache. `BitValue` exposes extraction,
insertion, shifts and masks for parity, tagging, compression, and other custom
representations without changing `TypeController`.
