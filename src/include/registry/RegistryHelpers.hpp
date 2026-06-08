#pragma once

#include "Registry.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace novac::registry {

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