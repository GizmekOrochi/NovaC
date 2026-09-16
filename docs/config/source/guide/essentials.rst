Essentials guide
================

Essentials provides reusable **statement-level and function-level** features on
top of an Engine.

Standard imperative core
------------------------

.. code-block:: cpp

   novac::assets::essentials::EssentialsController essentials{engine};
   essentials.installStandardCore();

The standard core installs:

* a program root;
* lexical scoped blocks;
* variable declarations, lookups, and assignments;
* expression statements;
* ``if`` / ``else``;
* ``while``;
* C-style ``for``;
* return statements;
* function declarations and calls;
* program entry-point integration.

Program root and entry point
----------------------------

A complete source file is parsed into the configured program node. During
execution, top-level function declarations are made available for calls and the
configured entry point is invoked when present.

The default entry point is ``main``. It is configurable; see
:doc:`customization`.

Scopes
------

Scoped blocks create nested runtime environments. Variables defined in an inner
scope are not automatically visible after that scope exits, while outer
bindings can still be resolved through parent environments.

Variables
---------

Essentials provides three related behaviors:

* declaration;
* lookup as an expression;
* assignment.

The values themselves still come from the expression system installed in the
Engine, typically through Atomic.

Control flow
------------

``if``/``else``, ``while``, and ``for`` are independent features. Loop options
include a configurable maximum iteration count to protect a language from
accidental infinite loops when desired.

Functions
---------

Functions support:

* parameters;
* local scopes;
* return values;
* recursion;
* native-function dispatch.

The native-function registry is deliberately separate from the language
syntax. NovaC does not impose a standard library.

Individual installation
-----------------------

``installStandardCore()`` is convenient, not mandatory. Smaller DSLs can
install only specific Essentials features or feature packs.
