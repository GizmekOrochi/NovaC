#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"
#include "novac/assets/atomic/OperationFeature.hpp"
#include "novac/engine/EngineController.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace novac::assets::atomic {

/**
 * @brief Configuration options for an AtomicController.
 *
 * Defines the parser domain and AST node kinds used to represent
 * generated unary and binary expressions.
 */
struct AtomicControllerOptions {
    /**
     * @brief Expression parsing domain used by installed features.
     */
    std::string expressionDomain{"expr"};

    /**
     * @brief AST node kind used for generated binary expressions.
     */
    std::string binaryNodeKind{"BinaryExpr"};

    /**
     * @brief AST node kind used for generated unary expressions.
     */
    std::string unaryNodeKind{"UnaryExpr"};
};

/**
 * @brief Coordinates registration of atomic literals and operations.
 *
 * The controller acts as an integration layer between atomic features
 * and an engine controller. It validates feature metadata, registers
 * parser patterns, installs expression node definitions, and maintains
 * metadata describing all registered literals and operations.
 *
 * The controller does not own the referenced EngineController instance.
 */
class AtomicController {
public:
    /**
     * @brief Creates an atomic controller bound to an engine.
     *
     * The configured expression domain becomes the engine's start domain.
     *
     * @param engine Engine controller used for registrations.
     * @param options Controller configuration options.
     *
     * @throws std::runtime_error If any required option is empty.
     */
    explicit AtomicController(controllers::EngineController &engine, AtomicControllerOptions options = {});

    /**
     * @brief Registers a literal feature.
     *
     * The feature is validated, installed into the engine, and recorded
     * in the controller's metadata registry.
     *
     * @param feature Literal feature to install.
     * @return Reference to this controller.
     *
     * @throws std::runtime_error If the feature metadata is invalid or duplicated.
     */
    AtomicController &use(const LiteralFeature &feature);

    /**
     * @brief Registers an operation feature.
     *
     * The feature is validated, installed into the engine, and recorded
     * in the controller's metadata registry.
     *
     * @param feature Operation feature to install.
     * @return Reference to this controller.
     *
     * @throws std::runtime_error If the feature metadata is invalid or duplicated.
     */
    AtomicController &use(const OperationFeature &feature);

    /**
     * @brief Returns the underlying engine controller.
     *
     * @return Mutable engine controller reference.
     */
    controllers::EngineController &engine();

    /**
     * @brief Returns the underlying engine controller.
     *
     * @return Immutable engine controller reference.
     */
    const controllers::EngineController &engine() const;

    /**
     * @brief Returns the expression parsing domain.
     *
     * @return Configured expression domain name.
     */
    const std::string &expressionDomain() const;

    /**
     * @brief Returns the binary expression node kind.
     *
     * @return AST node kind used for binary expressions.
     */
    const std::string &binaryNodeKind() const;

    /**
     * @brief Returns the unary expression node kind.
     *
     * @return AST node kind used for unary expressions.
     */
    const std::string &unaryNodeKind() const;

    /**
     * @brief Checks whether a literal is registered.
     *
     * @param id Literal identifier.
     * @return True if the literal exists.
     */
    bool hasLiteral(const std::string &id) const;

    /**
     * @brief Checks whether an operation is registered.
     *
     * @param id Operation identifier.
     * @return True if the operation exists.
     */
    bool hasOperation(const std::string &id) const;

    /**
     * @brief Returns metadata for all registered literals.
     *
     * @return Registered literal descriptions.
     */
    const std::vector<LiteralInfo> &literals() const;

    /**
     * @brief Returns metadata for all registered operations.
     *
     * @return Registered operation descriptions.
     */
    const std::vector<OperationInfo> &operations() const;

    /**
     * @brief Registers a token pattern with the engine.
     *
     * Duplicate registrations are ignored.
     *
     * @param pattern Pattern to register.
     */
    void registerPattern(const TokenPattern &pattern);

    /**
     * @brief Ensures that the shared binary expression node is installed.
     *
     * The node definition is created only once.
     */
    void ensureBinaryExpressionNode();

    /**
     * @brief Ensures that the shared unary expression node is installed.
     *
     * The node definition and unary dispatch logic are created only once.
     */
    void ensureUnaryExpressionNode();

    /**
     * @brief Registers a runtime handler for a unary operation.
     *
     * @param operationId Unique unary operation identifier.
     * @param handler Runtime evaluation handler.
     *
     * @throws std::runtime_error If the identifier is empty, the handler
     *         is empty, or a handler is already registered for the operation.
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
    std::unordered_set<std::string> literalIds_;
    std::unordered_set<std::string> operationIds_;
    std::unordered_set<std::string> registeredPatterns_;
    std::unordered_map<std::string, runtime::ExprHandler> unaryHandlers_;
    bool binaryNodeInstalled_{};
    bool unaryNodeInstalled_{};
};

} // namespace novac::assets::atomic