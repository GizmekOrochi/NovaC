# NovaC Language Author Guide

> Guide pratique pour créer un langage avec NovaC sans lire le code interne du moteur.

NovaC n'est pas un langage. NovaC est un framework pour construire des langages.

Le moteur ne connaît pas `int`, `return`, `if`, `while`, `struct`, `template`, `trait`, `import`, ni aucun mot-clé concret. Tout est ajouté par des **features**.

---

## 1. Le modèle mental

Un langage NovaC est une composition de features.

```cpp
Language lang = LanguageBuilder{}
    .use<CoreSyntaxFeature>()
    .use<IntFeature>()
    .use<FunctionFeature>()
    .use<ReturnFeature>()
    .use<BinaryOperatorFeature>("+", 20)
    .build();
```

Le programme suivant n'est valide que parce que les features correspondantes ont été installées :

```c
int main() {
    return 40 + 2;
}
```

Sans `IntFeature`, `int` n'existe pas.
Sans `ReturnFeature`, `return` n'existe pas.
Sans `BinaryOperatorFeature`, `+` n'existe pas.

---

## 2. Pipeline général

```txt
Source code
   ↓
LexerEngine
   ↓
Tokens
   ↓
ParserEngine
   ↓
AST Node tree
   ↓
ValidationRegistry
   ↓
ResolveRegistry
   ↓
TypeCheckRegistry
   ↓
LoweringRegistry
   ↓
HIR
   ↓
MIR
   ↓
Backend / Runtime
```

Chaque étape est extensible par features.

---

## 3. Règle d'or

Une feature doit enregistrer son comportement dans des registries.

Elle ne doit jamais modifier :

```txt
LexerEngine
ParserEngine
RuntimeEngine
LoweringEngine
TypeEngine
```

À éviter absolument :

```cpp
if (node.kind() == "return") { ... }
if (token.text() == "if") { ... }
```

À faire :

```cpp
runtime.statement("return", handler);
typeCheck.node("return", handler);
lowering.hir("return", handler);
```

---

## 4. Architecture des registries

Une feature peut utiliser :

```txt
LexerRegistry        -> mots-clés, symboles, tokens custom
GrammarRegistry      -> metadata pour docs, LSP, formatter
NodeRegistry         -> schemas AST
ParserRegistry       -> parsing exécutable
TypeRegistry         -> types
OperatorRegistry     -> opérateurs et overloads
ResolveRegistry      -> résolution de symboles
TypeCheckRegistry    -> vérification de types
ValidationRegistry   -> validation AST
RuntimeRegistry      -> exécution
LoweringRegistry     -> AST -> HIR -> MIR
ModuleRegistry       -> imports/exports/modules
MacroRegistry        -> macros
BackendRegistry      -> backends
```

---

## 5. Créer un langage minimal

```cpp
#include "novac/language/LanguageBuilder.hpp"
#include "novac/features/CoreSyntaxFeature.hpp"
#include "novac/features/IntFeature.hpp"
#include "novac/features/FunctionFeature.hpp"
#include "novac/features/ReturnFeature.hpp"
#include "novac/features/BinaryOperatorFeature.hpp"

using namespace novac;

int main() {
    auto lang = LanguageBuilder{}
        .use<CoreSyntaxFeature>()
        .use<IntFeature>()
        .use<FunctionFeature>()
        .use<ReturnFeature>()
        .use<BinaryOperatorFeature>("+", 20)
        .build();
}
```

Ce langage supporte :

```c
int main() {
    return 40 + 2;
}
```

---

## 6. Créer une feature

Toutes les features héritent de `LanguageFeature`.

```cpp
class MyFeature : public LanguageFeature {
public:
    FeatureInfo info() const override {
        return {
            .name = "my.feature",
            .version = "1.0.0",
            .requires = {"core.syntax"},
            .capabilities = {"my-capability"}
        };
    }
};
```

Une feature peut implémenter ces méthodes :

```cpp
void registerTokens(LexerRegistry&) const override;
void registerGrammar(GrammarRegistry&) const override;
void registerNodes(NodeRegistry&) const override;
void registerParsing(ParserRegistry&) const override;
void registerTypes(TypeRegistry&) const override;
void registerOperators(OperatorRegistry&, TypeRegistry&) const override;
void registerResolve(ResolveRegistry&) const override;
void registerTypeCheck(TypeCheckRegistry&) const override;
void registerValidation(ValidationRegistry&) const override;
void registerRuntime(RuntimeRegistry&) const override;
void registerLowering(LoweringRegistry&) const override;
```

---

## 7. Ajouter des tokens

Exemple pour une feature `return` :

