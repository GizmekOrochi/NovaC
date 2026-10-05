Engine guide
============

The Engine is NovaC's language-agnostic kernel. This page explains the role of
each subsystem and how data moves through them.

EngineController
----------------

:cpp:class:`novac::controllers::EngineController` is the main façade. Typical
direct-text operations are:

.. code-block:: cpp

   const auto tokens{engine.tokenize(source)};
   const auto tree{engine.parse(source)};
   engine.validate(*tree);
   const auto value{engine.eval(*tree)};

Logical sources have an opt-in preprocessing path:

.. code-block:: cpp

   novac::source::Source unit{
       "app:main",
       "main.nova",
       source
   };

   const auto processed{engine.preprocess(unit)};
   const auto tokens{engine.tokenizeSource(unit)};
   const auto tree{engine.parseSource(unit)};

``parse(string)`` does **not** preprocess. This is deliberate compatibility:
projects that already supply complete source strings keep the same behavior.
See :doc:`preprocessing` for source resolvers, includes/imports, conditionals,
and pragma hooks.

The same controller also exposes registration helpers used by assets and custom
features. ``EngineFeature`` installation is transactional: the controller is
updated only after all installers succeed.

Source resolution and preprocessing
-----------------------------------

:cpp:class:`novac::source::SourceController` resolves logical source specifiers
through application-owned callbacks. NovaC does not assume a filesystem.
Resolvers can read packages, IDE buffers, archives, virtual files, or any other
storage.

:cpp:class:`novac::source::PreprocessorController` processes directives before
the normal language lexer. It emits source fragments that retain their original
file/line/column origin. This makes conditional compilation capable of removing
otherwise invalid language text while preserving diagnostics for included
sources.

The standard preprocessing feature is optional. Install it explicitly:

.. code-block:: cpp

   engine.install(novac::source::standardPreprocessing());

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

When ``tokenizeSource`` is used, each preprocessed fragment is tokenized from its
stored :cpp:struct:`novac::diagnostics::SourceLocation`, so tokens from included
sources keep their own origin.

Parser: tokens -> AST
---------------------

NovaC uses a registry-driven Pratt parser. A Pratt parser is particularly useful
for expressions because precedence and associativity are attached to operator
parse rules instead of encoded in a large hierarchy of grammar functions.

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
and named fields. Fields can store values, child nodes, or lists of child nodes.

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
   and generic non-local control signals.

``RuntimeRegistry``
   Maps node kinds and operations to runtime handlers.

:cpp:struct:`novac::runtime::ControlSignal` lets a language propagate decisions
such as return, break-like behavior, continue-like behavior, retry, or a custom
construct through nested statements. Only the construct that owns a signal
should consume it; unrelated signals must keep propagating. Unhandled signals
reaching the runtime root are errors.

The legacy return helpers remain available and use the standard
``novac::runtime::ReturnSignalKind`` signal internally.

The Engine itself does not define variables, functions, loops, or a standard
library. Higher-level features register those semantics. For reusable custom
statement facilities, see :doc:`control_flow_extensions`.

Diagnostics
-----------

The diagnostics subsystem provides common source locations, spans, severities,
notes, warnings, and errors. Keeping diagnostics independent from a specific
language makes them reusable by custom lexer/parser/source features.

Optional lowering
-----------------

The Engine can also lower AST nodes into HIR and HIR into MIR. This path is not
required for normal AST interpretation. See :doc:`ir` before using the lowering
API.

For control-flow lowering, :cpp:class:`novac::ir::HIRBuilder` exposes generic
basic blocks and terminators rather than a fixed branch vocabulary. Language
features choose their operation names and can query
``currentBlockTerminated()`` before composing a nested lowerer.

Optional asset controllers
--------------------------

The Engine owns its core processing infrastructure, including the source and
preprocessing controllers. Higher-level language assets can still keep their
own controllers and lifecycle instead of becoming permanent semantic members of
``EngineController``. For example:

.. code-block:: cpp

   novac::controllers::EngineController engine;
   novac::assets::atomic::AtomicController atomic{engine};
   novac::types::TypeController types;

``AtomicController`` configures the engine because literals/operators affect the
parser/runtime pipeline. ``TypeController`` remains independent and can be
omitted for dynamic or otherwise untyped languages.
