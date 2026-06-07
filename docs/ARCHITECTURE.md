# Architecture

## Stable layer
Do not refactor unless necessary:
- Lexer
- Parser
- Node
- LanguageBuilder
- Feature dependency system

## Extensible high-power layer
- `TypeRegistry`, `TypeUnifier`, `Substitution`
- `TemplateRegistry`, `InstantiationEngine`, `InstantiationCache`
- `TraitRegistry`, `ConstraintSolver`
- `OverloadRegistry`, `OperatorRegistry`
- `MacroRegistry`, `MacroExpansionPass`
- `ASTLoweringPass`, `HIRLoweringPass`
- `BackendRegistry`, `BackendPipeline`
