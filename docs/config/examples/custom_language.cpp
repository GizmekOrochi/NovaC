#include <iostream>
#include <string>

#include "NovaC.hpp"

int main() {
    novac::controllers::EngineController engine{};

    novac::assets::atomic::AtomicController atomic{engine};
    atomic.installStandardCore();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.functions.functionKeyword = "fn";
    options.functions.returnKeyword = "give";
    options.functions.mainFunctionName = "start";
    options.variables.letKeyword = "var";

    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installStandardCore();

    essentials.functionRegistry().native(
        "print",
        [](const novac::ast::NodeList &arguments,
           novac::runtime::RuntimeContext &context)
           -> novac::runtime::Value {
            for(const auto &argument : arguments)
                std::cout << context.eval(*argument).toString();

            std::cout << '\n';
            return novac::runtime::Value::voidValue();
        }
    );

    const std::string source{R"(
fn factorial(n) {
    if n <= 1 {
        give 1;
    }

    give n * factorial(n - 1);
}

fn start() {
    var sum = 0;

    for(i = 0; i < 5; i = i + 1) {
        sum = sum + i;
    }

    print("factorial(5) =");
    print(factorial(5));
    print("sum =");
    print(sum);

    give factorial(5) + sum;
}
)"};

    const auto program{engine.parse(source)};
    engine.validate(*program);

    const auto result{engine.eval(*program)};
    std::cout << "returned: " << result.toString() << '\n';
}
