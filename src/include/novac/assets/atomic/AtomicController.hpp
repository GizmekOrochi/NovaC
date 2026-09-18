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
* Defines the parser domain used for atomic expressions and the AST node kinds
* used as shared carrier nodes for binary and unary operations.
*/
struct AtomicControllerOptions {
    /** Parser domain used for atomic expressions. */
    std::string expressionDomain{"expr"};

    /** AST node kind used to represent binary operations. */
    std::string binaryNodeKind{"BinaryExpr"};

    /** AST node kind used to represent unary operations. */
    std::string unaryNodeKind{"UnaryExpr"};
};

/**
* @brief Ownable group of literal features.
*
* Literal packs allow several literal implementations to be composed before
* they are installed into an AtomicController.
*
* Features stored in a pack are owned by the pack until it is consumed by the
* controller.
*/
struct LiteralPack {
    /** Literal features contained in this pack. */
    std::vector<std::unique_ptr<LiteralFeature>> features;

    /**
    * @brief Constructs and appends a literal feature to the pack.
    *
    * The feature is created with std::make_unique and stored by the pack.
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
    * Each feature is moved individually, transferring ownership to this pack.
    *
    * @param pack Pack whose features are consumed.
    * @return This pack, for fluent composition.
    */
    LiteralPack &merge(LiteralPack pack);
};

/**
* @brief Ownable group of operation features.
*
* Operation packs allow several operation implementations to be composed before
* they are installed into an AtomicController.
*
* Features stored in a pack are owned by the pack until it is consumed by the
* controller.
*/
struct OperationPack {
    /** Operation features contained in this pack. */
    std::vector<std::unique_ptr<OperationFeature>> features;

    /**
    * @brief Constructs and appends an operation feature to the pack.
    *
    * The feature is created with std::make_unique and stored by the pack.
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
    * Each feature is moved individually, transferring ownership to this pack.
    *
    * @param pack Pack whose features are consumed.
    * @return This pack, for fluent composition.
    */
    OperationPack &merge(OperationPack pack);
};

/**
* @brief Installs atomic literal and operation features into an engine.
*
* AtomicController provides the installation context used by atomic language
* features. It coordinates parser bindings, AST schemas, runtime handlers,
* feature metadata and shared expression carrier nodes.
*
* Concrete language choices are supplied through LiteralFeature,
* OperationFeature, LiteralPack and OperationPack rather than being hard-coded
* into the engine itself.
*
* The controller references an external EngineController and may also own
* features installed through packs or own().
*/
class AtomicController {
public:
    /**
    * @brief Installs the standard atomic language core.
    *
    * This is a compatibility alias for standardCore().
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
    * The configuration is validated, the engine reference is stored and the
    * configured expression domain becomes the engine's default parser start
    * domain.
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
    * The feature metadata is validated first. The feature is then installed and
    * its metadata is remembered by the controller.
    *
    * The caller remains responsible for the lifetime of the feature object.
    *
    * @param feature Literal feature to validate and install.
    * @return This controller, for fluent chaining.
    * @throws std::runtime_error If the literal metadata is invalid or duplicated.
    */
    AtomicController &use(const LiteralFeature &feature);

    /**
    * @brief Installs an operation feature without taking ownership.
    *
    * The feature metadata is validated first. The feature is then installed and
    * its metadata is remembered by the controller.
    *
    * The caller remains responsible for the lifetime of the feature object.
    *
    * @param feature Operation feature to validate and install.
    * @return This controller, for fluent chaining.
    * @throws std::runtime_error If the operation metadata is invalid or duplicated.
    */
    AtomicController &use(const OperationFeature &feature);

    /**
    * @brief Installs all literal features from a pack.
    *
    * Each feature is passed to own(), transferring ownership to the controller
    * after successful installation.
    *
    * @param pack Literal pack to consume.
    * @return This controller, for fluent chaining.
    */
    AtomicController &use(LiteralPack pack);

    /**
    * @brief Installs all operation features from a pack.
    *
    * Each feature is passed to own(), transferring ownership to the controller
    * after successful installation.
    *
    * @param pack Operation pack to consume.
    * @return This controller, for fluent chaining.
    */
    AtomicController &use(OperationPack pack);

    /**
    * @brief Installs and stores ownership of a literal feature.
    *
    * The pointer is validated, the feature is installed through use(), and the
    * controller keeps the feature alive after installation.
    *
    * @param feature Literal feature to own and install.
    * @return This controller, for fluent chaining.
    * @throws std::runtime_error If the feature pointer is null or invalid.
    */
    AtomicController &own(std::unique_ptr<LiteralFeature> feature);