```cpp
void registerTokens(LexerRegistry& lexer) const override {
    lexer.keyword("return");
    lexer.symbol(";");
}
```

Exemple pour un opérateur :

```cpp
lexer.symbol("+");
lexer.symbol("==");
lexer.symbol("<=");
```

Le lexer utilise le longest-match, donc `==` est reconnu avant `=`.

---

## 8. Ajouter un node AST

Les nodes NovaC sont génériques.

```cpp
auto node = Node::make("return");
node->field("value", expr);
```

Déclare aussi le schema du node :

```cpp
void registerNodes(NodeRegistry& nodes) const override {
    nodes.registerType({
        .kind = "return",
        .fields = {
            {"value", FieldKind::Node, true}
        },
        .documentation = "Return statement"
    });
}
```

Le schema sert à :

```txt
validation AST
documentation
formatter
LSP
introspection
debugger
```

---

## 9. Ajouter une expression

Exemple : integer literal.

```cpp
void registerParsing(ParserRegistry& parser) const override {
    parser.prefix("$int", [](ParserContext& ctx) {
        auto token = ctx.consume(TokenKind::Integer, "expected integer");
        int value = std::stoi(token.text());

        auto node = Node::make("int.literal");
        node->field("value", value);
        return node;
    });
}
```

Puis le runtime :

```cpp
void registerRuntime(RuntimeRegistry& runtime) const override {
    runtime.expression("int.literal", [](RuntimeContext&, const Node& node) {
        return Value::integer(std::get<int>(node.get("value")));
    });
}
```

Puis le type checking :

```cpp
void registerTypeCheck(TypeCheckRegistry& types) const override {
    types.node("int.literal", [](AnalysisContext& ctx, const Node& node) {
        ctx.setNodeType(node, ctx.language().types().find("int"));
    });
}
```

---

## 10. Ajouter un statement

Exemple : `return expr;`

```cpp
void registerParsing(ParserRegistry& parser) const override {
    parser.statement("return", [](ParserContext& ctx) {
        ctx.consumeText("return", "expected return");
        auto value = ctx.parseExpression();
        ctx.consumeText(";", "expected ';' after return");

        auto node = Node::make("return");
        node->field("value", value);
        return node;
    });
}
```

Runtime :

```cpp
void registerRuntime(RuntimeRegistry& runtime) const override {
    runtime.statement("return", [](Language& lang, RuntimeContext& ctx, const Node& node) {
        auto valueNode = std::get<NodePtr>(node.get("value"));
        auto value = lang.runtime().evaluate(lang, ctx, *valueNode);
        ctx.returnValue(value);
    });
}
```

Type check :

```cpp
void registerTypeCheck(TypeCheckRegistry& types) const override {
    types.node("return", [](AnalysisContext& ctx, const Node& node) {
        auto value = std::get<NodePtr>(node.get("value"));
        auto actual = ctx.nodeType(*value);
        auto expected = ctx.currentReturn();

        if (!ctx.language().types().isAssignable(expected, actual)) {
            throw DiagnosticException("return type mismatch");
        }
    });
}
```

---

## 11. Ajouter un opérateur

Exemple : `+` pour les entiers.

```cpp
class BinaryOperatorFeature : public LanguageFeature {
public:
    BinaryOperatorFeature(std::string symbol, int precedence)
        : symbol_(std::move(symbol)), precedence_(precedence) {}

    void registerTokens(LexerRegistry& lexer) const override {
        lexer.symbol(symbol_);
    }

    void registerParsing(ParserRegistry& parser) const override {
        auto symbol = symbol_;
        auto precedence = precedence_;

        parser.infix(symbol, precedence, Associativity::Left,
            [symbol, precedence](ParserContext& ctx, NodePtr left) {
                ctx.consumeText(symbol, "expected operator");
                auto right = ctx.parseExpression(precedence + 1);

                auto node = Node::make("binary");
                node->field("op", symbol);
                node->field("left", left);
                node->field("right", right);
                return node;
            });
    }

private:
    std::string symbol_;
    int precedence_;
};
```

Enregistrer l'overload :

```cpp
void registerOperators(OperatorRegistry& ops, TypeRegistry& types) const override {
    auto intType = types.primitive("int");

    ops.registerOperator({
        .symbol = "+",
        .parameters = {intType, intType},
        .result = intType,
        .runtime = [](const std::vector<Value>& args) {
            return Value::integer(args[0].asInt() + args[1].asInt());
        }
    });
}
```

---

## 12. Ajouter une déclaration

Exemple : fonction minimale.

```cpp
int main() {
    return 42;
}
```

Parser :

