Type system
===========

``TypeController`` is NovaC's compact type-semantic layer. It defines type
identity, type relations, literal typing, and operation typing without imposing
a C-like type system. Parsing and operator/literal identities remain in Atomic;
ABI layout algorithms, memory, and ownership remain separate concerns.

Subsystem layout
----------------

.. code-block:: text

   novac/assets/types/
   ├── TypeController.hpp
   ├── LayoutController.hpp
   ├── model/
   │   ├── Type.hpp
   │   └── Capabilities.hpp
   ├── aggregate/
   │   └── StructType.hpp
   └── semantics/
       └── TypeSemantics.hpp

``TypeController`` stays focused on type identity and semantics. Layout and the
reference struct implementation have separate implementation files.

Independent from EngineController
---------------------------------

Types is optional. ``EngineController`` does not own a ``TypeController`` and
does not expose an ``engine.types()`` shortcut. Construct the asset explicitly:

.. code-block:: cpp

   controllers::EngineController engine;
   atomic::AtomicController atomic{engine};
   types::TypeController types;

A language that does not need static type semantics simply omits the last line.
The Types asset can still consume ``LiteralFeature`` and ``OperationFeature``
descriptors from Atomic.


Generic type registry
---------------------

The registry stores ``TypeDefinition`` descriptors. ``PrimitiveType`` is one
built-in descriptor; custom language types can derive directly from
``TypeDefinition``.

.. code-block:: cpp

   class VectorType final : public TypeDefinition {
   public:
      std::size_t lanes;
      VectorType(TypeId id, std::size_t lanes)
         : TypeDefinition(std::move(id)), lanes(lanes) {}
   };

   auto vec{std::make_unique<VectorType>(TypeId{"vec4"}, 4)};
   types.registerType(std::move(vec));

NovaC deliberately has no parallel type-category enum. Descriptor identity is
represented by the actual C++ type, avoiding disagreement between RTTI and a
second manually maintained tag.

NovaC owns ``TypeDefinition::freeze()``. A custom descriptor may override
``onFreeze()`` but cannot bypass freezing of base typed extensions.

Aliases and canonical identity
------------------------------

Aliases are alternate names, not conversions:

.. code-block:: cpp

   types.definePrimitive("u64").bits(64).commit();
   types.registerAlias("size_t", TypeId{"u64"});

   auto canonical{types.canonical(TypeId{"size_t"})}; // u64
   bool same{types.equivalent(TypeId{"size_t"}, TypeId{"u64"})}; // true

Alias chains are allowed; cycles are rejected. Registrations that carry type
identities are canonicalized, so an alias does not create duplicate overload or
conversion spaces. ``equivalent`` is nominal equivalence after alias
canonicalization; NovaC does not silently infer structural equivalence.

Primitive storage description
-----------------------------

Semantic precision and physical storage are separate:

.. code-block:: cpp

   types.definePrimitive("u24")
       .bits(24)
       .storageBits(32)
       .alignment(4)
       .unsignedType()
       .commit();

``bitWidth`` is meaningful precision. ``storageBits`` is physical storage and
defaults to ``bitWidth``. Alignment only has to be non-zero. More restrictive
ABI and aggregate-layout policies belong outside Types.

The built-in representation classes are convenience descriptors. Languages can
derive custom ``PrimitiveRepresentation`` types for unusual formats.

Typed extensions become immutable after commit. Stored metadata values are
``const`` objects, not merely guarded by a mutable flag.

Conversions are global type relations
-------------------------------------

Conversions are stored by ``TypeController`` rather than inside primitive
descriptors. This makes the same mechanism usable for primitives and future
structs, pointers, arrays, or language-specific descriptors.

.. code-block:: cpp

   types.registerConversion(TypeId{"Meters"}, TypeId{"Feet"}, ConversionKind::Explicit);

The primitive builder provides convenience syntax and supports forward targets:

.. code-block:: cpp

   types.declareType("u16");
   types.definePrimitive("u8")
       .bits(8)
       .conversionTo(TypeId{"u16"}, ConversionKind::Implicit, 1)
       .commit();
   types.definePrimitive("u16").bits(16).commit();

Only direct implicit conversions participate in overload resolution. NovaC does
not chain ``u8 -> u16 -> u32`` implicitly. Conversion ranks are additive costs
and cost accumulation is overflow-checked. Replacing a type descriptor leaves
independent conversion relations intact.

Literal semantics reuse Atomic literals
---------------------------------------

.. code-block:: cpp

   IntegerLiteralAtomic integerLiteral;
   types.bindLiteral(integerLiteral, TypeId{"i32"});
   auto type = types.resolveLiteral(integerLiteral, node);

Resolvers may return aliases; results are canonicalized. Returning an unknown
type is rejected. Typing is keyed by literal feature identity, so two custom
features may share an AST node kind without overwriting each other.

Operation semantics reuse Atomic operations
-------------------------------------------

.. code-block:: cpp

   AddOperationAtomic add;
   types.registerOperation(add, {TypeId{"u16"}, TypeId{"u16"}}, TypeId{"u16"});

Registration validates operation arity and every operand/result type. Concrete
signatures are authoritative; ``OperationSemanticRule`` objects are generic
fallbacks.

