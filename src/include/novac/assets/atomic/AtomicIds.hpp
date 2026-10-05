#pragma once

#include "novac/engine/foundation/Ids.hpp"

namespace novac::assets::atomic::fields {

/** @brief Field storing a literal value. */
inline const ids::FieldName Value{"value"};

/** @brief Field storing an atomic operation identifier. */
inline const ids::FieldName Operation{"op"};

/** @brief Field storing the left operand of a binary expression. */
inline const ids::FieldName Left{"left"};

/** @brief Field storing the right operand of a binary expression. */
inline const ids::FieldName Right{"right"};

/** @brief Field storing the operand of a unary expression. */
inline const ids::FieldName Expression{"expr"};

} // namespace novac::assets::atomic::fields

namespace novac::assets::atomic::domains {

/** @brief Default parser domain used by Atomic expression features. */
inline const ids::ParseDomain Expression{"expr"};

} // namespace novac::assets::atomic::domains
