#include "NovaC.hpp"

#include "novac/assets/atomic/literals/IntegerLiteralAtomic.hpp"
#include "novac/assets/atomic/operations/NumericOperations.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace {

using novac::assets::types::TypeId;

constexpr const char *kDeclaration = "TypedVariableDeclaration";
constexpr const char *kVariable = "TypedVariableExpression";
constexpr const char *kPrint = "PrintStatement";
constexpr const char *kBinary = "BinaryExpr";
constexpr const char *kInteger = "IntegerLiteral";

class TypeChecker {
public:
    TypeChecker(
        const novac::assets::types::TypeController &types,
        const novac::assets::atomic::operations::AddOperationAtomic &add,
        const novac::assets::atomic::operations::SubtractOperationAtomic &sub,
        const novac::assets::atomic::literals::IntegerLiteralAtomic &integer)
        : types_{types}, add_{add}, sub_{sub}, integer_{integer} {}

    void check(const novac::ast::Node &program) {
        if (program.kind() != "Program")
            throw std::runtime_error("TypeChecker: expected Program root");

        for (const auto &statement : program.list("statements")) {
            if (!statement)
                throw std::runtime_error("TypeChecker: null statement");
            checkStatement(*statement);
        }
    }

private:
    TypeId checkExpression(const novac::ast::Node &node) {
        if (node.kind() == kInteger) {
            const auto result{types_.resolveLiteral(integer_, node)};
            if (!result)
                throw std::runtime_error("cannot infer integer literal type");
            return *result;
        }

        if (node.kind() == kVariable) {
            const std::string &name{node.str("name")};
            const auto it{variables_.find(name)};
            if (it == variables_.end())
                throw std::runtime_error("unknown variable '" + name + "'");
            return it->second;
        }

        if (node.kind() == kBinary) {
            const TypeId left{checkExpression(*node.child("left"))};
            const TypeId right{checkExpression(*node.child("right"))};
            const std::array<TypeId, 2> operands{left, right};

            const std::string &op{node.str("op")};
            novac::assets::types::OperationResolutionResult result{};

            if (op == add_.info().id)
                result = types_.resolveOperationDetailed(add_, operands);
            else if (op == sub_.info().id)
                result = types_.resolveOperationDetailed(sub_, operands);
            else
                throw std::runtime_error("unsupported binary operation '" + op + "'");

            if (!result.ok())
                throw std::runtime_error("invalid binary expression: " + result.diagnostic.message);

            return result.resolution->result;
        }

        throw std::runtime_error("cannot type expression node '" + node.kind() + "'");
    }

    void checkStatement(const novac::ast::Node &node) {
        if (node.kind() == kDeclaration) {
            const TypeId declared{node.str("type")};
            const std::string &name{node.str("name")};
            const TypeId value{checkExpression(*node.child("value"))};

            if (!types_.hasType(declared))
                throw std::runtime_error("unknown declared type '" + declared.name + "'");

            if (!types_.canImplicitlyConvert(value, declared)) {
                throw std::runtime_error("cannot initialize '" + name + "' of type " + declared.name + " from " + value.name);
            }

            if (!variables_.emplace(name, types_.canonical(declared)).second)
                throw std::runtime_error("duplicate variable '" + name + "'");
            return;
        }

        if (node.kind() == kPrint) {
            static_cast<void>(checkExpression(*node.child("value")));
            return;
        }

        throw std::runtime_error("unsupported statement node '" + node.kind() + "'");
    }

    const novac::assets::types::TypeController &types_;
    const novac::assets::atomic::operations::AddOperationAtomic &add_;
    const novac::assets::atomic::operations::SubtractOperationAtomic &sub_;
    const novac::assets::atomic::literals::IntegerLiteralAtomic &integer_;
    std::unordered_map<std::string, TypeId> variables_{};
};

std::string readFile(const std::string &path) {
    std::ifstream input{path};
    if (!input)
        throw std::runtime_error("cannot open '" + path + "'");

    return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

} // namespace

