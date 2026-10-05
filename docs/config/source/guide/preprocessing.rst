Source loading and preprocessing
================================

NovaC has two source-entry paths:

.. code-block:: text

   direct text                         logical Source
       |                                  |
       | engine.parse(text)               | engine.parseSource(...)
       v                                  v
     Lexer                         PreprocessorController
       |                                  |
       |                                  v
       |                          source-origin fragments
       |                                  |
       +--------------------------->     Lexer
                                          |
                                          v
                                        Parser

The direct ``parse(string)`` path is intentionally unchanged. Preprocessing is
opt-in through ``preprocess()``, ``tokenizeSource()`` or ``parseSource()``.

The complete tested example is :doc:`../examples/preprocessing`.

Source identity
---------------

:cpp:struct:`novac::source::Source` separates three concepts:

``id``
   Canonical identity used for cycle detection and import de-duplication. It is
   required and does not need to be a filesystem path.

``name``
   Human-readable name placed in diagnostics and token source locations. If a
   resolver returns an empty name, ``SourceController`` substitutes ``id``.

``text``
   Source contents.

For example, an application may resolve ``std:math`` to:

.. code-block:: cpp

   {
       .id = "package:stdlib/math",
       .name = "stdlib/math.nova",
       .text = "..."
   }

SourceController
----------------

:cpp:class:`novac::source::SourceController` owns an ordered chain of
application-defined resolvers. NovaC deliberately performs no built-in
filesystem I/O.

.. code-block:: cpp

   engine.sources().resolver(
       "workspace",
       [](const novac::source::SourceRequest &request)
           -> std::optional<novac::source::Source> {
           // Return a Source when this resolver recognizes request.specifier.
           return std::nullopt;
       });

Resolvers run in registration order until one returns a ``Source``. Returning
``std::nullopt`` means "not handled". Returning a source with an empty canonical
``id`` is an error.

``SourceRequest::specifier`` is the text supplied to ``#include``, ``#import``
or ``parseSource(specifier)``. ``SourceRequest::importer`` is the canonical
``Source::id`` of the source making an include/import request; it is empty when
``parseSource(specifier)`` resolves the root. Applications can use this to
implement relative paths or package-local resolution.

Why preprocessing runs before the language lexer
-------------------------------------------------

Conditional compilation must be able to remove text that is invalid for the
language lexer:

.. code-block:: text

   #if WINDOWS_ONLY
   @@@ text that this language cannot tokenize @@@
   #endif

When ``WINDOWS_ONLY`` is not defined, the invalid text is filtered before normal
lexing. This is why NovaC's preprocessor is a source phase rather than a token
rewrite pass.

Source locations are still preserved. The preprocessor emits
:cpp:struct:`novac::source::SourceFragment` objects, each carrying the original
``SourceLocation`` of its text. ``tokenizeSource()`` lexes each fragment from
that origin and produces one final End token located at the end of the **root**
source, even when the last visible text came from an included file.

Standard preprocessing feature
------------------------------

Install :cpp:func:`novac::source::standardPreprocessing` through the same
``EngineFeature`` mechanism used by the rest of NovaC:

.. code-block:: cpp

   engine.install(novac::source::standardPreprocessing());

It installs:

``#define NAME``
   Define a symbol for the current preprocessing session.

``#undef NAME``
   Remove a symbol.

``#include "specifier"`` / ``#include <specifier>``
   Resolve and preprocess the referenced source every time the directive is
   encountered. Standard include specifiers must be quoted or bracketed; bare
   text such as ``#include package name`` is rejected.

``#import "specifier"`` / ``#import <specifier>``
   Resolve the referenced source, then preprocess each canonical ``Source::id``
   at most once for ``#import`` requests in the current session. Standard import
   specifiers follow the same quoted/bracketed rule. ``#include`` is still
   always processed and is not suppressed by a previous import.

``#if NAME`` / ``#ifdef NAME``
   Keep the branch when ``NAME`` is currently defined. These forms
   are symbol-presence tests; ``#if`` does **not** evaluate a C-like expression.

