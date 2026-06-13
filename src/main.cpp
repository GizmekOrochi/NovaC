#include <iostream>
#include <string>
#include <vector>

#include "novac/assets/language/LanguageOptionsController.hpp"
#include "novac/assets/language/features/presets/StandardExpressionFeatures.hpp"
#include "novac/engine/EngineController.hpp"
#include "novac/engine/execution/Runtime.hpp"

namespace {

void printTitle(const std::string &title) {
    std::cout << "\n== " << title << " ==\n";
}

void printInstalledFeatures(const novac::language::LanguageOptionsController &language) {
    printTitle("installed language features");

    for (const novac::language::LanguageFeatureInfo &feature : language.features()) {
        std::cout << "- " << feature.name << " v" << feature.version;

        if (!feature.description.empty()) {
            std::cout << " : " << feature.description;
        }

        std::cout << '\n';
    }
}

void runExpressionShowcase(
    const novac::controllers::EngineController &engine,
    const std::vector<std::string> &sources) {
    printTitle("expressions");

    for (const std::string &source : sources) {
        const novac::ast::NodePtr root{engine.parse(source)};
        engine.validate(*root);
        const novac::runtime::Value result{engine.eval(*root)};
        std::cout << source << " => " << result.toString() << '\n';
    }
}

void runStatementShowcase(
    const novac::controllers::EngineController &engine,
    const std::vector<std::string> &sources) {
    printTitle("statements with shared RuntimeContext");

    novac::runtime::RuntimeContext context{engine.runtime()};

    for (const std::string &source : sources) {
        const novac::ast::NodePtr root{engine.parse(source, "stmt")};
        engine.validate(*root);
        context.exec(*root);
        std::cout << "exec: " << source << '\n';
    }

    const novac::ast::NodePtr expression{engine.parse("answer + 2")};
    engine.validate(*expression);
    const novac::runtime::Value value{context.eval(*expression)};
    std::cout << "answer + 2 => " << value.toString() << '\n';
}

} // namespace

int main() {
    novac::controllers::EngineController engine{};
    novac::language::LanguageOptionsController language{engine};

    novac::language::features::installStandardExpressionFeatures(language, "expr");
    novac::language::features::installStandardStatementFeatures(language, "stmt", "expr");

    printInstalledFeatures(language);

    runExpressionShowcase(
        engine,
        {
            "10 + 20 * (3 + 2)",
            "-10 + 4 * 3",
            "-3.5 + 2.25",
            "!(false) && true",
            "10 > 3 && 2 <= 2",
            "3.5 + 2.25",
            "10 / 4",
            "10 / 4.0",
            "10 % 4",
            "10.5 % 4.0",
            "\"nova\""
        });

    runStatementShowcase(
        engine,
        {
            "let answer = 40;",
            "answer + 1;"
        });

    return 0;
}