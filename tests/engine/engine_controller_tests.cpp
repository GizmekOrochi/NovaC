#include "../tester.hpp"
#include "novac/engine/EngineController.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

using novac::ast::FieldKind;
using novac::ast::FieldSchema;
using novac::ast::Node;
using novac::ast::NodePtr;
using novac::ast::NodeSchema;
using novac::controllers::EngineController;
using novac::controllers::EngineControllerOptions;
using novac::controllers::EngineFeature;
using novac::ir::HIRBuilder;
using novac::ir::Instruction;
using novac::ir::Literal;
using novac::ir::LoweringRegistry;
using novac::ir::MIRBuilder;
using novac::ir::MIRLoweringContext;
using novac::ir::Operand;
using novac::ir::ValueType;
using novac::parser::Associativity;
using novac::parser::ParserContext;
using novac::registry::DuplicatePolicy;
using novac::registry::RegisterStatus;
using novac::runtime::RuntimeContext;
using novac::runtime::Value;
using novac::token::Kind;
using novac::token::Token;

template <typename Fn>
bool throwsRuntimeError(Fn &&fn) {
    try {
        fn();
    } catch (const std::runtime_error &) {
        return true;
    } catch (...) {
        return false;
    }

    return false;
}

Token tok(Kind kind, std::string text, int line = 1, int column = 1) {
    return Token{kind, std::move(text), "", {}, line, column};
}

Token endTok(int line = 1, int column = 1) {
    return tok(Kind::End, "", line, column);
}

std::vector<Token> tokens(std::initializer_list<Token> values) {
    return std::vector<Token>{values};
}

NodeSchema literalSchema() {
    return NodeSchema{"IntegerLiteral", { FieldSchema{"value", FieldKind::Int, true, {}, {}}}, {"Expression"}, "Integer literal."};
}

NodePtr integerNode(int value) {
    NodePtr node{Node::make("IntegerLiteral")};
    node->set("value", value);
    return node;
}

void installIntegerParser(EngineController &engine, const std::string &domain = "expr") {
    engine.prefix(domain, "$int", [](ParserContext &context) {
        const Token token{context.consumeKind(Kind::Integer)};
        NodePtr node{Node::make("IntegerLiteral")};
        node->set("value", std::stoi(token.text));
        return node;
    });
}

void installIntegerRuntime(EngineController &engine) {
    engine.expression("IntegerLiteral", [](const Node &node, RuntimeContext &) {
        return Value::integer(node.integer("value"));
    });
}

void installSimpleHIRLowering(EngineController &engine) {
    engine.hir("IntegerLiteral", [](const Node &node, HIRBuilder &out, const LoweringRegistry &) {
        return out.emitValue("const.i32", {Operand::fromLiteral(node.integer("value"))}, ValueType::Int);
    });
}

void installSimpleMIRLowering(EngineController &engine) {
    engine.mir("const.i32", [](const Instruction &instruction, MIRBuilder &out, MIRLoweringContext &context, const LoweringRegistry &) {
        std::vector<Operand> operands{context.remapOperands(instruction.operands)};
        return out.emitValue("mov.i32", std::move(operands), instruction.type);
    });
}



} // namespace

TEST(EngineController, DefaultConstruction) {
    EngineController engine{};

    CHECK(engine.startDomain().empty());
    CHECK(engine.diagnostics().empty());
    CHECK(!engine.hasFeature("core"));
    CHECK(!engine.hasCapability("lexer"));
    CHECK(engine.lexer().symbols().empty());
}

TEST(EngineController, RegistersStandaloneCapability) {
    EngineController engine{};

    engine.registerCapability("asset.expression");

    CHECK(engine.hasCapability("asset.expression"));
}

TEST(EngineController, RejectsEmptyStandaloneCapability) {
    EngineController engine{};

    CHECK(throwsRuntimeError([&]() { engine.registerCapability(""); }));
}

TEST(EngineController, ConstructionWithOptions) {
    EngineController engine{EngineControllerOptions{DuplicatePolicy::Ignore, "expr"}};

    CHECK(engine.startDomain() == "expr");
}

TEST(EngineController, ExposesSubsystemRegistries) {
    EngineController engine{};

    CHECK(&engine.lexer() == &engine.lexer());
    CHECK(&engine.nodes() == &engine.nodes());
    CHECK(&engine.parser() == &engine.parser());
    CHECK(&engine.runtime() == &engine.runtime());
    CHECK(&engine.lowering() == &engine.lowering());
}

