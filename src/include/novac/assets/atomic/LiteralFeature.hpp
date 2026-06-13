#pragma once

#include "novac/assets/atomic/AtomicPattern.hpp"

#include <string>
#include <vector>

namespace novac::assets::atomic {

class AtomicController;

struct LiteralInfo {
    std::string id{};
    std::string version{"0.1.0"};
    std::string description{};
    std::string nodeKind{};
    TokenPattern pattern{};
    std::vector<std::string> capabilities{};
    std::vector<std::string> requiredCapabilities{};
};

class LiteralFeature {
public:
    virtual ~LiteralFeature();

    virtual LiteralInfo info() const = 0;
    virtual void install(AtomicController &controller) const = 0;
};

} // namespace novac::assets::atomic