```cpp
parser.declaration([](ParserContext& ctx) -> NodePtr {
    auto typeName = ctx.current().text();

    if (!ctx.language().types().find(typeName)) {
        return nullptr;
    }

    ctx.advance();
    auto name = ctx.consume(TokenKind::Identifier, "expected function name").text();

    ctx.consumeText("(", "expected '('");
    ctx.consumeText(")", "expected ')'");

    auto body = ctx.parseBlock();

    auto node = Node::make("function");
    node->field("returnType", typeName);
    node->field("name", name);
    node->field("body", body);
    return node;
});
```

---

## 13. Ajouter un type

Primitive :

```cpp
void registerTypes(TypeRegistry& types) const override {
    types.primitive("int");
}
```

Array :

```cpp
auto intType = types.find("int");
auto arrayType = types.arrayOf(intType);
```

Function type :

```cpp
auto fnType = types.function({intType, intType}, intType);
```

Generic type :

```cpp
auto T = types.typeVariable("T");
auto boxT = types.generic("Box", {T});
```

---

## 14. Ajouter une variable

Syntaxe possible :

```c
let x: int = 42;
```

Parsing :

```cpp
parser.statement("let", [](ParserContext& ctx) {
    ctx.consumeText("let", "expected let");
    auto name = ctx.consume(TokenKind::Identifier, "expected variable name").text();
    ctx.consumeText(":", "expected ':'");
    auto type = ctx.parseType();
    ctx.consumeText("=", "expected '='");
    auto init = ctx.parseExpression();
    ctx.consumeText(";", "expected ';'");

    return Node::make("var.decl")
        ->field("name", name)
        .field("type", type)
        .field("init", init);
});
```

Runtime :

```cpp
runtime.statement("var.decl", [](Language& lang, RuntimeContext& ctx, const Node& node) {
    auto init = std::get<NodePtr>(node.get("init"));
    auto value = lang.runtime().evaluate(lang, ctx, *init);
    ctx.environment().define(std::get<std::string>(node.get("name")), value);
});
```

---

## 15. Ajouter `if`

AST attendu :

```txt
Node("if")
  condition: Expression
  then: Statement
  else: Optional Statement
```

Parsing :

```cpp
parser.statement("if", [](ParserContext& ctx) {
    ctx.consumeText("if", "expected if");
    ctx.consumeText("(", "expected '('");
    auto condition = ctx.parseExpression();
    ctx.consumeText(")", "expected ')'");
    auto thenBranch = ctx.parseStatement();

    NodePtr elseBranch = nullptr;
    if (ctx.matchText("else")) {
        elseBranch = ctx.parseStatement();
    }

    auto node = Node::make("if");
    node->field("condition", condition);
    node->field("then", thenBranch);
    if (elseBranch) node->field("else", elseBranch);
    return node;
});
```

Runtime :

```cpp
runtime.statement("if", [](Language& lang, RuntimeContext& ctx, const Node& node) {
    auto cond = std::get<NodePtr>(node.get("condition"));
    auto result = lang.runtime().evaluate(lang, ctx, *cond);

    if (result.isTruthy()) {
        auto thenBranch = std::get<NodePtr>(node.get("then"));
        lang.runtime().execute(lang, ctx, *thenBranch);
    } else if (node.has("else")) {
        auto elseBranch = std::get<NodePtr>(node.get("else"));
        lang.runtime().execute(lang, ctx, *elseBranch);
    }
});
```

---

## 16. Ajouter du lowering

Le lowering doit être décentralisé.

```cpp
void registerLowering(LoweringRegistry& lowering) const override {
    lowering.hir("return", [](const Node& node, HIRBuilder& out) {
        out.emit("HIR_RETURN");
    });
}
```

Pour un opérateur :

```cpp
lowering.hir("binary", [](const Node& node, HIRBuilder& out) {
    auto op = std::get<std::string>(node.get("op"));
    out.emit("HIR_BINARY_" + op);
});
```

---

## 17. Ajouter un module

Syntaxe :

```c
import math;
```

Feature :

```cpp
class ImportFeature : public LanguageFeature {
public:
    void registerTokens(LexerRegistry& lexer) const override {
        lexer.keyword("import");
        lexer.symbol(".");
        lexer.symbol(";");
    }
};
```

Module system à utiliser :

```cpp
modules.load("math");
modules.exportSymbol("sqrt");
modules.importSymbol("math", "sqrt");
```

À gérer :

```txt
module cache
circular imports
visibility
namespaces
package paths
```

---

## 18. Ajouter un trait

Syntaxe cible :

```c
trait Addable<T> {
    fn add(self, other: T) -> T;
}
```

Un trait doit déclarer :

```txt
methods
associated types
default implementations
where clauses
```

Feature :

```cpp
traitRegistry.registerTrait({
    .name = "Addable",
    .methods = {...},
    .associatedTypes = {...}
});
```

