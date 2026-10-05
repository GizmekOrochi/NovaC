Engine API
==========

This is the member-level reference for NovaC's language-agnostic Engine. For a
conceptual introduction, read :doc:`../guide/engine`. For HIR/MIR concepts,
read :doc:`../guide/ir` before the IR classes below.

Controller
----------

.. doxygenclass:: novac::controllers::EngineController
   :members:

Lexer and tokens
----------------

.. doxygenclass:: novac::lexer::LexerRegistry
   :members:

.. doxygenclass:: novac::lexer::Lexer
   :members:

.. doxygenstruct:: novac::token::Token
   :members:

Parser
------

.. doxygenclass:: novac::parser::ParserRegistry
   :members:

.. doxygenclass:: novac::parser::ParserContext
   :members:

.. doxygenclass:: novac::parser::Parser
   :members:

AST
---

.. doxygenclass:: novac::ast::Node
   :members:

.. doxygenclass:: novac::ast::NodeRegistry
   :members:

.. doxygenstruct:: novac::ast::FieldSchema
   :members:

.. doxygenstruct:: novac::ast::NodeSchema
   :members:

Runtime
-------

.. doxygenclass:: novac::runtime::Value
   :members:

.. doxygenclass:: novac::runtime::Environment
   :members:

.. doxygenclass:: novac::runtime::RuntimeContext
   :members:

.. doxygenclass:: novac::runtime::RuntimeRegistry
   :members:

.. doxygenclass:: novac::runtime::Runtime
   :members:

Diagnostics
-----------

.. doxygenclass:: novac::diagnostics::DiagnosticEngine
   :members:

.. doxygenstruct:: novac::diagnostics::Diagnostic
   :members:

Intermediate representations
----------------------------

.. doxygenstruct:: novac::ir::ValueId
   :members:

.. doxygenstruct:: novac::ir::BlockId
   :members:

.. doxygenclass:: novac::ir::Literal
   :members:

.. doxygenclass:: novac::ir::Operand
   :members:

.. doxygenstruct:: novac::ir::Instruction
   :members:

.. doxygenstruct:: novac::ir::Terminator
   :members:

.. doxygenstruct:: novac::ir::BasicBlock
   :members:

.. doxygenstruct:: novac::ir::HIRModule
   :members:

.. doxygenstruct:: novac::ir::MIRModule
   :members:

.. doxygenclass:: novac::ir::HIRBuilder
   :members:

.. doxygenclass:: novac::ir::MIRBuilder
   :members:

.. doxygenclass:: novac::ir::MIRLoweringContext
   :members:

.. doxygenclass:: novac::ir::LoweringRegistry
   :members:

.. doxygenclass:: novac::ir::ASTLoweringPass
   :members:

.. doxygenclass:: novac::ir::HIRLoweringPass
   :members:

Control-flow extension facilities
---------------------------------

.. doxygenenum:: novac::controlflow::TriggerRegistration

.. doxygenstruct:: novac::controlflow::StatementSpec
   :members:

.. doxygenclass:: novac::controlflow::LoopGuard
   :members:

.. doxygenstruct:: novac::ids::ControlSignalKind
   :members:

.. doxygenstruct:: novac::runtime::ControlSignal
   :members:

Source and preprocessing
------------------------

.. doxygenstruct:: novac::source::Source
   :members:

.. doxygenstruct:: novac::source::SourceRequest
   :members:

.. doxygenstruct:: novac::source::SourceFragment
   :members:

.. doxygenstruct:: novac::source::PreprocessedSource
   :members:

.. doxygenstruct:: novac::source::PreprocessOptions
   :members:

.. doxygenstruct:: novac::source::Directive
   :members:

.. doxygenenum:: novac::source::DirectiveMode

.. doxygenclass:: novac::source::SourceController
   :members:

.. doxygenclass:: novac::source::PreprocessorContext
   :members:

.. doxygenclass:: novac::source::PreprocessorController
   :members:

.. doxygenfunction:: novac::source::standardPreprocessing
