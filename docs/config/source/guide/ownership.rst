Ownership and lifetime
======================

NovaC is C++, so object lifetime is part of the API contract. This page gives
the practical rules that matter when assembling a language.

Keep the Engine alive
---------------------

Atomic and Essentials configure an existing ``EngineController``. The Engine
should therefore outlive the controllers and runtime work that depend on it.

A straightforward program keeps the objects in one scope and in this order:

.. code-block:: cpp

   novac::controllers::EngineController engine{};
   novac::assets::atomic::AtomicController atomic{engine};
   novac::assets::essentials::EssentialsController essentials{engine};

   // configure and use the language while all three are alive

AST nodes
---------

Parsing returns ``novac::ast::NodePtr``. Keep the returned smart pointer alive
for as long as you need the tree.

.. code-block:: cpp

   const novac::ast::NodePtr program{engine.parse(source)};
   engine.validate(*program);
   const novac::runtime::Value result{engine.eval(*program)};

Native function callbacks
-------------------------

Native functions are stored in the ``FunctionRegistry``. Captured references in
your lambda must remain valid for as long as the function can be called.

Prefer value captures or references to application objects with clearly longer
lifetimes.

Lowering passes
---------------

``ASTLoweringPass`` and ``HIRLoweringPass`` reference an external
``LoweringRegistry``. Keep that registry alive while the pass is used.

General rule
------------

When the API accepts a reference and stores behavior for later use, avoid
creating short-lived configuration objects around a long-lived Engine unless
the relevant API explicitly documents ownership transfer.

The member-level API reference will become more explicit about these contracts
as Doxygen comments are expanded.