Implémentation :

```c
impl Addable<int> for int {
    fn add(self, other: int) -> int {
        return self + other;
    }
}
```

Le checker doit vérifier :

```txt
missing methods
wrong signatures
associated type mismatch
where clause validity
```

---

## 19. Ajouter un template

Syntaxe cible :

```c
template<T>
T identity(T x) {
    return x;
}
```

Pipeline :

```txt
Generic declaration
   ↓
Type substitution
   ↓
AST deep clone
   ↓
Specialization name generation
   ↓
Instantiation cache
```

Exemple :

```txt
identity<int>
identity<string>
```

Deviennent :

```txt
identity__int
identity__string
```

---

## 20. Ajouter une macro

Macro registry :

```cpp
macros.registerMacro("derive", [](MacroContext& ctx, const Node& input) {
    ...
});
```

À respecter :

```txt
hygienic identifiers
source mapping
span preservation
recursion limit
macro diagnostics
```

---

## 21. Ajouter un backend

Backend minimal :

```cpp
class MyBackend : public Backend {
public:
    std::string name() const override {
        return "my-backend";
    }

    void emit(const MIRModule& mir) override {
        ...
    }
};
```

Enregistrement :

```cpp
backends.add("my-backend", [] {
    return std::make_unique<MyBackend>();
});
```

Pipeline :

```txt
AST -> HIR -> MIR -> Backend
```

---

## 22. Feature dependencies

Une feature peut exiger d'autres features :

```cpp
FeatureInfo info() const override {
    return {
        .name = "stmt.return",
        .version = "1.0.0",
        .requires = {"decl.function"}
    };
}
```

Ou exiger des capabilities :

```cpp
.requiresCapabilities = {"type-system"}
```

Le builder doit valider :

```txt
duplicate features
missing dependencies
conflicts
cycles
version compatibility
missing capabilities
```

---

## 23. Bonnes pratiques

Toujours :

```txt
Créer une feature
Déclarer les tokens
Déclarer les nodes
Déclarer le parsing
Déclarer le type checking
Déclarer le runtime
Déclarer le lowering
```

Jamais :

```txt
Modifier ParserEngine pour ajouter if/while/return
Modifier RuntimeEngine pour ajouter un node
Mettre un gros switch sur node.kind()
Hardcoder int/string/bool dans le core
```

---

## 24. Exemple complet : mini C-like language

```cpp
Language lang = LanguageBuilder{}
    .use<CoreSyntaxFeature>()
    .use<IntFeature>()
    .use<FunctionFeature>()
    .use<ReturnFeature>()
    .use<VariableFeature>()
    .use<IfFeature>()
    .use<WhileFeature>()
    .use<BinaryOperatorFeature>("+", 20)
    .use<BinaryOperatorFeature>("<", 10)
    .build();
```

Programme :

```c
int main() {
    int x = 0;
    while (x < 10) {
        x = x + 1;
    }
    return x;
}
```

---

## 25. Exemple complet : calculator language

```cpp
Language lang = LanguageBuilder{}
    .use<CoreSyntaxFeature>()
    .use<IntFeature>()
    .use<FloatFeature>()
    .use<BinaryOperatorFeature>("+", 20)
    .use<BinaryOperatorFeature>("-", 20)
    .use<BinaryOperatorFeature>("*", 30)
    .use<BinaryOperatorFeature>("/", 30)
    .build();
```

---

## 26. Checklist pour créer une feature propre

```txt
[ ] Nom unique
[ ] Version
[ ] Dependencies
[ ] Capabilities
[ ] Tokens
[ ] Grammar metadata
[ ] Node schemas
[ ] Parsing
[ ] ResolveSymbols
[ ] TypeCheck
[ ] Runtime
[ ] Lowering HIR
[ ] Lowering MIR
[ ] Diagnostics
[ ] Tests
[ ] Example
[ ] Docs
```

---

## 27. Quand modifier le core ?

Presque jamais.

Tu modifies le core uniquement si tu ajoutes un nouveau mécanisme général, par exemple :

```txt
nouveau type de registry
nouveau backend abstrait
nouvelle forme de diagnostic
nouveau système de cache global
```

Tu ne modifies pas le core pour ajouter :

```txt
if
while
return
struct
class
operator +
array
string
template
```

Tout ça est une feature.

---

## 28. Résumé

Pour créer un langage avec NovaC :

```txt
1. Choisis les features
2. Compose-les avec LanguageBuilder
3. Écris tes propres features si nécessaire
4. Ne touche pas au core
5. Ajoute tests + exemples + docs
```

NovaC est réussi si un utilisateur peut créer un langage complet sans lire le code interne du moteur.
