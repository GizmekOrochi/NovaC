Testing NovaC and your language
===============================

NovaC includes an in-repository lightweight test framework under
``tests/tester.hpp``. The repository suite covers Engine, Atomic, and
Essentials behavior.

Run NovaC's suite
-----------------

.. code-block:: console

   $ make test

Test your language at multiple levels
-------------------------------------

A useful language test strategy has several layers.

Lexer tests
   Verify token kinds, source text, and custom symbols/keywords.

Parser/AST tests
   Verify node kinds, field structure, precedence, and invalid syntax.

Runtime tests
   Verify values, scopes, functions, control flow, and errors.

End-to-end tests
   Feed complete source programs through parse -> validate -> eval and check the
   final result/output.

Documentation examples
----------------------

The main examples in this documentation live as real C++ files in
``docs/config/examples`` rather than duplicated snippets. They are compiled
while maintaining the documentation so examples cannot silently drift away
from the public API.

Sanitizers
----------

For runtime-heavy changes, build tests with AddressSanitizer and
UndefinedBehaviorSanitizer:

.. code-block:: console

   $ g++ -std=c++20 \
       -fsanitize=address,undefined \
       -fno-omit-frame-pointer \
       -Isrc -Isrc/include -Itests \
       ...

Unit tests tell you whether expected behavior is correct. Sanitizers can also
find lifetime bugs and undefined behavior that still happen to produce the
expected result.
