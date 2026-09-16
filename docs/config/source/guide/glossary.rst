Glossary
========

AST — Abstract Syntax Tree
   Tree representation of parsed program structure. NovaC AST nodes are generic
   and schema-driven.

Basic block
   Straight-line sequence of IR instructions ending in a terminator such as a
   branch or return.

DSL — Domain-Specific Language
   A language designed for a focused problem domain rather than general-purpose
   programming.

HIR — High-Level Intermediate Representation
   NovaC's first optional IR level after the AST.

IR — Intermediate Representation
   Internal program representation designed for analysis, transformation, or
   translation between source syntax and a later backend.

Lowering
   Translation from one representation to another, typically from a
   higher-level form to a more normalized or lower-level form.

MIR — Medium-Level Intermediate Representation
   NovaC's optional second IR level after HIR.

Parse domain
   Named group of parser rules. Domains let independent grammars such as
   expressions, statements, and programs coexist.

Pratt parser
   Expression parsing technique where prefix/infix/postfix rules carry binding
   power / precedence behavior.

Registry
   Runtime-configured mapping used by NovaC to associate identifiers, tokens,
   node kinds, or operations with behavior.

Runtime handler
   Callback that defines how a particular AST node or operation behaves during
   evaluation/execution.

Schema
   Description of the valid fields and constraints for an AST node kind.

Terminator
   IR operation that ends a basic block, such as return or branch.
