Engine guide
============

The Engine is NovaC's language-agnostic kernel. This page explains the role of
each subsystem and how data moves through them.

EngineController
----------------

:cpp:class:`novac::controllers::EngineController` is the main façade. Typical
high-level operations are:

.. code-block:: cpp

   const auto tokens{engine.tokenize(source)};
   const auto tree{engine.parse(source)};
   engine.validate(*tree);
   const auto value{engine.eval(*tree)};

The same controller also exposes registration helpers used by assets and custom
features.

Lexer: text -> tokens
---------------------

The lexer converts source characters into tokens. Language-specific lexical
choices are stored in :cpp:class:`novac::lexer::LexerRegistry` rather than
hardcoded in the lexer.

Examples of configurable lexical concepts include:

* keywords;
* symbols;
* identifier start/continue rules;
* line comments;
* block comments.

Tokens carry their kind, source text, and source location information and are
represented by :cpp:struct:`novac::token::Token`.

Parser: tokens -> AST
---------------------

NovaC uses a registry-driven Pratt parser. A Pratt parser is particularly
useful for expressions because precedence and associativity are attached to
operator parse rules instead of encoded in a large hierarchy of grammar
functions.

NovaC additionally groups grammar rules into named **parse domains**. Typical
asset domains include expression, statement, and program parsing. This keeps
unrelated grammars composable.

Registered parser behavior can include:

* regular parse rules;
* fallback rules;
* prefix rules;
* infix rules;
* postfix rules;
* precedence;
* associativity.

AST: generic tree + schema
--------------------------

:cpp:class:`novac::ast::Node` is a dynamic AST node identified by a node kind
and named fields. Fields can store values, child nodes, or lists of child
nodes.

:cpp:struct:`novac::ast::NodeSchema` and
:cpp:struct:`novac::ast::FieldSchema` describe valid structure. This lets the
Engine validate a language-specific AST while remaining unaware of the
language itself.

Runtime: AST -> behavior
------------------------

Runtime execution is dispatch by AST kind. Features register handlers for the
nodes they own.

The main runtime concepts are:

``Value``
   Generic runtime value representation.

``Environment``
   Name/value bindings with optional parent environments.

``RuntimeContext``
   Active execution state, including environments, node bindings, evaluation,
   and return propagation.

``RuntimeRegistry``
   Maps node kinds and operations to runtime handlers.

The Engine itself does not define variables, functions, loops, or a standard
library. Higher-level features register those semantics.

Diagnostics
-----------

The diagnostics subsystem provides common source locations, spans, severities,
notes, warnings, and errors. Keeping diagnostics independent from a specific
language makes them reusable by custom lexer/parser/features.

Optional lowering
-----------------

The Engine can also lower AST nodes into HIR and HIR into MIR. This path is not
required for normal AST interpretation. See :doc:`ir` before using the lowering
API.


Optional asset controllers
--------------------------

The engine owns only the core language pipeline. Higher-level assets keep their
own controllers and lifecycle instead of becoming permanent ``EngineController``
members. For example:

.. code-block:: cpp

   controllers::EngineController engine;
   atomic::AtomicController atomic{engine};
   types::TypeController types;

``AtomicController`` configures the engine because literals/operators affect the
parser/runtime pipeline. ``TypeController`` remains independent and can be omitted
for dynamic or otherwise untyped languages.
