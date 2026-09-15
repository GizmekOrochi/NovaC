#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace novac::assets::essentials {

class EssentialsController;

/**
 * @brief Describes an Essentials language feature.
 *
 * Stores the metadata exposed by an EssentialFeature, including its stable
 * identifier, version, generated AST node kinds, provided traits, and feature
 * dependencies. The controller uses this information for validation,
 * discovery, and duplicate detection.
 */
struct EssentialInfo {
    /** @brief Unique identifier used to register the feature. */
    std::string id{};
    /** @brief Feature version exposed through controller metadata. */
    std::string version{"0.1.0"};
    /** @brief Human-readable summary of the feature. */
    std::string description{};
    /** @brief AST node kinds registered or produced by the feature. */
    std::vector<std::string> nodeKinds{};
    /** @brief Semantic node traits provided by the feature. */
    std::vector<std::string> traits{};
    /** @brief Feature identifiers that must already be installed. */
    std::vector<std::string> requirements{};
};

/**
 * @brief Base interface for installable Essentials features.
 *
 * An Essentials feature encapsulates one reusable language building block.
 * Implementations expose metadata through info() and register their parser,
 * schema, and runtime behavior through install().
 */
class EssentialFeature {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~EssentialFeature();

    /**
     * @brief Returns metadata describing the feature.
     *
     * @return Feature identification, capabilities, and requirements.
     */
    virtual EssentialInfo info() const = 0;
    /**
     * @brief Installs the feature into an Essentials controller.
     *
     * Implementations register the syntax, AST schemas, and runtime handlers
     * required by the feature.
     *
     * @param controller Controller receiving the feature registration.
     */
    virtual void install(EssentialsController &controller) const = 0;
};

/**
 * @brief Ownable collection of Essentials features.
 *
 * Packs make it possible to compose multiple language building blocks and
 * transfer their ownership to an EssentialsController in one operation.
 */
struct EssentialPack {
    /** @brief Features currently owned by this pack. */
    std::vector<std::unique_ptr<EssentialFeature>> features{};

    /**
     * @brief Constructs and appends a feature to the pack.
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
     * @param pack Pack whose features are consumed.
     * @return This pack, for fluent composition.
     */
    EssentialPack &merge(EssentialPack pack);
};

} // namespace novac::assets::essentials
