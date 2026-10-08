Testing NovaC and your language
===============================

NovaC includes an in-repository lightweight test framework under
``tests/tester.hpp``. The repository suite covers Engine, Atomic, Essentials,
and core reliability behavior.

Run NovaC's suite
-----------------

.. code-block:: console

   $ make test

The normal suite links against the same ``lib/libNovaC.a`` produced by the
default build, so it also validates the static-library distribution boundary.

Sanitizers
----------

Use the dedicated sanitizer targets for lifetime/memory errors and undefined
behavior:

.. code-block:: console

   $ make test-asan
   $ make test-ubsan

``test-asan`` rebuilds NovaC and the tests with AddressSanitizer and enables
leak detection. ``test-ubsan`` rebuilds them with UndefinedBehaviorSanitizer
and stops on the first reported undefined behavior.

Keeping the builds separate is intentional: a normal test run, an ASan run,
and a UBSan run each have their own object tree and executable.

Test your language at multiple levels
-------------------------------------

A useful language test strategy has several layers.

Lexer tests
   Verify token kinds, source text, and custom symbols/keywords.

Parser/AST tests
   Verify node kinds, field structure, precedence, malformed syntax,
   incomplete expressions, and fallback behavior.

Runtime tests
   Verify values, scopes, functions, control flow, diagnostics, and errors.

End-to-end tests
   Feed complete source programs through parse -> validate -> eval and check the
   final result/output.

Documentation examples
----------------------

The main examples in this documentation live as real C++ files in
``docs/config/examples`` rather than duplicated snippets. Run them with:

.. code-block:: console

   $ make doc-examples

``make doc-strict`` also requires those examples to compile and execute before
Sphinx is allowed to complete. This prevents documentation examples from
silently drifting away from the public API.

Release validation
------------------

The minimum repository release gate is:

.. code-block:: console

   $ make release-check

This combines the normal test suite, ASan, UBSan, and strict documentation
validation. GitHub CI additionally exercises GCC, Clang, installation, and the
documentation jobs on a clean runner.