TEST(EngineController, RegistersKeywordsAndSymbols) {
    EngineController engine{};

    CHECK(engine.keyword("if") == RegisterStatus::Inserted);
    CHECK(engine.symbol("+") == RegisterStatus::Inserted);

    CHECK(engine.lexer().isKeyword("if"));
    CHECK(engine.lexer().symbols().size() == 1);
    CHECK(engine.lexer().symbols()[0] == "+");
}

TEST(EngineController, TokenizesUsingConfiguredLexer) {
    EngineController engine{};
    engine.keyword("if");
    engine.symbol("+");

    std::vector<Token> output{engine.tokenize("if x + 1")};

    CHECK(output.size() == 5);
    CHECK(output[0].kind == Kind::Keyword);
    CHECK(output[0].text == "if");
    CHECK(output[1].kind == Kind::Identifier);
    CHECK(output[1].text == "x");
    CHECK(output[2].kind == Kind::Symbol);
    CHECK(output[2].text == "+");
    CHECK(output[3].kind == Kind::Integer);
    CHECK(output[3].text == "1");
    CHECK(output[4].kind == Kind::End);
}

TEST(EngineController, RegistersAndValidatesNodeSchema) {
    EngineController engine{};

    CHECK(engine.node(literalSchema()) == RegisterStatus::Inserted);

    NodePtr node{engine.makeNode("IntegerLiteral")};
    node->set("value", 42);

    engine.validate(*node);
}

TEST(EngineController, RejectsInvalidNodeDuringValidation) {
    EngineController engine{};
    engine.node(literalSchema());

    NodePtr node{engine.makeNode("IntegerLiteral")};

    CHECK(throwsRuntimeError([&]() { engine.validate(*node); }));
}

TEST(EngineController, MakesNodesFromStringAndTypedKind) {
    EngineController engine{};

    NodePtr first{engine.makeNode("IntegerLiteral")};
    novac::ids::NodeKind kind{"Identifier"};
    NodePtr second{engine.makeNode(kind)};

    CHECK(first != nullptr);
    CHECK(first->kind() == "IntegerLiteral");
    CHECK(second != nullptr);
    CHECK(second->kind() == "Identifier");
}

TEST(EngineController, RejectsEmptyNodeKind) {
    EngineController engine{};

    CHECK(throwsRuntimeError([&]() { static_cast<void>(engine.makeNode("")); }));
}

TEST(EngineController, RequiresStartDomainForDefaultParse) {
    EngineController engine{};
    installIntegerParser(engine);

    CHECK(throwsRuntimeError([&]() { static_cast<void>(engine.parse("1")); }));
    CHECK(throwsRuntimeError([&]() { static_cast<void>(engine.parseTokens(tokens({tok(Kind::Integer, "1"), endTok()}))); }));
}

TEST(EngineController, SetsStartDomain) {
    EngineController engine{};

    engine.setStartDomain("expr");

    CHECK(engine.startDomain() == "expr");
}

TEST(EngineController, RejectsEmptyStartDomain) {
    EngineController engine{};

    CHECK(throwsRuntimeError([&]() { engine.setStartDomain(""); }));
}

TEST(EngineController, ParsesSourceWithExplicitDomain) {
    EngineController engine{};
    installIntegerParser(engine);

    NodePtr node{engine.parse("42", "expr")};

    CHECK(node != nullptr);
    CHECK(node->kind() == "IntegerLiteral");
    CHECK(node->integer("value") == 42);
}

TEST(EngineController, ParsesSourceWithConfiguredStartDomain) {
    EngineController engine{EngineControllerOptions{DuplicatePolicy::Error, "expr"}};
    installIntegerParser(engine);

    NodePtr node{engine.parse("7")};

    CHECK(node != nullptr);
    CHECK(node->integer("value") == 7);
}

TEST(EngineController, ParsesTokensWithExplicitDomain) {
    EngineController engine{};
    installIntegerParser(engine);

    NodePtr node{engine.parseTokens(tokens({tok(Kind::Integer, "9"), endTok()}), "expr")};

    CHECK(node != nullptr);
    CHECK(node->integer("value") == 9);
}

TEST(EngineController, ParsesTokensWithConfiguredStartDomain) {
    EngineController engine{EngineControllerOptions{DuplicatePolicy::Error, "expr"}};
    installIntegerParser(engine);

    NodePtr node{engine.parseTokens(tokens({tok(Kind::Integer, "11"), endTok()}))};

    CHECK(node != nullptr);
    CHECK(node->integer("value") == 11);
}