int main(int argc, char **argv) {
    using namespace novac;

    controllers::EngineController engine{};

    //Expressions: integer literals, + and -
    assets::atomic::AtomicController atomic{engine};
    assets::atomic::literals::IntegerLiteralAtomic integer{};
    assets::atomic::operations::AddOperationAtomic add{};
    assets::atomic::operations::SubtractOperationAtomic sub{};

    atomic.use(integer).use(add).use(sub);

    //Program root
    assets::essentials::EssentialsController essentials{engine};
    essentials.installProgram();
    engine.setStartDomain("program");

    //Types
    assets::types::TypeController types{};

    types.definePrimitive("short")
        .bits(16)
        .alignmentBytes(2)
        .signedType()
        .representation<assets::types::IntegerRepresentation>(
            assets::types::IntegerEncoding::TwosComplement)
        .commit();

    types.definePrimitive("int")
        .bits(32)
        .alignmentBytes(4)
        .signedType()
        .representation<assets::types::IntegerRepresentation>(
            assets::types::IntegerEncoding::TwosComplement)
        .commit();

    types.definePrimitive("long")
        .bits(64)
        .alignmentBytes(8)
        .signedType()
        .representation<assets::types::IntegerRepresentation>(
            assets::types::IntegerEncoding::TwosComplement)
        .commit();

    // C-like widening conversions. NovaC deliberately does not chain
    // implicit conversions, so short -> long is registered directly.
    types.registerConversion(TypeId{"short"}, TypeId{"int"}, assets::types::ConversionKind::Implicit, 1);
    types.registerConversion(TypeId{"int"}, TypeId{"long"}, assets::types::ConversionKind::Implicit, 1);
    types.registerConversion(TypeId{"short"}, TypeId{"long"}, assets::types::ConversionKind::Implicit, 2);

    // Small integer literals are shorts; the rest are ints. The current NovaC
    // integer literal/runtime representation is int, so this demo does not
    // create source literals outside the host int range.
    types.bindLiteral(integer, [](const ast::Node &node) -> std::optional<TypeId> {
        const int value{node.integer("value")};
        if (value >= -32768 && value <= 32767)
            return TypeId{"short"};
        return TypeId{"int"};
    });

    // short arithmetic follows C-style integer promotion.
    types.registerOperation(add, {TypeId{"short"}, TypeId{"short"}}, TypeId{"int"});
    types.registerOperation(add, {TypeId{"int"}, TypeId{"int"}}, TypeId{"int"});
    types.registerOperation(add, {TypeId{"long"}, TypeId{"long"}}, TypeId{"long"});

    types.registerOperation(sub, {TypeId{"short"}, TypeId{"short"}}, TypeId{"int"});
    types.registerOperation(sub, {TypeId{"int"}, TypeId{"int"}}, TypeId{"int"});
    types.registerOperation(sub, {TypeId{"long"}, TypeId{"long"}}, TypeId{"long"});

    types.finalize();

    //Typed variables -------------------------------------------------
    for (const std::string keyword : {"short", "int", "long"})
        engine.keyword(keyword);
    engine.keyword("print");
    engine.symbol("=");
    engine.symbol(";");

    engine.node({
        .kind = kDeclaration,
        .fields = {
            {.name = "type", .kind = ast::FieldKind::String, .required = true},
            {.name = "name", .kind = ast::FieldKind::String, .required = true},
            {.name = "value", .kind = ast::FieldKind::Node, .required = true}
        },
        .traits = {"stmt", "decl"},
        .doc = "Typed local variable declaration"
    });

    engine.node({
        .kind = kVariable,
        .fields = {{.name = "name", .kind = ast::FieldKind::String, .required = true}},
        .traits = {"expr"},
        .doc = "Typed variable lookup"
    });

    engine.node({
        .kind = kPrint,
        .fields = {{.name = "value", .kind = ast::FieldKind::Node, .required = true}},
        .traits = {"stmt"},
        .doc = "Print statement"
    });

    const auto declarationParser{[&engine](parser::ParserContext &context) {
        const std::string type{context.advance().text};
        const std::string name{context.consumeKind(token::Kind::Identifier).text};
        context.consume("=");
        ast::NodePtr value{context.parse("expr")};
        context.consume(";");

        ast::NodePtr node{engine.makeNode(kDeclaration)};
        node->set("type", type);
        node->set("name", name);
        node->set("value", value);
        return node;
    }};

    engine.parseRule("stmt", "short", declarationParser);
    engine.parseRule("stmt", "int", declarationParser);
    engine.parseRule("stmt", "long", declarationParser);

    engine.parseRule("stmt", "print", [&engine](parser::ParserContext &context) {
        context.consume("print");
        ast::NodePtr value{context.parse("expr")};
        context.consume(";");

        ast::NodePtr node{engine.makeNode(kPrint)};
        node->set("value", value);
        return node;
    });

    engine.prefix("expr", "$identifier", [&engine](parser::ParserContext &context) {
        const std::string name{context.consumeKind(token::Kind::Identifier).text};
        ast::NodePtr node{engine.makeNode(kVariable)};
        node->set("name", name);
        return node;
    });

    // Runtime values are still NovaC runtime::Value integers. Static typing is
    // checked before execution by TypeChecker below.
    engine.statement(kDeclaration, [](const ast::Node &node, runtime::RuntimeContext &context) {
        const std::string &name{node.str("name")};
        runtime::Value value{context.eval(*node.child("value"))};
        if (!context.env().define(name, std::move(value)))
            throw std::runtime_error("duplicate runtime variable '" + name + "'");
    });

    engine.expression(kVariable, [](const ast::Node &node, runtime::RuntimeContext &context) {
        const std::string &name{node.str("name")};
        const runtime::Value *value{context.env().resolve(name)};
        if (!value)
            throw std::runtime_error("unknown runtime variable '" + name + "'");
        return *value;
    });

    engine.statement(kPrint, [](const ast::Node &node, runtime::RuntimeContext &context) {
        std::cout << context.eval(*node.child("value")).toString() << '\n';
    });

    const std::string source{argc > 1 ? readFile(argv[1]) : R"(
short a = 10;
int b = a + 20;
long c = b - 5;
print a;
print b;
print c;
)"};

    const ast::NodePtr program{engine.parse(source)};
    engine.validate(*program);

    TypeChecker checker{types, add, sub, integer};
    checker.check(*program);

    engine.eval(*program);
    return 0;
}
