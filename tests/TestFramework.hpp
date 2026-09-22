//
// Adel Lis created TestFramework on 22/09/2026.
//

#pragma once

#include <iostream>
#include <string_view>

namespace test
{
    inline int g_checks = 0;
    inline int g_failures = 0;

    struct Case
    {
        std::string_view name;
        void (*fn)();
    };

    /// Run every case, print a line each, and return a process exit code.
    template <std::size_t N>
    int run(const Case (&cases)[N])
    {
        int failed_cases = 0;
        for (const Case& c : cases)
        {
            const int before = g_failures;
            c.fn();
            const bool ok = (g_failures == before);
            if (!ok) ++failed_cases;
            std::cout << (ok ? "  PASS  " : "  FAIL  ") << c.name << '\n';
        }

        std::cout << '\n' << g_checks << " checks, " << g_failures << " failed, "
            << failed_cases << " of " << N << " cases failed\n";

        return g_failures == 0 ? 0 : 1;
    }
} // namespace test

#define CHECK(expr)                                                         \
    do {                                                                    \
        ++test::g_checks;                                                   \
        if (!(expr)) {                                                      \
            ++test::g_failures;                                             \
            std::cout << "      " << __FILE__ << ':' << __LINE__            \
                      << "  CHECK(" #expr ") failed\n";                     \
        }                                                                   \
    } while (0)

#define REQUIRE(expr)                                                       \
    do {                                                                    \
        ++test::g_checks;                                                   \
        if (!(expr)) {                                                      \
            ++test::g_failures;                                             \
            std::cout << "      " << __FILE__ << ':' << __LINE__            \
                      << "  REQUIRE(" #expr ") failed, case aborted\n";     \
            return;                                                         \
        }                                                                   \
    } while (0)
