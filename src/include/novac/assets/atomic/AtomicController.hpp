#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"
#include "novac/assets/atomic/OperationFeature.hpp"
#include "novac/engine/EngineController.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace novac::assets::atomic {

/**
* @brief Configuration used by AtomicController.
*
* Defines the expression domain and the AST node kinds used for generated
* binary and unary expression carrier nodes.
*/
struct AtomicControllerOptions {
    std::string expressionDomain{"expr"};
    std::string binaryNodeKind{"BinaryExpr"};
    std::string unaryNodeKind{"UnaryExpr"};
};

/**
* @brief Ownable group of literal features.
*
* Literal packs are used to compose literal support outside of AtomicController.
* The controller takes ownership of the contained features when the pack is used.
*/
struct LiteralPack {
    std::vector<std::unique_ptr<LiteralFeature>> features;

    /**
    * @brief Constructs and appends a literal feature to the pack.
    *
    * @tparam Feature Concrete LiteralFeature type to construct.
    * @tparam Args Constructor argument types.
    * @param args Arguments forwarded to the feature constructor.
    * @return This pack, for fluent composition.
    */
    template <typename Feature, typename... Args>
    LiteralPack &add(Args &&...args) {
        features.push_back(std::make_unique<Feature>(std::forward<Args>(args)...));
        return *this;
    }

    /**
    * @brief Moves all features from another pack into this pack.
    *
    * @param pack Pack whose features are consumed.
    * @return This pack, for fluent composition.
    */
    LiteralPack &merge(LiteralPack pack);
};

/**
* @brief Ownable group of operation features.
*
* Operation packs are used to compose operation support outside of
* AtomicController. The controller takes ownership of the contained features
* when the pack is used.
*/
struct OperationPack {
    std::vector<std::unique_ptr<OperationFeature>> features;

    /**
    * @brief Constructs and appends an operation feature to the pack.
    *
    * @tparam Feature Concrete OperationFeature type to construct.
    * @tparam Args Constructor argument types.
    * @param args Arguments forwarded to the feature constructor.
    * @return This pack, for fluent composition.
    */
    template <typename Feature, typename... Args>
    OperationPack &add(Args &&...args) {
        features.push_back(std::make_unique<Feature>(std::forward<Args>(args)...));
        return *this;
    }

    /**
    * @brief Moves all features from another pack into this pack.
    *
    * @param pack Pack whose features are consumed.
    * @return This pack, for fluent composition.
    */
    OperationPack &merge(OperationPack pack);
};

/**
* @brief Installs atomic literal and operation features into an engine.
*
* AtomicController acts as a registry and installation context for atomic
* expression features. Concrete language choices are provided through
* LiteralPack and OperationPack objects rather than hard-coded controller
* methods.
*/
class AtomicController {
public:
    /**
    * @brief Installs the standard atomic language core.
    *
    * Installs:
    * - standard literals
    * - standard numeric operations
    * - standard comparison operations
    * - standard logical operations
    *
    * @return This controller, for fluent chaining.
    */
    AtomicController &installStandardCore();

    /**
    * @brief Creates a controller bound to an engine.
    *
    * The engine reference is stored and the configured expression domain is set as
    * the engine start domain.
    *
    * @param engine Engine controller configured by this instance. It must outlive
    * the AtomicController.
    * @param options Expression domain and AST node kind configuration.
    * @throws std::runtime_error If any configured domain or node kind is empty.
    */
    explicit AtomicController(controllers::EngineController &engine, AtomicControllerOptions options = {});

    /**
    * @brief Installs a literal feature without taking ownership.
    *
    * @param feature Literal feature to validate and install.
    * @return This controller, for fluent chaining.
    * @throws std::runtime_error If the literal metadata is invalid or duplicated.
    */
    AtomicController &use(const LiteralFeature &feature);

