//
// Adel Lis created test_agent on 22/09/2026.
//

#include "TestFramework.hpp"

#include "Agent.hpp"
#include "Config.hpp"
#include "MarketState.hpp"
#include "Price.hpp"
#include "Random.hpp"
#include "RingBuffer.hpp"

#include <cmath>
#include <vector>

using namespace cda;

namespace
{
    /// Floating-point comparison: never use == on doubles.
    bool close(double a, double b, double tol = 1e-9)
    {
        return std::fabs(a - b) < tol;
    }

    /// A history of `n` copies of ln(price): a perfectly flat market.
    RingBuffer<double> flat_history(double price, std::size_t n)
    {
        RingBuffer<double> h{n};
        for (std::size_t i = 0; i < n; ++i) h.push(std::log(price));
        return h;
    }

    // ---------------------------------------------------------------- ring buffer

    void test_ring_buffer_basic()
    {
        RingBuffer<double> rb{3};

        CHECK(rb.empty());
        CHECK(rb.capacity() == 3);

        rb.push(10.0);
        rb.push(20.0);
        CHECK(rb.size() == 2);
        CHECK(!rb.full());
        CHECK(close(rb.ago(0), 20.0));
        CHECK(close(rb.ago(1), 10.0));

        rb.push(30.0);
        CHECK(rb.full());
        CHECK(close(rb.ago(0), 30.0));
        CHECK(close(rb.ago(2), 10.0));
    }

    void test_ring_buffer_overwrites_oldest()
    {
        RingBuffer<int> rb{3};
        for (int i = 1; i <= 5; ++i) rb.push(i); // 1,2,3 overwritten by 4,5

        CHECK(rb.size() == 3);
        CHECK(rb.ago(0) == 5);
        CHECK(rb.ago(1) == 4);
        CHECK(rb.ago(2) == 3);
    }

    void test_ring_buffer_survives_many_wraps()
    {
        RingBuffer<int> rb{7};
        for (int i = 0; i < 1000; ++i) rb.push(i);

        CHECK(rb.size() == 7);
        for (std::size_t lag = 0; lag < 7; ++lag)
        {
            CHECK(rb.ago(lag) == 999 - static_cast<int>(lag));
        }
    }

    // --------------------------------------------------------------------- agent

    void test_agent_parameters_are_in_range()
    {
        Config cfg;
        Rng rng{1};

        for (std::uint32_t i = 0; i < 200; ++i)
        {
            Agent a{i, cfg, rng};
            CHECK(a.id() == i);
            CHECK(a.weight_fundamental() >= 0.0 && a.weight_fundamental() <= cfg.sigma_fundamental);
            CHECK(a.weight_chartist() >= 0.0 && a.weight_chartist() <= cfg.sigma_chartist);
            CHECK(a.weight_noise() >= 0.0 && a.weight_noise() <= cfg.sigma_noise);
            CHECK(a.memory() >= cfg.min_memory && a.memory() <= cfg.max_memory);
            CHECK(a.horizon() >= cfg.min_horizon && a.horizon() <= cfg.max_horizon);
        }
    }

    void test_population_is_heterogeneous()
    {
        Config cfg;
        Rng rng{7};

        std::vector<Agent> agents;
        for (std::uint32_t i = 0; i < 100; ++i) agents.emplace_back(i, cfg, rng);

        // If the engine were passed by value somewhere, every agent would be identical.
        int distinct = 0;
        for (std::size_t i = 1; i < agents.size(); ++i)
        {
            if (!close(agents[i].weight_fundamental(), agents[0].weight_fundamental()))
            {
                ++distinct;
            }
        }
        CHECK(distinct > 90);
    }

    void test_same_seed_reproduces_population()
    {
        Config cfg;
        Rng rng_a{123};
        Rng rng_b{123};

        for (std::uint32_t i = 0; i < 50; ++i)
        {
            Agent a{i, cfg, rng_a};
            Agent b{i, cfg, rng_b};
            CHECK(close(a.weight_fundamental(), b.weight_fundamental()));
            CHECK(close(a.weight_chartist(), b.weight_chartist()));
            CHECK(a.memory() == b.memory());
        }
    }

    void test_pure_fundamentalist_expected_return()
    {
        // The worked example from part 1, as a test.
        Config cfg;
        cfg.fundamental_price = 102.0;
        cfg.tau_f = 50.0;
        cfg.sigma_fundamental = 1.0;
        cfg.sigma_chartist = 0.0; // force a pure fundamentalist
        cfg.sigma_noise = 0.0;

        Rng rng{5};
        Agent a{0, cfg, rng};
        REQUIRE(a.weight_chartist() == 0.0);
        REQUIRE(a.weight_noise() == 0.0);

        const auto history = flat_history(100.0, 60);
        const MarketState market{100.0, history, 1};

        const double expected = std::log(102.0 / 100.0) / 50.0;
        CHECK(close(a.expected_return(market, cfg, 0.0), expected));
    }

