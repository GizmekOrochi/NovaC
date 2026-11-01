#pragma once

// NovaC intentionally exposes one public umbrella header. Individual novac/...
// headers remain available internally and for advanced builds. <NovaC.hpp> intentionally exposes only
// the supported language engine.

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

// Semantics and language control flow
#include "novac/engine/semantics/Symbols.hpp"
#include "novac/engine/controlflow/ControlFlow.hpp"

// Generic HIR/MIR/IR primitives can be used by interpreters and analyzers.
#include "novac/engine/transformation/TypeIdentity.hpp"
#include "novac/engine/transformation/IR.hpp"
#include "novac/engine/transformation/GenericIR.hpp"

// Core engine: language-agnostic pipeline primitives.
#include "novac/engine/EngineController.hpp"

// Optional runtime execution infrastructure
#include "novac/engine/execution/Runtime.hpp"