TEST(EngineController, RejectsEmptyExplicitParseDomain) {
    EngineController engine{};

    CHECK(throwsRuntimeError([&]() { static_cast<void>(engine.parseTokens(tokens({endTok()}), "")); }));
}

TEST(EngineController, RegistersParserFallback) {
    EngineController engine{};

    CHECK(engine.fallback("expr", [](ParserContext &context) -> NodePtr {
        if (context.cur().kind != Kind::Identifier) {
            return nullptr;
        }

        NodePtr node{Node::make("Identifier")};
        node->set("name", context.advance().text);
        return node;
    }) == RegisterStatus::Inserted);

    NodePtr node{engine.parseTokens(tokens({tok(Kind::Identifier, "x"), endTok()}), "expr")};

    CHECK(node != nullptr);
    CHECK(node->kind() == "Identifier");
    CHECK(node->str("name") == "x");
}

TEST(EngineController, RegistersParseRule) {
    EngineController engine{};

    CHECK(engine.parseRule("stmt", "return", [](ParserContext &context) {
        context.consume("return");
        return Node::make("ReturnStatement");
    }) == RegisterStatus::Inserted);

    NodePtr node{engine.parseTokens(tokens({tok(Kind::Keyword, "return"), endTok()}), "stmt")};

    CHECK(node != nullptr);
    CHECK(node->kind() == "ReturnStatement");
}

TEST(EngineController, RegistersPrattInfixAndPostfix) {
    EngineController engine{};

    installIntegerParser(engine);
    engine.infix("expr", "+", 10, [](ParserContext &, NodePtr left, Token op, NodePtr right) {
        NodePtr node{Node::make("BinaryExpression")};
        node->set("left", left);
        node->set("op", op.text);
        node->set("right", right);
        return node;
    });
    engine.postfix("expr", "!", 20, [](ParserContext &, NodePtr left, Token op) {
        NodePtr node{Node::make("PostfixExpression")};
        node->set("left", left);
        node->set("op", op.text);
        return node;
    });

    NodePtr node{engine.parseTokens(tokens({
        tok(Kind::Integer, "1"),
        tok(Kind::Symbol, "+"),
        tok(Kind::Integer, "2"),
        tok(Kind::Symbol, "!"),
        endTok()
    }), "expr")};

    CHECK(node->kind() == "BinaryExpression");
    CHECK(node->str("op") == "+");
    CHECK(node->child("right")->kind() == "PostfixExpression");
}

TEST(EngineController, RegistersTypedParserRules) {
    EngineController engine{};
    novac::ids::ParseDomain domain{"typed"};

    CHECK(engine.prefix(domain, "$int", [](ParserContext &context) {
        const Token token{context.consumeKind(Kind::Integer)};
        return integerNode(std::stoi(token.text));
    }) == RegisterStatus::Inserted);

    CHECK(engine.infix(domain, "+", 10, Associativity::Left, [](ParserContext &, NodePtr left, Token op, NodePtr right) {
        NodePtr node{Node::make("BinaryExpression")};
        node->set("left", left);
        node->set("op", op.text);
        node->set("right", right);
        return node;
    }) == RegisterStatus::Inserted);

    CHECK(engine.postfix(domain, "!", 20, [](ParserContext &, NodePtr left, Token op) {
        NodePtr node{Node::make("PostfixExpression")};
        node->set("left", left);
        node->set("op", op.text);
        return node;
    }) == RegisterStatus::Inserted);

    NodePtr node{engine.parseTokens(tokens({tok(Kind::Integer, "1"), endTok()}), "typed")};

    CHECK(node->kind() == "IntegerLiteral");
    CHECK(node->integer("value") == 1);
}

TEST(EngineController, EvaluatesExpression) {
    EngineController engine{};
    installIntegerRuntime(engine);

    Node node{"IntegerLiteral"};
    node.set("value", 42);

    CHECK(engine.eval(node).asInt() == 42);
}

TEST(EngineController, ExecutesStatement) {
    EngineController engine{};
    bool called{false};

    CHECK(engine.statement("Print", [&](const Node &, RuntimeContext &) {
        called = true;
    }) == RegisterStatus::Inserted);

    Node node{"Print"};
    engine.exec(node);

    CHECK(called);
}

