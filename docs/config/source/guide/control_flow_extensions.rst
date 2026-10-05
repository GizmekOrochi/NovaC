Custom control-flow extensions
==============================

NovaC does not require custom control-flow syntax to be added to
``EssentialsController``. A language can define its own statement by composing
normal Engine registrations and installing them through an
:cpp:class:`novac::controllers::EngineFeature`.

The control-flow facilities are intentionally small. They do **not** introduce
another controller, another feature hierarchy, or a fixed branch instruction
set. They provide reusable glue around the Engine APIs that already own syntax,
AST schemas, runtime handlers, and HIR lowering.

The complete tested example is :doc:`../examples/custom_control_flow`.

StatementSpec
-------------

:cpp:struct:`novac::controlflow::StatementSpec` groups the registrations that
usually belong to one statement:

``domain``
   Parser domain in which the statement is recognized.

``trigger``
   Token text used as the parser-rule key.

``triggerRegistration``
   Whether the helper should also register ``trigger`` as a lexer keyword, a
   lexer symbol, or leave lexical registration to another feature.

``schema``
   AST schema owned by the statement.

``parse``
   Parser callback that constructs the AST node.

``runtime``
   Optional runtime statement handler.

``hir``
   Optional AST-to-HIR lowerer.

``novac::controlflow::statement(spec)`` returns an ordinary
``EngineFeature::Installer``. Installation therefore remains transactional: if
one registration fails, the original :cpp:class:`novac::controllers::EngineController`
is not left partially modified.

A minimal custom conditional
----------------------------

The shape of a language-defined ``unless`` feature is:

.. code-block:: cpp

   novac::controllers::EngineFeature unless{"language.unless"};

   unless.onInstall(novac::controlflow::statement({
       .domain = novac::ids::ParseDomain{"statement"},
       .trigger = "unless",
       .triggerRegistration = novac::controlflow::TriggerRegistration::Keyword,
       .schema = {
           "Unless",
           {
               {"condition", novac::ast::FieldKind::Node, true, {}, {"Expression"}},
               {"body", novac::ast::FieldKind::Node, true, {}, {"Statement"}}
           },
           {"Statement"},
           "Language-defined conditional"
       },
       .parse = /* parse unless + condition + body */,
       .runtime = /* execute body when condition is false */,
       .hir = /* create condition/body/exit control-flow blocks */
   }));

   engine.install(unless);

Nothing in this feature depends on Essentials. The host language only needs to
provide whatever expression/body domains the statement chooses to parse.

Trigger registration
--------------------

:cpp:enum:`novac::controlflow::TriggerRegistration` has three modes:

``Keyword``
   Register the trigger through ``EngineController::keyword`` before installing
   the parser rule.

``Symbol``
   Register the trigger through ``EngineController::symbol``.

``Existing``
   Do not modify the lexer. Use this when another feature already owns the
   token or when the parser rule is keyed by a token class such as
   ``$identifier``.

Use ``Existing`` deliberately. It does not verify that another feature actually
registered a matching lexical form.

Runtime control signals
-----------------------

Some control-flow decisions must travel through nested statements before the
construct that owns them can react. :cpp:struct:`novac::runtime::ControlSignal`
provides that mechanism.

Signal identifiers use :cpp:struct:`novac::ids::ControlSignalKind`, so they are
not confused with unrelated operation/node strings:

.. code-block:: cpp

   inline const novac::ids::ControlSignalKind BreakSignal{"my-language.break"};

   context.signal({BreakSignal});

Only one signal can be pending in a ``RuntimeContext``. Raising another signal
while one is pending is an error. A construct must explicitly consume the
signal it owns before raising a replacement, so an unrelated ``return`` or
language-specific decision cannot be overwritten accidentally.

Sequential Essentials constructs stop executing children when **any** signal is
pending. A custom construct that owns a signal must consume only its own signal
and propagate all others. A loop usually follows this pattern:

.. code-block:: cpp

   context.exec(*body);

   if (context.hasSignal(BreakSignal)) {
       context.takeSignal();
       break;
   }

   if (context.hasSignal()) {
       return; // return/continue/another language-specific signal propagates
   }

If a signal reaches the root :cpp:class:`novac::runtime::Runtime` without being
consumed, execution fails with an unhandled-control-signal error. This makes a
forgotten owner visible instead of silently discarding the control decision.

Return compatibility
^^^^^^^^^^^^^^^^^^^^

The existing return API remains available:

.. code-block:: cpp

   context.returnValue(value);
   if (context.hasReturn()) {
       auto value = context.takeReturn();
   }

It is implemented with ``novac::runtime::ReturnSignalKind``. ``takeReturn()``
only consumes a pending return signal; a different custom signal is preserved.

LoopGuard
---------

:cpp:class:`novac::controlflow::LoopGuard` is a small reusable safety primitive
for language-defined loops.

.. code-block:: cpp

   novac::controlflow::LoopGuard guard{100000};

   while (condition()) {
       guard.step();
       context.exec(*body);
       // handle/propagate signals here
   }

Call ``step()`` immediately before each body execution. A maximum of ``0`` means
unlimited execution. The guard counts iterations only; it does not interpret
break, continue, return, or language-specific signals.

HIR lowering
------------

NovaC does not impose operation names such as ``branch`` or ``jump`` on custom
languages. A HIR lowerer chooses its own operations and control-flow convention.
The generic builder only provides blocks, instructions, and terminators.

Typical conditional lowering is:

.. code-block:: cpp

   const auto condition{lowering.lowerHIR(*node.child("condition"), out)};
   const auto body{out.createBlock("unless.body")};
   const auto exit{out.createBlock("unless.exit")};

   out.terminate(
       "branch.false",
       {novac::ir::Operand::fromValue(*condition)},
       {body, exit});

   out.setCurrentBlock(body);
   lowering.lowerHIR(*node.child("body"), out);
   if (!out.currentBlockTerminated()) {
       out.terminate("jump", {}, {exit});
   }

   out.setCurrentBlock(exit);

``currentBlockTerminated()`` is important when composing lowerers: a nested body
may already have emitted a terminating operation, so the outer lowerer must not
blindly add another terminator.

What this API deliberately does not define
------------------------------------------

The facility does not define standard ``if``, ``while``, ``break`` or
``continue`` semantics. Essentials remains the reusable standard implementation
of those constructs. It also does not define a universal HIR control-flow
instruction set. A language/backend can choose the operations it needs while
still using NovaC's generic block structure.