Semantic rules return a ``SemanticOperationResolution`` containing only the
result type and declarative conversion requests. ``TypeController`` resolves
those requests against the registered conversion graph, so custom rules cannot
forge conversion ranks or metadata.

Detailed resolution diagnostics
-------------------------------

``resolveOperationDetailed`` preserves failure reasons and explanatory data:

.. code-block:: cpp

   auto result{types.resolveOperationDetailed(add, operands)};
   if (!result.ok()) {
       std::cerr << result.diagnostic.message << '\n';
   }

Statuses are ``Resolved``, ``NoMatch``, ``Ambiguous``, ``UnknownOperand``,
``InvalidArity``, and ``InvalidConfiguration``. Ambiguous concrete overloads
also expose candidate signatures and their conversion costs through
``result.diagnostic.candidates``.

Concrete overloads are considered before semantic rules. Rule priority only
orders semantic rules; it never overrides a concrete signature.

Validation and finalization
---------------------------

``validate()`` rejects unresolved forward declarations, aliases resolving to
missing types, and dangling conversion endpoints. ``finalize()`` validates and
then freezes controller configuration:

.. code-block:: cpp

   types.finalize();
   assert(types.finalized());

After finalization, type/alias/conversion/literal/operation/rule registration is
rejected. Resolution remains available.

The controller owns its own lifecycle. Call ``validate()`` when you only want
to check references, or ``finalize()`` when the type configuration should become
immutable. ``EngineController`` deliberately has no knowledge of this asset.

Complete typed mini-language
----------------------------

The small snippets above can be combined into a complete language. The tested
example defines ``short``, ``int`` and ``long``, typed variables, ``+`` and ``-``,
and a ``print`` statement:

.. code-block:: text

   short a = 10;
   int b = a + 20;
   long c = b - 5;
   print a;
   print b;
   print c;

The example uses ``TypeController`` for primitive definitions, widening
conversions, literal typing, and operation resolution. A small AST type checker
tracks variable types and verifies each declaration before runtime evaluation.

See :doc:`../examples/typed_minilang` for the complete tested implementation.


Complex types and capabilities
------------------------------

Custom types may expose behavior without adding virtual methods to
``TypeDefinition``. ``ExtensionSet`` remains typed metadata, while
``TypeCapabilities`` contains optional polymorphic behavior.

The initial generic interfaces are deliberately small:

- ``CompositionCapability`` exposes logical child components;
- ``LayoutCapability`` computes physical layout;
- ``MemberCapability`` resolves named semantic members.

A completely custom type can install its own behavior:

.. code-block:: cpp

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

Capabilities become immutable when the descriptor is registered. No capability
is required: compile-time-only or opaque types may intentionally expose none.

Layout remains separate
-----------------------

``LayoutController`` is separate from ``TypeController``. Primitive layout uses
``storageBits`` and ``alignment``; a non-primitive type provides a
``LayoutCapability`` when it has a physical representation.

.. code-block:: cpp

   LayoutController layouts{types};
   TypeLayout layout{layouts.compute(TypeId{"Point"})};

Recursive by-value layout is diagnosed. This leaves future pointer/reference
semantics free to break recursive type graphs without teaching aggregate code
about pointers.

Struct as a reference implementation
------------------------------------

``StructType`` is implemented on top of the generic capability mechanism rather
than as a special case in ``TypeController``:

.. code-block:: cpp

   using namespace novac::assets::types::aggregate;

   defineStruct(types, "Example")
       .field("a", TypeId{"short"})
       .field("b", TypeId{"int"})
       .field("c", TypeId{"short"})
       .commit();

   types.finalize();

   LayoutController layouts{types};
   TypeLayout layout{layouts.compute(TypeId{"Example"})};

The descriptor can also be queried through its generic behavior interfaces:

.. code-block:: cpp

   const TypeDefinition &example{types.requireType(TypeId{"Example"})};
   const auto *members{example.capabilities.get<MemberCapability>()};
   const auto member{members->findMember(example, "b")};

   // member->type == TypeId{"int"}

With 2-byte ``short`` and 4-byte ``int``, ``NaturalStructLayout`` produces
field offsets 0, 4 and 8, a 4-byte alignment, and a final size of 12 bytes.
The same infrastructure can support tuples, records, matrices, GPU values, or
other language-defined complex types without changing ``TypeController``.

See :doc:`../examples/complex_types` for the complete tested example.


Custom literal type: two-bit cardinal directions
------------------------------------------------

Custom types can also define their own source literals through Atomic. For
example, a language can model the four cardinal directions as a two-bit type:

.. code-block:: text

   ← = 00
   ↑ = 01
   → = 10
   ↓ = 11

A custom ``LiteralFeature`` installs the four arrow symbols while Types only
needs to know that the feature produces ``CardinalDirection``:

.. code-block:: cpp

   CardinalDirectionLiteral arrows;
   atomic.use(arrows);

   types.definePrimitive("CardinalDirection")
       .bits(2)
       .storageBits(2)
       .alignment(1)
       .unsignedType()
       .representation<CardinalDirectionRepresentation>()
       .commit();

   types.bindLiteral(arrows, TypeId{"CardinalDirection"});

This keeps lexical syntax in Atomic and type identity/representation in Types.
No arrow token or cardinal-direction rule is hard-coded into NovaC.

See :doc:`../examples/cardinal_direction` for the complete tested example.