TEST(EngineController, RegistersTypedRuntimeHandlers) {
    EngineController engine{};

    novac::ids::NodeKind exprKind{"Literal"};
    novac::ids::NodeKind stmtKind{"Stmt"};
    novac::ids::NodeKind declKind{"Decl"};

    CHECK(engine.expression(exprKind, [](const Node &, RuntimeContext &) {
        return Value::integer(1);
    }) == RegisterStatus::Inserted);

    CHECK(engine.statement(stmtKind, [](const Node &, RuntimeContext &) {
    }) == RegisterStatus::Inserted);

    CHECK(engine.declaration(declKind, [](const NodePtr &, RuntimeContext &) {
    }) == RegisterStatus::Inserted);

    Node expr{"Literal"};
    Node stmt{"Stmt"};

    CHECK(engine.eval(expr).asInt() == 1);
    engine.exec(stmt);
}

TEST(EngineController, EvaluatesBinaryOperator) {
    EngineController engine{};

    CHECK(engine.binaryOperator("+", [](const Node &, RuntimeContext &) {
        return Value::integer(3);
    }) == RegisterStatus::Inserted);

    Node node{"binary"};
    node.set("op", std::string{"+"});

    CHECK(engine.eval(node).asInt() == 3);
}

TEST(EngineController, EvaluatesTypedBinaryOperator) {
    EngineController engine{};
    novac::ids::Operation op{"+"};

    CHECK(engine.binaryOperator(op, [](const Node &, RuntimeContext &) {
        return Value::integer(5);
    }) == RegisterStatus::Inserted);

    Node node{"binary"};
    node.set("op", std::string{"+"});

    CHECK(engine.eval(node).asInt() == 5);
}

TEST(EngineController, SetsBinaryNodeKind) {
    EngineController engine{};

    engine.setBinaryNodeKind("BinaryExpression");

    CHECK(engine.runtime().binaryNodeKind() == "BinaryExpression");
}

TEST(EngineController, LowersAstToHIR) {
    EngineController engine{};
    installSimpleHIRLowering(engine);

    Node node{"IntegerLiteral"};
    node.set("value", 42);

    auto hir{engine.lowerToHIR(node)};

    CHECK(hir.blocks.size() == 1);
    CHECK(hir.blocks[0].instructions.size() == 1);
    CHECK(hir.blocks[0].instructions[0].op == "const.i32");
    CHECK(hir.blocks[0].instructions[0].type == ValueType::Int);
}

TEST(EngineController, LowersHIRToMIR) {
    EngineController engine{};
    installSimpleMIRLowering(engine);

    novac::ir::HIRModule hir{};
    hir.blocks.push_back(novac::ir::BasicBlock{novac::ir::BlockId{0}, "entry", {}, {}});
    hir.blocks[0].instructions.push_back(novac::ir::Instruction{
        "const.i32",
        novac::ir::ValueId{0},
        {Operand::fromLiteral(42)},
        ValueType::Int
    });

    auto mir{engine.lowerToMIR(hir)};

    CHECK(mir.blocks.size() == 1);
    CHECK(mir.blocks[0].instructions.size() == 1);
    CHECK(mir.blocks[0].instructions[0].op == "mov.i32");
    CHECK(mir.blocks[0].instructions[0].result.has_value());
}

TEST(EngineController, LowersAstDirectlyToMIR) {
    EngineController engine{};
    installSimpleHIRLowering(engine);
    installSimpleMIRLowering(engine);

    Node node{"IntegerLiteral"};
    node.set("value", 42);

    auto mir{engine.lowerToMIR(node)};

    CHECK(mir.blocks.size() == 1);
    CHECK(mir.blocks[0].instructions.size() == 1);
    CHECK(mir.blocks[0].instructions[0].op == "mov.i32");
}

TEST(EngineController, ReportsTokenizeErrorsToDiagnostics) {
    EngineController engine{};
    CHECK(throwsRuntimeError([&]() { static_cast<void>(engine.tokenize("@")); }));
    CHECK(engine.diagnostics().hasErrors());
    CHECK(engine.diagnostics().diagnostics().size() == 1);
}

TEST(EngineController, ReportsParseErrorsToDiagnostics) {
    EngineController engine{EngineControllerOptions{DuplicatePolicy::Error, "expr"}};
    installIntegerParser(engine);
    CHECK(throwsRuntimeError([&]() { static_cast<void>(engine.parse("1 2")); }));
    CHECK(engine.diagnostics().hasErrors());
    CHECK(engine.diagnostics().diagnostics().size() == 1);
}