    /**
    * @brief Installs and stores ownership of an operation feature.
    *
    * The pointer is validated, the feature is installed through use(), and the
    * controller keeps the feature alive after installation.
    *
    * @param feature Operation feature to own and install.
    * @return This controller, for fluent chaining.
    * @throws std::runtime_error If the feature pointer is null or invalid.
    */
    AtomicController &own(std::unique_ptr<OperationFeature> feature);

    /**
    * @brief Compatibility helper installing default integer literal support.
    *
    * @return This controller, for fluent chaining.
    */
    AtomicController &integer();

    /**
    * @brief Compatibility helper installing integer literal support with suffixes.
    *
    * @param nodeKind AST node kind used for the literal.
    * @param suffixes Accepted literal suffixes.
    * @return This controller, for fluent chaining.
    */
    AtomicController &integer(std::string nodeKind, std::vector<std::string> suffixes);

    /**
    * @brief Compatibility helper installing default floating-point literal support.
    *
    * @return This controller, for fluent chaining.
    */
    AtomicController &floating();

    /**
    * @brief Compatibility helper installing floating-point literal support with suffixes.
    *
    * @param nodeKind AST node kind used for the literal.
    * @param suffixes Accepted literal suffixes.
    * @return This controller, for fluent chaining.
    */
    AtomicController &floating(std::string nodeKind, std::vector<std::string> suffixes);

    /**
    * @brief Compatibility helper installing default string literal support.
    *
    * @return This controller, for fluent chaining.
    */
    AtomicController &stringLiteral();

    /**
    * @brief Compatibility helper installing string literal support with suffixes.
    *
    * @param nodeKind AST node kind used for the literal.
    * @param suffixes Accepted literal suffixes.
    * @return This controller, for fluent chaining.
    */
    AtomicController &stringLiteral(std::string nodeKind, std::vector<std::string> suffixes);

    /**
    * @brief Compatibility helper installing default boolean literal support.
    *
    * @return This controller, for fluent chaining.
    */
    AtomicController &boolean();

    /**
    * @brief Compatibility helper installing boolean literal support with custom tokens.
    *
    * @param nodeKind AST node kind used for boolean literals.
    * @param trueToken Token representing true.
    * @param falseToken Token representing false.
    * @return This controller, for fluent chaining.
    */
    AtomicController &boolean(std::string nodeKind, std::string trueToken, std::string falseToken);

    /**
    * @brief Compatibility helper installing addition.
    *
    * A single-operation pack is created and installed using the supplied token.
    *
    * @param token Token used for addition.
    * @return This controller, for fluent chaining.
    */
    AtomicController &add(std::string token = "+");

    /**
    * @brief Compatibility helper installing subtraction.
    *
    * A single-operation pack is created and installed using the supplied token.
    *
    * @param token Token used for subtraction.
    * @return This controller, for fluent chaining.
    */
    AtomicController &subtract(std::string token = "-");

    /**
    * @brief Installs the standard literal feature set.
    *
    * Includes integer, floating-point, string and boolean literals.
    *
    * @return This controller, for fluent chaining.
    */
    AtomicController &standardLiterals();

    /**
    * @brief Installs standard numeric operations.
    *
    * @return This controller, for fluent chaining.
    */
    AtomicController &standardNumericOperations();

    /**
    * @brief Installs standard comparison operations.
    *
    * @return This controller, for fluent chaining.
    */
    AtomicController &standardComparisonOperations();

    /**
    * @brief Installs standard logical operations.
    *
    * @return This controller, for fluent chaining.
    */
    AtomicController &standardLogicalOperations();

    /**
    * @brief Installs all standard operation features.
    *
    * Numeric, comparison and logical operation packs are merged and installed.
    *
    * @return This controller, for fluent chaining.
    */
    AtomicController &standardOperations();

    /**
    * @brief Installs the standard atomic core.
    *
    * Standard literals are installed first, followed by all standard
    * operations.
    *
    * @return This controller, for fluent chaining.
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
    * Metadata is stored in installation order.
    *
    * @return Literal metadata in installation order.
    */
    const std::vector<LiteralInfo> &literals() const;

    /**
    * @brief Returns metadata for installed operation features.
    *
    * Metadata is stored in installation order.
    *
    * @return Operation metadata in installation order.
    */
    const std::vector<OperationInfo> &operations() const;

