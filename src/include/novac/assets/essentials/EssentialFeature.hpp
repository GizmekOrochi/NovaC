#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace novac::assets::essentials {

class EssentialsController;

/**
 * @brief Describes an installable Essentials language feature.
 *
 * EssentialInfo stores the metadata exposed by an EssentialFeature. It is used
 * for identification, discovery, documentation and duplicate detection by the
 * EssentialsController.
 *
 * Features can declare capabilities they provide and capabilities they require.
 * EssentialsController validates those requirements before installation so
 * composition depends on behavior contracts rather than concrete feature ids.
 */
struct EssentialInfo {
    /**
     * @brief Unique feature identifier.
     *
     * The controller uses this value to detect duplicate installations.
     */
    std::string id{};

    /**
     * @brief Feature version.
     */
    std::string version{"0.1.0"};

    /**
     * @brief Human-readable feature description.
     */
    std::string description{};

    /**
     * @brief AST node kinds registered or produced by the feature.
     */
    std::vector<std::string> nodeKinds{};

    /**
     * @brief Semantic AST traits provided by the feature.
     */
    std::vector<std::string> traits{};

    /**
     * @brief Capabilities provided by this feature.
     *
     * Provided capabilities become available to Essentials features installed
     * later.
     */
    std::vector<std::string> capabilities{};

    /**
     * @brief Capabilities required before this feature can be installed.
     */
    std::vector<std::string> requirements{};
};

/**
 * @brief Base interface for installable Essentials features.
 *
 * An Essentials feature represents one reusable statement-level language
 * building block. Concrete implementations expose descriptive metadata through
 * info() and register their syntax, schemas and runtime behavior through
 * install().
 *
 * Features may be installed by reference or owned by an EssentialsController.
 */
class EssentialFeature {
public:
    /**
     * @brief Virtual destructor.
     *
     * Allows concrete feature implementations to be destroyed safely through
     * an EssentialFeature pointer or reference.
     */
    virtual ~EssentialFeature();

    /**
     * @brief Returns metadata describing the feature.
     *
     * @return Feature identification and descriptive metadata.
     */
    virtual EssentialInfo info() const = 0;

    /**
     * @brief Installs the feature into an Essentials controller.
     *
     * Implementations typically register lexer tokens, parser rules, AST
     * schemas and runtime handlers through the controller.
     *
     * @param controller Controller receiving the feature registration.
     */
    virtual void install(EssentialsController &controller) const = 0;
};

/**
 * @brief Ownable collection of Essentials features.
 *
 * EssentialPack makes it possible to compose several feature implementations
 * before transferring them to an EssentialsController.
 *
 * Features are stored with unique ownership.
 */
struct EssentialPack {
    /**
     * @brief Features currently owned by this pack.
     */
    std::vector<std::unique_ptr<EssentialFeature>> features{};

    /**
     * @brief Constructs and appends a feature to the pack.
     *
     * The feature is created with std::make_unique and stored by the pack.
     *
     * @tparam Feature Concrete EssentialFeature type to construct.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to the feature constructor.
     * @return This pack, for fluent composition.
     */
    template <typename Feature, typename... Args>
    EssentialPack &add(Args &&...args) {
        features.push_back(std::make_unique<Feature>(std::forward<Args>(args)...));
        return *this;
    }

    /**
     * @brief Moves every feature from another pack into this pack.
     *
     * Features are moved in their existing order. The consumed pack is cleared
     * after ownership has been transferred.
     *
     * @param pack Pack whose features are consumed.
     * @return This pack, for fluent composition.
     */
    EssentialPack &merge(EssentialPack pack);
};

} // namespace novac::assets::essentials