``#ifndef NAME``
   Keep the branch when ``NAME`` is not defined.

``#else`` / ``#endif``
   Select the alternate branch and close the current conditional. These
   structural directives do not accept arguments; trailing non-comment text is
   rejected.

There is currently no standard ``#elif`` and no macro text substitution.
``#define`` stores symbol presence only.

Defines and nested sources
--------------------------

A preprocessing session shares its symbol set across the root and all included
or imported sources. An included configuration source can therefore define a
symbol used later by its importer:

.. code-block:: text

   #include "config"
   #if FEATURE_X
   feature_x_source
   #endif

The session also shares import de-duplication state and diagnostics. Conditional
nesting, however, must be balanced inside each individual source: an included
file cannot close a conditional that was opened by its parent.

PreprocessOptions
-----------------

:cpp:struct:`novac::source::PreprocessOptions` controls a processing run.

``defines``
   Symbols present before processing the root source.

``directivePrefix``
   Directive introducer. The default is ``#``. A directive is recognized only
   before ordinary code on its logical line; leading whitespace and comments
   are allowed.

``maxIncludeDepth``
   Maximum recursive include/import levels **below the root**. The default is
   ``128`` and ``0`` disables the limit. For example, ``1`` permits
   ``root -> child`` but rejects ``root -> child -> grandchild``.

The directive scanner uses the configured lexer line/block comment delimiters,
so text resembling a directive inside comments is ignored. A line comment ends
the directive. A block comment inside directive arguments behaves like lexical
whitespace, so text after a same-line closing ``*/`` remains part of the
directive instead of being silently discarded. Multi-line block comments still
suppress nested directive-looking text until the comment closes.

Custom directives
-----------------

A language can register a directive directly or as part of an
``EngineFeature``:

.. code-block:: cpp

   feature.onInstall([](novac::controllers::EngineController &engine) {
       engine.directive(
           "warning",
           [](const novac::source::Directive &directive,
              novac::source::PreprocessorContext &context) {
               context.diagnostics().warning(directive.arguments, directive.span);
           });
   });

Handlers registered with the default ``DirectiveMode::ActiveOnly`` run only in
an active conditional branch. Structural directives that must execute while an
outer branch is inactive can use ``DirectiveMode::Always``. Use that mode only
when the handler correctly maintains conditional structure.

The context can:

* query/modify defines;
* push, alternate, and pop conditional state;
* include/import another source;
* emit generated text at an explicit source origin;
* report diagnostics;
* inspect the current source.

Pragma-like hooks
-----------------

``#pragma`` is reserved for named pragma dispatch rather than ordinary
``engine.directive("pragma", ...)`` registration.

.. code-block:: cpp

   engine.pragma(
       "dialect",
       [](const novac::source::Directive &directive,
          novac::source::PreprocessorContext &) {
           // Handles: #pragma dialect strict
           // directive.name      == "pragma"
           // directive.arguments == "strict"
       });

The pragma name itself is removed before the callback, and ``argumentsSpan`` is
adjusted to cover the dispatched arguments. Unknown pragmas are errors while the
branch is active.

Include cycles and errors
-------------------------

The active source stack is tracked by canonical ``Source::id``. Recursive
include/import cycles are rejected and the error reports the canonical chain.
The depth limit is a separate guard against very deep acyclic graphs.

Unknown active directives, unmatched/duplicate conditional markers, malformed
standard directive arguments, unresolved sources, unterminated preprocessing
block comments, and unknown active pragmas are reported as preprocessing
errors.

Choosing the right entry point
------------------------------

Use ``parse(text)`` when you already have one complete source string and do not
want preprocessing.

Use ``tokenizeSource(Source)`` when you want the source/preprocessor layer but
need to inspect or transform tokens yourself.

Use ``parseSource(Source)`` when you already own the root source object.

Use ``parseSource(specifier)`` when the root should also be resolved through
``SourceController``.
