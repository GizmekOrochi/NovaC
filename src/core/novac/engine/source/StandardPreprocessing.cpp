#include "novac/engine/source/StandardPreprocessing.hpp"

#include <cctype>
#include <stdexcept>
#include <string>

namespace novac::source {

namespace {

/**
 * @brief Implements the `trim` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
std::string trim(const std::string &value) {
    std::size_t begin{};
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }
    std::size_t end{value.size()};
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return value.substr(begin, end - begin);
}


/**
 * @brief Returns the value required by `requireNoArguments`.
 *
 * @param directive Value supplied for `directive`.
 */
void requireNoArguments(const Directive &directive) {
    if (!trim(directive.arguments).empty()) {
        throw std::runtime_error(
            "standard preprocessing: directive '" + directive.name + "' does not accept arguments");
    }
}

/**
 * @brief Returns the value required by `requireSymbol`.
 *
 * @param directive Value supplied for `directive`.
 * @return Value produced by the operation.
 */
std::string requireSymbol(const Directive &directive) {
    const std::string symbol{trim(directive.arguments)};
    if (symbol.empty()) {
        throw std::runtime_error("standard preprocessing: directive '" + directive.name + "' requires a symbol");
    }
    for (unsigned char ch : symbol) {
        if (std::isalnum(ch) == 0 && ch != '_') {
            throw std::runtime_error(
                "standard preprocessing: invalid symbol '" + symbol + "' for directive '" + directive.name + "'");
        }
    }
    return symbol;
}

/**
 * @brief Returns the value required by `requireSpecifier`.
 *
 * @param directive Value supplied for `directive`.
 * @return Value produced by the operation.
 */
std::string requireSpecifier(const Directive &directive) {
    const std::string value{trim(directive.arguments)};
    if (value.empty()) {
        throw std::runtime_error("standard preprocessing: directive '" + directive.name + "' requires a source specifier");
    }

    if (value.front() == '"') {
        if (value.size() < 2 || value.back() != '"') {
            throw std::runtime_error(
                "standard preprocessing: quoted source specifier must end with '\"'");
        }
        return value.substr(1, value.size() - 2);
    }
    if (value.front() == '<') {
        if (value.size() < 2 || value.back() != '>') {
            throw std::runtime_error(
                "standard preprocessing: bracketed source specifier must end with '>'");
        }
        return value.substr(1, value.size() - 2);
    }

    throw std::runtime_error(
        "standard preprocessing: source specifier for directive '" + directive.name
        + "' must be quoted (\"...\") or bracketed (<...>)");
}

} // namespace

/**
 * @brief Configures the standard behavior provided by `standardPreprocessing`.
 *
 * @return Value produced by the operation.
 */
controllers::EngineFeature standardPreprocessing() {
    controllers::EngineFeature feature{"source.standard-preprocessing"};
    feature
        .description("Standard NovaC preprocessing directives")
        .provides("source.preprocessing.standard")
        .onInstall([](controllers::EngineController &engine) {
            engine.directive("define", [](const Directive &directive, PreprocessorContext &context) {
                context.define(requireSymbol(directive));
            });
            engine.directive("undef", [](const Directive &directive, PreprocessorContext &context) {
                context.undefine(requireSymbol(directive));
            });
            engine.directive("include", [](const Directive &directive, PreprocessorContext &context) {
                context.include(requireSpecifier(directive));
            });
            engine.directive("import", [](const Directive &directive, PreprocessorContext &context) {
                context.import(requireSpecifier(directive));
            });
            engine.directive(
                "if",
                [](const Directive &directive, PreprocessorContext &context) {
                    context.pushCondition(context.defined(requireSymbol(directive)));
                },
                DirectiveMode::Always);
            engine.directive(
                "ifdef",
                [](const Directive &directive, PreprocessorContext &context) {
                    context.pushCondition(context.defined(requireSymbol(directive)));
                },
                DirectiveMode::Always);
            engine.directive(
                "ifndef",
                [](const Directive &directive, PreprocessorContext &context) {
                    context.pushCondition(!context.defined(requireSymbol(directive)));
                },
                DirectiveMode::Always);
            engine.directive(
                "else",
                [](const Directive &directive, PreprocessorContext &context) {
                    requireNoArguments(directive);
                    context.alternateCondition();
                },
                DirectiveMode::Always);
            engine.directive(
                "endif",
                [](const Directive &directive, PreprocessorContext &context) {
                    requireNoArguments(directive);
                    context.popCondition();
                },
                DirectiveMode::Always);
        });
    return feature;
}

} // namespace novac::source
