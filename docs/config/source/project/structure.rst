Repository structure
====================

.. code-block:: text

   NovaC/
   |-- Makefile
   |-- README.md
   |-- docs/
   |   |-- config/
   |   |   |-- conf.py
   |   |   |-- Doxyfile
   |   |   |-- requirements.txt
   |   |   |-- examples/           # real, compile-tested documentation examples
   |   |   `-- source/             # hand-written Sphinx pages
   |   `-- documentation/          # generated HTML
   |-- src/
   |   |-- include/NovaC.hpp       # umbrella header
   |   |-- include/novac/          # public API
   |   `-- core/novac/             # implementation
   `-- tests/
       |-- engine/
       |   |-- controlflow/
       |   `-- source/
       `-- assets/
           |-- atomic/
           |-- essentials/
           |-- memory/
           `-- types/

Public API
----------

``src/include/novac`` is the public API tree. The Engine keeps its generic
subsystems under ``engine/``. In addition to syntax/execution/transformation,
1.4 adds:

``engine/source/``
   Logical source identity, ordered resolver callbacks, preprocessing contexts,
   standard preprocessing registration, and source-origin-preserving fragments.

``engine/controlflow/``
   Reusable helpers for custom statement installation and loop iteration guards.
   Generic runtime signals themselves live with the runtime because they are
   execution-state primitives rather than an asset.

Like Atomic and Essentials, the Types asset keeps its controller at the asset
root and groups supporting concepts in small ``model/``, ``semantics/`` and
``aggregate/`` subdirectories. Generic complex-type behavior lives in
``model/Capabilities.hpp``; physical layout is orchestrated by
``LayoutController``; ``StructType`` is a reference implementation rather than a
``TypeController`` special case.

The Memory asset follows the same compact pattern: ``MemoryController.hpp`` at
the root, a small core model, one raw-access interface, and one
allocation-policy interface. Alias/canonicalization, conversion relations,
validation and operation diagnostics remain inside the compact Types asset.

Doxygen scans the public tree recursively so Breathe can expose documented
symbols in the API section.

Implementation
--------------

``src/core/novac`` mirrors the public hierarchy and contains implementation
sources. Public source/control-flow headers therefore have corresponding
implementation units under ``src/core/novac/engine/source`` and
``src/core/novac/engine/controlflow``.

Documentation source vs output
------------------------------

``docs/config`` is source-controlled documentation input and build
configuration. ``docs/config/examples`` contains executable examples compiled
against the **installed** NovaC package by ``make doc-examples``.
``docs/documentation`` is generated output and can be recreated with
``make doc``.
