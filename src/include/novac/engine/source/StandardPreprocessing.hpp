#pragma once

#include "../EngineController.hpp"

namespace novac::source {

/**
 * @brief Creates the standard preprocessing feature.
 *
 * The feature installs #define, #undef, #include, #import, #if, #ifdef,
 * #ifndef, #else and #endif. Conditions are symbol-presence checks only; this
 * feature does not implement macro text substitution or C-style #if expressions.
 *
 * Pragma dispatch remains generic and language features can register their own
 * named hooks through EngineController::pragma().
 *
 * @return Engine feature containing the standard directive registrations.
 */
controllers::EngineFeature standardPreprocessing();

} // namespace novac::source
