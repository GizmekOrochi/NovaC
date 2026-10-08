#pragma once

// NovaC intentionally exposes one public umbrella header. Individual novac/...
// headers remain available internally and for advanced builds, but consumers
// can include only <NovaC.hpp> whether they use the default controllers or
// assemble every compiler subsystem themselves.

#include "novac/Version.hpp"

// Engine foundations
#include "novac/engine/foundation/Diagnostic.hpp"
#include "novac/engine/foundation/Ids.hpp"
#include "novac/engine/foundation/Metadata.hpp"
#include "novac/engine/foundation/registry/Registry.hpp"
#include "novac/engine/foundation/registry/RegistryHelpers.hpp"

// Source pipeline
#include "novac/engine/source/Source.hpp"
#include "novac/engine/source/SourceController.hpp"
#include "novac/engine/source/Preprocessor.hpp"
#include "novac/engine/source/PreprocessorController.hpp"
#include "novac/engine/source/StandardPreprocessing.hpp"

// Syntax
#include "novac/engine/syntax/Token.hpp"
#include "novac/engine/syntax/Lexer.hpp"
#include "novac/engine/syntax/Node.hpp"
#include "novac/engine/syntax/Parser.hpp"

// Semantics and transformations
#include "novac/engine/semantics/Symbols.hpp"
#include "novac/engine/transformation/IR.hpp"
#include "novac/engine/transformation/GenericIR.hpp"
#include "novac/engine/controlflow/ControlFlow.hpp"

// Generic compilation infrastructure and optional classic facade
#include "novac/engine/compilation/Compilation.hpp"
#include "novac/engine/backend/Backend.hpp"
#include "novac/engine/EngineController.hpp"
#include "novac/engine/compilation/CompilationController.hpp"

// Optional runtime execution infrastructure
#include "novac/engine/execution/Runtime.hpp"

// Reusable asset foundations
#include "novac/assets/AssetTraits.hpp"

// Type/layout/storage assets
#include "novac/assets/types/model/Type.hpp"
#include "novac/assets/types/model/Capabilities.hpp"
#include "novac/assets/types/model/Storage.hpp"
#include "novac/assets/types/semantics/TypeSemantics.hpp"
#include "novac/assets/types/TypeController.hpp"
#include "novac/assets/types/LayoutController.hpp"
#include "novac/assets/types/StorageController.hpp"
#include "novac/assets/types/aggregate/StructType.hpp"

// Memory assets
#include "novac/assets/memory/MemoryContext.hpp"
#include "novac/assets/memory/model/Memory.hpp"
#include "novac/assets/memory/access/BitAccess.hpp"
#include "novac/assets/memory/allocation/AllocationStrategy.hpp"
#include "novac/assets/memory/behavior/MemoryCapabilities.hpp"
#include "novac/assets/memory/behavior/MemoryOperations.hpp"
#include "novac/assets/memory/MemoryController.hpp"

// Atomic language assets
#include "novac/assets/atomic/AtomicIds.hpp"
#include "novac/assets/atomic/AtomicPattern.hpp"
#include "novac/assets/atomic/LiteralFeature.hpp"
#include "novac/assets/atomic/OperationFeature.hpp"
#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/atomic/literals/LiteralParsing.hpp"
#include "novac/assets/atomic/literals/BooleanLiteralAtomic.hpp"
#include "novac/assets/atomic/literals/FloatLiteralAtomic.hpp"
#include "novac/assets/atomic/literals/IntegerLiteralAtomic.hpp"
#include "novac/assets/atomic/literals/StringLiteralAtomic.hpp"
#include "novac/assets/atomic/operations/ComparisonOperations.hpp"
#include "novac/assets/atomic/operations/IntegerArithmetic.hpp"
#include "novac/assets/atomic/operations/LogicalOperations.hpp"
#include "novac/assets/atomic/operations/NumericOperations.hpp"

// Essentials language assets
#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialFeature.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/helpers/ParsingHelpers.hpp"
#include "novac/assets/essentials/helpers/SchemaHelpers.hpp"
#include "novac/assets/essentials/scopes/ScopedBlocks.hpp"
#include "novac/assets/essentials/variables/ExpressionStatements.hpp"
#include "novac/assets/essentials/variables/Variables.hpp"
#include "novac/assets/essentials/controlflow/ForLoops.hpp"
#include "novac/assets/essentials/controlflow/IfStatements.hpp"
#include "novac/assets/essentials/controlflow/WhileLoops.hpp"
#include "novac/assets/essentials/functions/FunctionRegistry.hpp"
#include "novac/assets/essentials/functions/Functions.hpp"
#include "novac/assets/essentials/functions/FunctionMain.hpp"
#include "novac/assets/essentials/functions/ReturnStatements.hpp"
