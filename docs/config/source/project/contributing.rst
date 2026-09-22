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
used for validation.

After changing public API behavior, build/install behavior, or a documented
workflow, update the corresponding RST page and keep the executable examples in
sync.

Release checklist
-----------------

Before publishing a release or merging documentation-sensitive build changes,
run:

.. code-block:: console

   $ make test
   $ make test-asan
   $ make test-ubsan
   $ make doc-strict
   $ make release-check

``doc-strict`` compiles/runs the documentation examples and treats Sphinx
warnings as errors. ``release-check`` repeats the normal tests + ASan + UBSan + strict-doc gate
used for local release validation.

For installation changes, also verify a staged install:

.. code-block:: console

   $ rm -rf stage
   $ make install DESTDIR="$PWD/stage" PREFIX=/usr

Then open ``docs/documentation/index.html`` and inspect navigation, code blocks,
search, API pages, and mobile-width layout.