    /**
    * @brief Installs an operation feature without taking ownership.
    *
    * @param feature Operation feature to validate and install.
    * @return This controller, for fluent chaining.
    * @throws std::runtime_error If the operation metadata is invalid or duplicated.
    */
    AtomicController &use(const OperationFeature &feature);

    /**
    * @brief Installs all literal features from a pack.
    *
    * The controller takes ownership of every feature contained in the pack.
    *
    * @param pack Literal pack to consume.
    * @return This controller, for fluent chaining.
    */
    AtomicController &use(LiteralPack pack);

    /**
    * @brief Installs all operation features from a pack.
    *
    * The controller takes ownership of every feature contained in the pack.
    *
    * @param pack Operation pack to consume.
    * @return This controller, for fluent chaining.
    */
    AtomicController &use(OperationPack pack);

    /**
    * @brief Installs and stores ownership of a literal feature.
    *
    * @param feature Literal feature to own and install.
    * @return This controller, for fluent chaining.
    * @throws std::runtime_error If the feature pointer is null or invalid.
    */
    AtomicController &own(std::unique_ptr<LiteralFeature> feature);

    /**
    * @brief Installs and stores ownership of an operation feature.
    *
    * @param feature Operation feature to own and install.
    * @return This controller, for fluent chaining.
    * @throws std::runtime_error If the feature pointer is null or invalid.
    */
    AtomicController &own(std::unique_ptr<OperationFeature> feature);

    /**
    * @brief Compatibility helper installing integer literal support.
    */
    AtomicController &integer();

    /**
    * @brief Compatibility helper installing integer literal support with suffixes.
    */
    AtomicController &integer(std::string nodeKind, std::vector<std::string> suffixes);

    /**
    * @brief Compatibility helper installing floating-point literal support.
    */
    AtomicController &floating();

    /**
    * @brief Compatibility helper installing floating-point literal support with suffixes.
    */
    AtomicController &floating(std::string nodeKind, std::vector<std::string> suffixes);

    /**
    * @brief Compatibility helper installing string literal support.
    */
    AtomicController &stringLiteral();

    /**
    * @brief Compatibility helper installing string literal support with suffixes.
    */
    AtomicController &stringLiteral(std::string nodeKind, std::vector<std::string> suffixes);

    /**
    * @brief Compatibility helper installing boolean literal support.
    */
    AtomicController &boolean();

    /**
    * @brief Compatibility helper installing boolean literal support with custom tokens.
    */
    AtomicController &boolean(std::string nodeKind, std::string trueToken, std::string falseToken);

    /**
    * @brief Compatibility helper installing addition.
    */
    AtomicController &add(std::string token = "+");

    /**
    * @brief Compatibility helper installing subtraction.
    */
    AtomicController &subtract(std::string token = "-");

    /**
    * @brief Installs the standard literal feature set.
    */
    AtomicController &standardLiterals();

    /**
    * @brief Installs standard numeric operations.
    */
    AtomicController &standardNumericOperations();

    /**
    * @brief Installs standard comparison operations.
    */
    AtomicController &standardComparisonOperations();

    /**
    * @brief Installs standard logical operations.
    */
    AtomicController &standardLogicalOperations();

    /**
    * @brief Installs all standard operation features.
    */
    AtomicController &standardOperations();

    /**
    * @brief Installs the standard atomic core.
    */
    AtomicController &standardCore();

    /**
    * @brief Returns the underlying engine controller.
    *
    * @return Mutable engine controller referenced by this AtomicController.
    */
    controllers::EngineController &engine();

    /**
    * @brief Returns the underlying engine controller.
    *
    * @return Const engine controller referenced by this AtomicController.
    */
    const controllers::EngineController &engine() const;

    /**
    * @brief Returns the configured expression domain.
    *
    * @return Expression domain used by this controller.
    */
    const std::string &expressionDomain() const;

    /**
    * @brief Returns the configured binary expression node kind.
    *
    * @return AST node kind used for binary expression carrier nodes.
    */
    const std::string &binaryNodeKind() const;

