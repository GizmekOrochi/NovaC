#ifndef TESTER_HPP
#define TESTER_HPP

#include <chrono>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace tester {

inline constexpr const char* RED = "\033[31m";
inline constexpr const char* GREEN = "\033[32m";
inline constexpr const char* YELLOW = "\033[33m";
inline constexpr const char* RESET = "\033[0m";

struct Failure : std::exception {
    std::string message;

    explicit Failure(std::string msg) : message(std::move(msg)) {}

    const char* what() const noexcept override {
        return message.c_str();
    }
};

struct TestCase {
    std::string suite;
    std::string name;
    void (*function)();
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char* suite, const char* name, void (*function)()) {
        registry().push_back({suite, name, function});
    }
};

inline std::string full_name(const TestCase& test) {
    return test.suite + "." + test.name;
}

inline void fail_check(const char* expr, const char* file, int line) {
    std::ostringstream oss;
    oss << file << ':' << line << "\n" << "             Check failed: " << expr;
    throw Failure{oss.str()};
}

template <typename A, typename B>
inline void fail_eq(const A& actual, const B& expected, const char* actual_expr, const char* expected_expr, const char* file, int line) {
    std::ostringstream oss;
    oss << file << ':' << line << "\n"
        << "             Expected equality of these values:\n"
        << "               " << actual_expr << " = " << actual << "\n"
        << "               " << expected_expr << " = " << expected;
    throw Failure{oss.str()};
}

template <typename A, typename B>
inline void fail_near(const A& actual, const B& expected, double eps, const char* actual_expr, const char* expected_expr, const char* eps_expr, const char* file, int line) {
    std::ostringstream oss;
    oss << file << ':' << line << "\n"
        << "             Expected near equality:\n"
        << "               " << actual_expr << " = " << actual << "\n"
        << "               " << expected_expr << " = " << expected << "\n"
        << "               epsilon " << eps_expr << " = " << eps;
    throw Failure{oss.str()};
}

inline int run_all_tests() {
    auto& tests = registry();

    std::vector<std::string> failed_tests;
    int passed = 0;

    std::cout << "[==========] Running " << tests.size() << " test(s).\n\n";

    for (const auto& test : tests) {
        const std::string name = full_name(test);

        std::cout << "[ RUN      ] " << name << '\n';
        const auto start = std::chrono::steady_clock::now();

        try {
            test.function();

            const auto end = std::chrono::steady_clock::now();
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            std::cout << GREEN << "[  PASSED  ] " << RESET << name << " (" << ms << " ms)\n\n";

            ++passed;
        } catch (const Failure& e) {
            failed_tests.push_back(name);
            std::cout << RED << "[  FAILED  ] " << RESET << name << '\n' << "             " << e.what() << "\n\n";
        } catch (const std::exception& e) {
            failed_tests.push_back(name);
            std::cout << RED << "[  FAILED  ] " << RESET << name << '\n' << "             Unexpected std::exception: " << e.what() << "\n\n";
        } catch (...) {
            failed_tests.push_back(name);
            std::cout << RED << "[  FAILED  ] " << RESET << name << '\n' << "             Unknown exception\n\n";
        }
    }

    std::cout << "========================================\n";

    if (!failed_tests.empty()) {
        std::cout << RED << "Failed tests:\n" << RESET;
        for (const auto& name : failed_tests) {
            std::cout << "  - " << name << '\n';
        }
        std::cout << '\n';
    }

    std::cout << "[==========] " << tests.size() << " test(s) ran.\n";
    std::cout << GREEN << "[  PASSED  ] " << passed << " test(s).\n" << RESET;

    if (!failed_tests.empty()) {
        std::cout << RED << "[  FAILED  ] " << failed_tests.size() << " test(s).\n" << RESET;
    } else {
        std::cout << GREEN << "100% tests passed.\n" << RESET;
    }

    return failed_tests.empty() ? 0 : 1;
}

} // namespace tester

#define TEST(SUITE, NAME)                                                        \
    static void tester_test_##SUITE##_##NAME();                                  \
    static ::tester::Registrar tester_registrar_##SUITE##_##NAME(                \
        #SUITE, #NAME, &tester_test_##SUITE##_##NAME                             \
    );                                                                           \
    static void tester_test_##SUITE##_##NAME()

#define CHECK(EXPR)                                                              \
    do {                                                                         \
        if (!(EXPR)) {                                                           \
            ::tester::fail_check(#EXPR, __FILE__, __LINE__);                     \
        }                                                                        \
    } while (false)

#define CHECK_EQ(ACTUAL, EXPECTED)                                               \
    do {                                                                         \
        const auto& tester_actual_value = (ACTUAL);                              \
        const auto& tester_expected_value = (EXPECTED);                          \
        if (!(tester_actual_value == tester_expected_value)) {                   \
            ::tester::fail_eq(                                                   \
                tester_actual_value,                                             \
                tester_expected_value,                                           \
                #ACTUAL,                                                         \
                #EXPECTED,                                                       \
                __FILE__,                                                        \
                __LINE__                                                         \
            );                                                                   \
        }                                                                        \
    } while (false)

#define CHECK_NEAR(ACTUAL, EXPECTED, EPSILON)                                    \
    do {                                                                         \
        const auto tester_actual_value = (ACTUAL);                               \
        const auto tester_expected_value = (EXPECTED);                           \
        const auto tester_epsilon_value = (EPSILON);                             \
        if (std::fabs(static_cast<double>(tester_actual_value) -                 \
                      static_cast<double>(tester_expected_value)) >              \
            static_cast<double>(tester_epsilon_value)) {                         \
            ::tester::fail_near(                                                 \
                tester_actual_value,                                             \
                tester_expected_value,                                           \
                static_cast<double>(tester_epsilon_value),                       \
                #ACTUAL,                                                         \
                #EXPECTED,                                                       \
                #EPSILON,                                                        \
                __FILE__,                                                        \
                __LINE__                                                         \
            );                                                                   \
        }                                                                        \
    } while (false)

#define REQUIRE(EXPR) CHECK(EXPR)
#define REQUIRE_EQ(ACTUAL, EXPECTED) CHECK_EQ(ACTUAL, EXPECTED)
#define REQUIRE_NEAR(ACTUAL, EXPECTED, EPSILON) CHECK_NEAR(ACTUAL, EXPECTED, EPSILON)

#endif // TESTER_HPP