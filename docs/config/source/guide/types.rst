Type system
===========

``TypeController`` provides reusable type infrastructure without imposing a
C-like type system. It reuses Atomic ``LiteralFeature`` and ``OperationFeature``
objects as semantic identities instead of creating parallel literal/operator
hierarchies.

Subsystem layout
----------------

::

   novac/assets/types/
   ├── TypeController.hpp
   ├── model/
   │   └── Type.hpp
   └── semantics/
       └── TypeSemantics.hpp

The implementation lives in one ``TypeController.cpp``.

Generic type registry
---------------------

The controller stores generic ``TypeDefinition`` descriptors. ``PrimitiveType``
is one built-in descriptor; languages may register custom derived descriptors
through ``registerType``.

.. code-block:: cpp

   class VectorType final : public TypeDefinition {
   public:
       VectorType(TypeId id, std::size_t lanes)
           : TypeDefinition(std::move(id)), lanes(lanes) {}
       std::size_t lanes;
   };

   auto vec = std::make_unique<VectorType>(TypeId{"vec4"}, 4);
   types.registerType(std::move(vec));

NovaC owns the freeze step. Custom descriptors may override ``onFreeze()`` but
cannot bypass freezing of the base typed extensions.

Primitive layout
----------------

Semantic precision and physical storage are separate:

.. code-block:: cpp

   types.definePrimitive("u24")
       .bits(24)
       .storageBits(32)
       .alignment(4)
       .unsignedType()
       .commit();

``bitWidth`` is meaningful precision. ``storageBits`` is physical storage and
defaults to ``bitWidth``. Alignment only has to be non-zero; stricter ABI rules
belong to a language/backend policy.

Typed extensions become immutable after commit. Stored objects are const, not
merely protected by a mutable ``frozen`` flag.

Conversions are global type relations
-------------------------------------

Conversions are stored by ``TypeController`` rather than inside primitive
descriptors. This makes the same mechanism usable for future structs, pointers,
arrays, or language-specific types.

.. code-block:: cpp

   types.registerConversion(
       TypeId{"Meters"},
       TypeId{"Feet"},
       ConversionKind::Explicit
   );

The primitive builder offers convenience syntax and supports forward targets:

.. code-block:: cpp

   types.declareType("u16");
   types.definePrimitive("u8")
       .bits(8)
       .conversionTo(TypeId{"u16"}, ConversionKind::Implicit, 1)
       .commit();
   types.definePrimitive("u16").bits(16).commit();

After language construction, call ``types.validate()``. It rejects unresolved
forward declarations and dangling conversion endpoints.

Only direct implicit conversions participate in overload resolution. NovaC does
not chain ``u8 -> u16 -> u32`` implicitly. Conversion ranks are additive costs;
cost accumulation is overflow-checked.

Literal semantics reuse Atomic literals
---------------------------------------

.. code-block:: cpp

   IntegerLiteralAtomic integerLiteral;
   types.bindLiteral(integerLiteral, TypeId{"i32"});
   auto type = types.resolveLiteral(integerLiteral, node);

Typing is bound to the literal feature id. ``resolveLiteral(node)`` is a
convenience fallback based on node kind; conflicting feature results are
reported rather than overwritten.

Operation semantics reuse Atomic operations
-------------------------------------------

.. code-block:: cpp

   AddOperationAtomic add;
   types.registerOperation(
       add,
       {TypeId{"u16"}, TypeId{"u16"}},
       TypeId{"u16"}
   );

Registration validates operation arity and every operand/result type. Concrete
signatures are authoritative; ``OperationSemanticRule`` objects are generic
fallbacks.

Semantic rules do not manufacture ``TypeConversion`` descriptors. They return a
``SemanticOperationResolution`` containing the result type and declarative
``SemanticConversionRequest`` entries. ``TypeController`` resolves each request
against its own conversion registry, so rank and metadata always come from the
registered conversion.

Detailed resolution
-------------------

``resolveOperationDetailed`` preserves failure reasons:

.. code-block:: cpp

   switch (result.status) {
   case OperationResolutionStatus::Resolved:       break;
   case OperationResolutionStatus::NoMatch:        break;
   case OperationResolutionStatus::Ambiguous:      break;
   case OperationResolutionStatus::UnknownOperand: break;
   case OperationResolutionStatus::InvalidArity:   break;
   }

Concrete overloads are considered before semantic rules. Rule priority only
orders semantic rules; it never overrides a concrete signature.

Engine integration
------------------

``EngineController`` owns a ``TypeController`` so an Engine feature can install
type semantics alongside parsing, runtime behavior, and lowering.
