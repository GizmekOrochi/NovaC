#pragma once

#include "novac/assets/atomic/AtomicPattern.hpp"
#include "novac/engine/EngineController.hpp"

#include <string>
#include <vector>

namespace novac::assets::atomic {

enum class OperationArity {
    Unary,
    Binary,
    Postfix
};

struct OperationInfo {
    std::string id{};
    std::string version{"0.1.0"};
    std::string description{};
    OperationArity arity{OperationArity::Binary};
    TokenPattern pattern{};
    int precedence{};
    parser::Associativity associativity{parser::Associativity::Left};
    std::vector<std::string> capabilities{};
    std::vector<std::string> requiredCapabilities{};
};

class AtomicController;

class OperationFeature {
public:
    virtual ~OperationFeature();

    virtual OperationInfo info() const = 0;
    virtual void install(AtomicController &controller) const = 0;
};

} // namespace novac::assets::atomic