    /**
    * @brief Returns the configured unary expression node kind.
    *
    * @return AST node kind used for unary expression carrier nodes.
    */
    const std::string &unaryNodeKind() const;

    /**
    * @brief Checks whether a literal id is already installed.
    *
    * @param id Literal feature id.
    * @return true if the literal id is registered; otherwise false.
    */
    bool hasLiteral(const std::string &id) const;

    /**
    * @brief Checks whether an operation id is already installed.
    *
    * @param id Operation feature id.
    * @return true if the operation id is registered; otherwise false.
    */
    bool hasOperation(const std::string &id) const;

    /**
    * @brief Returns metadata for installed literal features.
    *
    * @return Literal metadata in installation order.
    */
    const std::vector<LiteralInfo> &literals() const;

    /**
    * @brief Returns metadata for installed operation features.
    *
    * @return Operation metadata in installation order.
    */
    const std::vector<OperationInfo> &operations() const;

    /**
    * @brief Registers a token pattern with the engine once.
    *
    * Empty token patterns are ignored. Keyword patterns are registered as keywords;
    * other token patterns are registered as symbols.
    *
    * @param pattern Token pattern to register.
    */
    void registerPattern(const TokenPattern &pattern);

    /**
    * @brief Ensures the binary expression carrier node exists.
    *
    * The node is installed at most once and the engine binary node kind is updated
    * to the configured binary node kind.
    */
    void ensureBinaryExpressionNode();

    /**
    * @brief Ensures the unary expression carrier node exists.
    *
    * The node and its evaluator are installed at most once. Unary expression
    * evaluation dispatches to handlers registered with registerUnaryOperation().
    */
    void ensureUnaryExpressionNode();

    /**
    * @brief Registers a runtime handler for a unary operation id.
    *
    * @param operationId Operation id stored in unary expression nodes.
    * @param handler Runtime expression handler used to evaluate the operation.
    * @throws std::runtime_error If the id is empty, the handler is empty, or a
    * handler already exists for the id.
    */
    void registerUnaryOperation(std::string operationId, runtime::ExprHandler handler);

private:
    void validateLiteral(const LiteralInfo &info) const;
    void validateOperation(const OperationInfo &info) const;
    void rememberLiteral(LiteralInfo info);
    void rememberOperation(OperationInfo info);

    controllers::EngineController &engine_;
    AtomicControllerOptions options_;
    std::vector<LiteralInfo> literals_;
    std::vector<OperationInfo> operations_;
    std::vector<std::unique_ptr<LiteralFeature>> ownedLiterals_;
    std::vector<std::unique_ptr<OperationFeature>> ownedOperations_;
    std::unordered_set<std::string> literalIds_;
    std::unordered_set<std::string> operationIds_;
    std::unordered_set<std::string> registeredPatterns_;
    std::unordered_map<std::string, runtime::ExprHandler> unaryHandlers_;
    bool binaryNodeInstalled_{};
    bool unaryNodeInstalled_{};
};

