Generic complex types
=====================

This example is the complex-type counterpart of :doc:`type_system`. It keeps
``TypeController`` generic while building a concrete ``StructType`` on top of
NovaC's capability system.

The example defines two primitive types and then a struct equivalent to:

.. code-block:: text

   struct Example {
       short a;
       int   b;
       short c;
   }

The struct is registered as an ordinary ``TypeDefinition``. Its behavior comes
from capabilities installed by the reference struct implementation:

* ``CompositionCapability`` exposes its fields as logical components;
* ``LayoutCapability`` lets ``LayoutController`` compute bit layout;
* ``MemberCapability`` provides named member lookup.

There is no ``TypeController::defineStruct`` or ``requireStruct`` special case.
Consumers query generic type behavior instead.

.. literalinclude:: ../../examples/complex_types.cpp
   :language: cpp
   :linenos:

Expected output
---------------

.. code-block:: text

   Example
   bits=96 size=12 align=4
   0:0
   1:4
   2:8
   b:int

The natural layout is internally bit-based: ``a`` starts at bit 0, ``b`` at
bit 32, and ``c`` at bit 64. The byte helpers render those as offsets 0, 4 and 8,
with a total size of 96 bits (12 bytes) and 32-bit (4-byte) alignment.

The final member lookup intentionally goes through ``MemberCapability``. The
returned ``TypeMember`` references the original ``TypeComponent``, so field
metadata remains available through ``member->extensions()`` without duplication.
This shows the important architectural property: a language-defined type can
expose custom behavior without adding another type-specific method to
``TypeController``.