    void test_fundamentalist_leans_against_the_price()
    {
        Config cfg;
        cfg.fundamental_price = 100.0;
        cfg.sigma_chartist = 0.0;
        cfg.sigma_noise = 0.0;

        Rng rng{11};
        Agent a{0, cfg, rng};

        const auto history = flat_history(100.0, 60);

        // Price below the fundamental: the asset is cheap, so buy.
        const MarketState cheap{95.0, history, 1};
        CHECK(a.expected_return(cheap, cfg, 0.0) > 0.0);

        const auto order_cheap = a.decide(cheap, cfg, rng, 1);
        REQUIRE(order_cheap.has_value());
        CHECK(order_cheap->side == Side::Buy);

        // Price above the fundamental: the asset is dear, so sell.
        const MarketState dear{105.0, history, 2};
        CHECK(a.expected_return(dear, cfg, 0.0) < 0.0);

        const auto order_dear = a.decide(dear, cfg, rng, 2);
        REQUIRE(order_dear.has_value());
        CHECK(order_dear->side == Side::Sell);
    }

    void test_chartist_follows_the_trend()
    {
        Config cfg;
        cfg.fundamental_price = 100.0;
        cfg.sigma_fundamental = 0.0; // pure chartist
        cfg.sigma_chartist = 1.0;
        cfg.sigma_noise = 0.0;
        cfg.min_memory = 10;
        cfg.max_memory = 10;

        Rng rng{3};
        Agent a{0, cfg, rng};
        REQUIRE(a.memory() == 10);

        // A steadily rising market: each step is +1%.
        RingBuffer<double> rising{20};
        double p = 100.0;
        for (int i = 0; i < 20; ++i)
        {
            rising.push(std::log(p));
            p *= 1.01;
        }

        const MarketState market{std::exp(rising.ago(0)), rising, 20};

        // The average log return over 10 steps is ln(1.01).
        CHECK(close(a.expected_return(market, cfg, 0.0), std::log(1.01), 1e-9));
        CHECK(a.expected_return(market, cfg, 0.0) > 0.0); // extrapolates upward

        const auto order = a.decide(market, cfg, rng, 1);
        REQUIRE(order.has_value());
        CHECK(order->side == Side::Buy);
    }

    void test_chartist_is_silent_without_enough_history()
    {
        Config cfg;
        cfg.sigma_fundamental = 0.0;
        cfg.sigma_chartist = 1.0;
        cfg.sigma_noise = 0.0;
        cfg.min_memory = 30;
        cfg.max_memory = 30;

        Rng rng{4};
        Agent a{0, cfg, rng};

        RingBuffer<double> shallow{50};
        for (int i = 0; i < 10; ++i) shallow.push(std::log(100.0 + i)); // only 10 values

        const MarketState market{109.0, shallow, 10};
        CHECK(close(a.expected_return(market, cfg, 0.0), 0.0));
    }

    void test_order_is_shaded_in_the_right_direction()
    {
        Config cfg;
        cfg.fundamental_price = 110.0;
        cfg.sigma_chartist = 0.0;
        cfg.sigma_noise = 0.0;
        cfg.k_max = 0.05;

        Rng rng{9};
        Agent a{0, cfg, rng};

        const auto history = flat_history(100.0, 60);
        const MarketState market{100.0, history, 1};

        for (std::uint64_t i = 0; i < 100; ++i)
        {
            const auto order = a.decide(market, cfg, rng, i);
            REQUIRE(order.has_value());
            REQUIRE(order->side == Side::Buy);

            // A buyer bids below its own valuation, and its valuation is above p_t.
            const double r_hat = a.expected_return(market, cfg, 0.0);
            const double valuation = market.price * std::exp(r_hat * a.horizon());
            CHECK(to_price(order->price) <= valuation);
            CHECK(order->quantity == 1);
            CHECK(order->trader_id == 0);
            CHECK(order->price > 0);
        }
    }

    void test_bid_and_ask_rounding_are_conservative()
    {
        // 100.376 sits between the ticks 100.37 and 100.38.
        CHECK(bid_to_ticks(100.376) == 10037);
        CHECK(ask_to_ticks(100.376) == 10038);
        CHECK(bid_to_ticks(100.374) == 10037);
        CHECK(ask_to_ticks(100.374) == 10038);
    }
} // anonymous namespace

int main()
{
    const test::Case cases[] = {
        {"ring buffer basics", test_ring_buffer_basic},
        {"ring buffer overwrites oldest", test_ring_buffer_overwrites_oldest},
        {"ring buffer survives many wraps", test_ring_buffer_survives_many_wraps},
        {"agent parameters are in range", test_agent_parameters_are_in_range},
        {"population is heterogeneous", test_population_is_heterogeneous},
        {"same seed reproduces population", test_same_seed_reproduces_population},
        {"pure fundamentalist expected return", test_pure_fundamentalist_expected_return},
        {"fundamentalist leans against price", test_fundamentalist_leans_against_the_price},
        {"chartist follows the trend", test_chartist_follows_the_trend},
        {"chartist silent without history", test_chartist_is_silent_without_enough_history},
        {"order shaded in right direction", test_order_is_shaded_in_the_right_direction},
        {"tick rounding is conservative", test_bid_and_ask_rounding_are_conservative},
    };
    return test::run(cases);
}
