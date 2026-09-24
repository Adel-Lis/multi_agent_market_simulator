//
// Adel Lis created Config on 22/09/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <limits>

namespace cda
{
    /// Market-wide parameters
    /// Baseline values are from Chiarella & Iori (2002), except the two time
    /// scales. As printed, the paper's tau_max = 100 with sigma_eps = 0.05 gives
    /// sigma_eps * tau_max = 5, so a one-sigma noise draw prices an order at
    /// e^5 ~ 148x the market and the model is explosive from its first order.
    /// Reducing sigma_eps does NOT repair this, because tau_i also multiplies the
    /// chartist term; reducing tau_max does. See analysis/NOTES.md.
    struct Config
    {
        std::size_t n_agents = 100; // N (paper also uses 1000)

        double fundamental_price = 100.0; // p_f
        double tau_f = 10.0; // paper: 100; rescaled for mean reversion
        double sigma_eps = 0.05; // sigma_epsilon (paper value)

        double k_max = 0.05; // k_i ~ U[0, k_max] (paper value)

        double sigma_fundamental = 1.0; // sigma_1: g1 ~ |N(0, s1)| (paper value)
        double sigma_chartist = 1.0; // sigma_2: g2 ~ N(0, s2)  (paper value)

        std::size_t min_memory = 1; // L_i ~ discrete U[1, 100] (paper value)
        std::size_t max_memory = 100;

        std::uint32_t min_horizon = 1; // tau_i ~ discrete U[1, tau_max]
        std::uint32_t max_horizon = 2; // paper: 100; the unstable parameter

        std::uint64_t order_lifetime = 100; // paper value
        std::uint64_t n_steps = 20'000; // paper uses 20k to 100k
        std::uint64_t seed = 42;

        // Not in the paper. A very large default means "effectively off"; the run
        // summary reports how often it binds, so an explosion is visible rather
        // than silently truncated. It binds 0% of the time at these settings.
        double max_log_move = 100.0;
        double min_price = 0.01;

        // Not in the paper: the book starts empty there, and does so here too.
        std::size_t initial_depth = 0;
        double initial_band = 0.02;

        // @throws std::invalid_argument if any parameter is unusable
        void validate() const;
    };
}
