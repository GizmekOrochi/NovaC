#pragma once

#include "TypeSystem.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace novac::runtime {

class Value;

} // namespace novac::runtime

namespace novac::overload {

struct Candidate {
    std::string name{};
    std::vector<types::TypeRef> params{};
    types::TypeRef result{};
    int priority{};
};

struct RankedCandidate {
    Candidate candidate{};
    int score{};
};

class OverloadSet {
public:
    void add(Candidate candidate);

    const std::vector<Candidate> &candidates() const;

private:
    std::vector<Candidate> candidates_;
};

class OverloadRegistry {
public:
    void add(Candidate candidate);

    std::optional<RankedCandidate> best(const std::string &name, const std::vector<types::TypeRef> &args, const types::TypeRegistry &types) const;

private:
    std::unordered_map<std::string, OverloadSet> sets_;
};

class OperatorRegistry {
public:
    void addOperator(std::string op, Candidate candidate);

    std::optional<RankedCandidate> resolve(const std::string &op, const std::vector<types::TypeRef> &args, const types::TypeRegistry &types) const;

private:
    OverloadRegistry overloads_;
};

} // namespace novac::overload