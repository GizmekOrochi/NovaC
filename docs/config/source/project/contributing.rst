Contributing and documentation style
====================================

NovaC documentation has two sources of truth with different responsibilities.

Public C++ comments
-------------------

Doxygen comments in ``src/include`` should document facts tied directly to an
API symbol:

* purpose;
* parameter semantics;
* return semantics;
* ownership/lifetime expectations;
* preconditions;
* exceptions;
* non-obvious side effects.

Example:

.. code-block:: cpp

   /**
    * @brief Registers a native function by name.
    *
    * @param name Language-level function name.
    * @param function Native implementation to invoke.
    * @throws std::runtime_error If the name is invalid or duplicated.
    */
   void native(const std::string &name, NativeFunction function);

Sphinx guides
-------------

Use RST pages for information that spans multiple API symbols:

* mental models;
* architecture;
* tutorials;
* why a subsystem exists;
* workflows;
* examples;
* design guidance.

Do not duplicate the full API manually in RST. Breathe already imports Doxygen
output into the API reference.

Examples must be real programs
------------------------------

Important examples live in ``docs/config/examples`` and are included with
``literalinclude``. This avoids one copy in the documentation and another copy
used for testing.

Release checklist
-----------------

Before publishing documentation changes:

.. code-block:: console

   $ make test
   $ make doc-strict

Then open ``docs/documentation/index.html`` and inspect navigation, code blocks,
search, API pages, and mobile-width layout.
