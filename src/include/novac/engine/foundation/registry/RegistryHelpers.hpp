#pragma once

#include "Registry.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace novac::registry {

/**
 * @brief Inserts or updates a registry entry according to a duplicate policy.
 *
 * @param map Registry storage to update.
 * @param key Entry key.
 * @param value Entry value.
 * @param duplicatePolicy Policy applied when key already exists.
 * @param owner Name used as context in exception messages.
 * @return Registration result.
 *
 * @throws std::runtime_error If key already exists and duplicatePolicy is DuplicatePolicy::Error.
 */
template<class Map, class Value>
RegisterStatus registerEntry(Map &map, std::string key, Value value, DuplicatePolicy duplicatePolicy, const std::string &owner) {
    const auto iter{map.find(key)};

    if (iter != map.end()) {
        if (duplicatePolicy == DuplicatePolicy::Ignore)
            return RegisterStatus::Ignored;

        if (duplicatePolicy == DuplicatePolicy::Replace) {
            iter->second = std::move(value);

            return RegisterStatus::Replaced;
        }

        throw std::runtime_error(owner + ": duplicate registration '" + key + "'");
    }

    map.emplace(std::move(key), std::move(value));

    return RegisterStatus::Inserted;
}

} // namespace novac::registry