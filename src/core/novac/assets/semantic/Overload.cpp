#include "novac/assets/semantic/Overload.hpp"

#include <stdexcept>
#include <utility>

namespace novac::overload {

void OverloadSet::add(Candidate candidate) {
    candidates_.push_back(std::move(candidate));
}

const std::vector<Candidate> &OverloadSet::candidates() const {
    return candidates_;
}

void OverloadRegistry::add(Candidate candidate) {
    const std::string name{candidate.name};

    sets_[name].add(std::move(candidate));
}

std::optional<RankedCandidate> OverloadRegistry::best(const std::string &name, const std::vector<types::TypeRef> &args, const types::TypeRegistry &types) const {
    const auto iter{sets_.find(name)};

    if (iter == sets_.end())
        return std::nullopt;

    std::optional<RankedCandidate> bestCandidate{};

    for (const Candidate &candidate : iter->second.candidates()) {
        if (candidate.params.size() != args.size())
            continue;

        int score{candidate.priority};
        bool valid{true};

        for (std::size_t index{}; index < args.size(); ++index) {
            if (candidate.params[index]->display() == args[index]->display()) score += 10;
            else if (types.assignable(candidate.params[index], args[index])) score += 1;
            else {
                valid = false;
                break;
            }
        }

        if (!valid)
            continue;

        if (!bestCandidate || score > bestCandidate->score) {
            bestCandidate = RankedCandidate{candidate, score};
            continue;
        }

        if (score == bestCandidate->score)
            throw std::runtime_error("OverloadRegistry::best: ambiguous overload call '" + name + "'");
    }

    return bestCandidate;
}

void OperatorRegistry::addOperator(std::string op, Candidate candidate) {
    candidate.name = std::move(op);

    overloads_.add(std::move(candidate));
}

std::optional<RankedCandidate> OperatorRegistry::resolve(const std::string &op, const std::vector<types::TypeRef> &args, const types::TypeRegistry &types) const {
    return overloads_.best(op, args, types);
}

} // namespace novac::overload