TEST(EngineController, ReportsRuntimeErrorsToDiagnostics) {
    EngineController engine{};
    Node node{"MissingRuntimeHandler"};
    CHECK(throwsRuntimeError([&]() { static_cast<void>(engine.eval(node)); }));
    CHECK(engine.diagnostics().hasErrors());
    CHECK(engine.diagnostics().diagnostics().size() == 1);
}

TEST(EngineController, ReportsValidationErrorsToDiagnostics) {
    EngineController engine{};
    engine.node(literalSchema());
    Node node{"IntegerLiteral"};
    CHECK(throwsRuntimeError([&]() { engine.validate(node); }));
    CHECK(engine.diagnostics().hasErrors());
    CHECK(engine.diagnostics().diagnostics().size() == 1);
}

TEST(EngineController, ProvidesDiagnosticsAccess) {
    EngineController engine{};

    engine.diagnostics().error("boom");

    CHECK(engine.diagnostics().hasErrors());

    const EngineController &constEngine{engine};
    CHECK(constEngine.diagnostics().hasErrors());
}

TEST(EngineController, InstallsFeatureAtomically) {
    EngineController engine{};

    EngineFeature feature{"core"};
    feature
        .version("1.0.0")
        .provides("core")
        .onInstall([](EngineController &candidate) {
            candidate.keyword("if");
            candidate.symbol("+");
        });

    engine.install(feature);

    CHECK(engine.hasFeature("core"));
    CHECK(engine.hasCapability("core"));
    CHECK(engine.lexer().isKeyword("if"));
    CHECK(engine.lexer().symbols().size() == 1);
}

TEST(EngineController, RejectsDuplicateFeature) {
    EngineController engine{};

    EngineFeature feature{"core"};
    feature.provides("core");

    engine.install(feature);

    CHECK(throwsRuntimeError([&]() { engine.install(feature); }));
}

TEST(EngineController, RejectsFeatureWithMissingCapability) {
    EngineController engine{};

    EngineFeature feature{"parser"};
    feature.requiresCapability("lexer");

    CHECK(throwsRuntimeError([&]() { engine.install(feature); }));
    CHECK(!engine.hasFeature("parser"));
}

TEST(EngineController, InstallsFeatureWhenDependencyIsPresent) {
    EngineController engine{};

    EngineFeature lexer{"lexer"};
    lexer.provides("lexer");

    EngineFeature parser{"parser"};
    parser.dependsOn("lexer").provides("parser");

    engine.install(lexer);
    engine.install(parser);

    CHECK(engine.hasFeature("lexer"));
    CHECK(engine.hasFeature("parser"));
    CHECK(engine.hasCapability("lexer"));
    CHECK(engine.hasCapability("parser"));
}

TEST(EngineController, RejectsFeatureConflictDeclaredByCandidate) {
    EngineController engine{};

    EngineFeature first{"first"};
    EngineFeature second{"second"};
    second.conflictsWith("first");

    engine.install(first);

    CHECK(throwsRuntimeError([&]() { engine.install(second); }));
    CHECK(!engine.hasFeature("second"));
}

TEST(EngineController, RejectsFeatureConflictDeclaredByInstalledFeature) {
    EngineController engine{};

    EngineFeature first{"first"};
    first.conflictsWith("second");

    EngineFeature second{"second"};

    engine.install(first);

    CHECK(throwsRuntimeError([&]() { engine.install(second); }));
    CHECK(!engine.hasFeature("second"));
}

TEST(EngineController, FailedFeatureInstallDoesNotMutateEngine) {
    EngineController engine{};

    EngineFeature feature{"bad"};
    feature.onInstall([](EngineController &candidate) {
        candidate.keyword("if");
        throw std::runtime_error("install failed");
    });

    CHECK(throwsRuntimeError([&]() { engine.install(feature); }));
    CHECK(!engine.hasFeature("bad"));
    CHECK(!engine.lexer().isKeyword("if"));
}

TEST(EngineController, SnapshotAndRestore) {
    EngineController engine{};
    engine.keyword("if");

    EngineController snapshot{engine.snapshot()};

    engine.keyword("while");
    CHECK(engine.lexer().isKeyword("if"));
    CHECK(engine.lexer().isKeyword("while"));

    engine.restore(snapshot);

    CHECK(engine.lexer().isKeyword("if"));
    CHECK(!engine.lexer().isKeyword("while"));
}

