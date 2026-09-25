#pragma once

#include <string_view>

#define NOVAC_VERSION_MAJOR 1
#define NOVAC_VERSION_MINOR 2
#define NOVAC_VERSION_PATCH 0
#define NOVAC_VERSION_STRING "1.2.0"

namespace novac {

inline constexpr unsigned VersionMajor = NOVAC_VERSION_MAJOR;
inline constexpr unsigned VersionMinor = NOVAC_VERSION_MINOR;
inline constexpr unsigned VersionPatch = NOVAC_VERSION_PATCH;
inline constexpr std::string_view Version = NOVAC_VERSION_STRING;

} // namespace novac