    /**
    * @brief Registers a concrete token pattern with the engine once.
    *
    * Patterns without token text are ignored because token-key and suffix
    * patterns do not require lexer symbol registration.
    *
    * Keyword patterns are registered as lexer keywords and other concrete
    * patterns are registered as symbols. Previously registered patterns are
    * ignored.
    *
    * @param pattern Token pattern to register.
    */
    void registerPattern(const TokenPattern &pattern);

    /**
    * @brief Ensures the binary expression carrier node exists.
    *
    * The shared binary node schema is installed at most once. It contains an
    * operator id and left and right expression children.
    *
    * The engine runtime binary dispatcher is also configured to use this node
    * kind.
    */
    void ensureBinaryExpressionNode();

    /**
    * @brief Ensures the unary expression carrier node exists.
    *
    * The shared unary node schema and its runtime expression handler are
    * installed at most once.
    *
    * At runtime, the handler reads the node's operation id and dispatches to
    * the corresponding handler registered through registerUnaryOperation().
    */
    void ensureUnaryExpressionNode();

    /**
    * @brief Registers a runtime handler for a unary operation id.
    *
    * Unary carrier nodes store an operation id in their "op" field. During
    * evaluation, that id is used to select one of these handlers.
    *
    * @param operationId Operation id stored in unary expression nodes.
    * @param handler Runtime expression handler used to evaluate the operation.
    * @throws std::runtime_error If the id is empty, the handler is empty, or a
    * handler already exists for the id.
    */
    void registerUnaryOperation(std::string operationId, runtime::ExprHandler handler);

private:
    /**
    * @brief Validates literal metadata before installation.
    *
    * The literal must have a non-empty id and node kind, and its id must not
    * already be installed.
    */
    void validateLiteral(const LiteralInfo &info) const;

    /**
    * @brief Validates operation metadata before installation.
    *
    * The operation must have a non-empty id, a usable token pattern and an id
    * that has not already been installed.
    */
    void validateOperation(const OperationInfo &info) const;

    /**
    * @brief Records metadata for an installed literal.
    *
    * The id is added to the lookup set and the complete metadata is appended
    * to the installation-order list.
    */
    void rememberLiteral(LiteralInfo info);

    /**
    * @brief Records metadata for an installed operation.
    *
    * The id is added to the lookup set and the complete metadata is appended
    * to the installation-order list.
    */
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
* The default implementation matches the generic "$int" parser token key.
*
* @param nodeKind AST node kind used for integer literal nodes.
* @return Literal pack containing the integer literal feature.
*/
LiteralPack integer(std::string nodeKind = "IntegerLiteral");

/**
* @brief Creates a pack containing integer literal support with suffixes.
*
* Suffixes are normalized and combined into a regular-expression token pattern.
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
* The default implementation matches the generic "$float" parser token key.
*
* @param nodeKind AST node kind used for floating-point literal nodes.
* @return Literal pack containing the floating-point literal feature.
*/
LiteralPack floating(std::string nodeKind = "FloatLiteral");

/**
* @brief Creates a pack containing floating-point literal support with suffixes.
*
* Suffixes are normalized and combined into a regular-expression token pattern.
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
* The default implementation matches the generic "$string" parser token key.
*
* @param nodeKind AST node kind used for string literal nodes.
* @return Literal pack containing the string literal feature.
*/
LiteralPack stringLiteral(std::string nodeKind = "StringLiteral");

/**
* @brief Creates a pack containing string literal support with suffixes.
*
* Suffixes are normalized and combined into a regular-expression token pattern.
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
* Boolean literals use configurable concrete tokens for true and false.
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
* Includes integer, floating-point, string, and boolean literal support using
* their default configuration.
*
* @return Literal pack containing standard literal features.
*/
LiteralPack standard();

}

namespace operations {

/**
* @brief Token configuration for numeric operations.
*
* Each field can be replaced to give the standard numeric operation set a
* different surface syntax.
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
*
* Each field can be replaced to customize the concrete comparison operators
* used by the generated feature pack.
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
*
* Each field can be replaced to customize the concrete logical operators used
* by the generated feature pack.
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
* Each configured token is converted to the appropriate TokenPattern before the
* operation feature is added to the pack.
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
* Numeric, comparison and logical packs are merged using their default token
* configuration.
*
* @return Operation pack containing standard operation features.
*/
OperationPack standard();

}

} // namespace novac::assets::atomic