namespace literals {

/**
* @brief Creates a pack containing integer literal support.
*
* @param nodeKind AST node kind used for integer literal nodes.
* @return Literal pack containing the integer literal feature.
*/
LiteralPack integer(std::string nodeKind = "IntegerLiteral");

/**
* @brief Creates a pack containing integer literal support with suffixes.
*
* @param nodeKind AST node kind used for integer literal nodes.
* @param suffixes Accepted suffixes, with or without leading underscores.
* @return Literal pack containing the integer literal feature.
* @throws std::runtime_error If the suffix list is empty or contains an invalid suffix.
*/
LiteralPack integer(std::string nodeKind, std::vector<std::string> suffixes);

/**
* @brief Creates a pack containing floating-point literal support.
*
* @param nodeKind AST node kind used for floating-point literal nodes.
* @return Literal pack containing the floating-point literal feature.
*/
LiteralPack floating(std::string nodeKind = "FloatLiteral");

/**
* @brief Creates a pack containing floating-point literal support with suffixes.
*
* @param nodeKind AST node kind used for floating-point literal nodes.
* @param suffixes Accepted suffixes, with or without leading underscores.
* @return Literal pack containing the floating-point literal feature.
* @throws std::runtime_error If the suffix list is empty or contains an invalid suffix.
*/
LiteralPack floating(std::string nodeKind, std::vector<std::string> suffixes);

/**
* @brief Creates a pack containing string literal support.
*
* @param nodeKind AST node kind used for string literal nodes.
* @return Literal pack containing the string literal feature.
*/
LiteralPack stringLiteral(std::string nodeKind = "StringLiteral");

/**
* @brief Creates a pack containing string literal support with suffixes.
*
* @param nodeKind AST node kind used for string literal nodes.
* @param suffixes Accepted suffixes, with or without leading underscores.
* @return Literal pack containing the string literal feature.
* @throws std::runtime_error If the suffix list is empty or contains an invalid suffix.
*/
LiteralPack stringLiteral(std::string nodeKind, std::vector<std::string> suffixes);

/**
* @brief Creates a pack containing boolean literal support.
*
* @param nodeKind AST node kind used for boolean literal nodes.
* @param trueToken Token recognized as the boolean true literal.
* @param falseToken Token recognized as the boolean false literal.
* @return Literal pack containing the boolean literal feature.
*/
LiteralPack boolean(std::string nodeKind = "BooleanLiteral", std::string trueToken = "true", std::string falseToken = "false");

/**
* @brief Creates a pack containing the standard literal feature set.
*
* Includes integer, floating-point, string, and boolean literal support.
*
* @return Literal pack containing standard literal features.
*/
LiteralPack standard();

}

namespace operations {

/**
* @brief Token configuration for numeric operations.
*/
struct NumericOperationOptions {
    std::string add{"+"};
    std::string subtract{"-"};
    std::string multiply{"*"};
    std::string divide{"/"};
    std::string modulo{"%"};
    std::string negate{"-"};
};

/**
* @brief Token configuration for comparison operations.
*/
struct ComparisonOperationOptions {
    std::string equal{"=="};
    std::string notEqual{"!="};
    std::string less{"<"};
    std::string lessEqual{"<="};
    std::string greater{">"};
    std::string greaterEqual{">="};
};

/**
* @brief Token configuration for logical operations.
*/
struct LogicalOperationOptions {
    std::string andToken{"&&"};
    std::string orToken{"||"};
    std::string notToken{"!"};
};

/**
* @brief Creates a pack containing numeric operation support.
*
* Includes addition, subtraction, multiplication, division, modulo, and numeric
* negation.
*
* @param options Tokens used by numeric operations.
* @return Operation pack containing numeric operation features.
* @throws std::runtime_error If any configured token is empty.
*/
OperationPack numeric(NumericOperationOptions options = {});

/**
* @brief Creates a pack containing comparison operation support.
*
* Includes equality, inequality, less-than, less-than-or-equal, greater-than,
* and greater-than-or-equal operations.
*
* @param options Tokens used by comparison operations.
* @return Operation pack containing comparison operation features.
* @throws std::runtime_error If any configured token is empty.
*/
OperationPack comparison(ComparisonOperationOptions options = {});

/**
* @brief Creates a pack containing logical operation support.
*
* Includes logical AND, logical OR, and logical NOT.
*
* @param options Tokens used by logical operations.
* @return Operation pack containing logical operation features.
* @throws std::runtime_error If any configured token is empty.
*/
OperationPack logical(LogicalOperationOptions options = {});

/**
* @brief Creates a pack containing the standard operation feature set.
*
* Includes numeric, comparison, and logical operation support using default
* tokens.
*
* @return Operation pack containing standard operation features.
*/
OperationPack standard();

}

} // namespace novac::assets::atomic