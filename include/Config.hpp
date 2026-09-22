//
// Adel Lis created Config on 22/09/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>

namespace cda
{
    struct Config
    {
        std::size_t n_agents = 100;

        double fundamental_price = 100.0; // p_f
        double tau_f = 50.0; // mean-reversion time of the fundamental term
        double sigma_eps = 0.004; // std dev of the noise shock
        double k_max = 0.005; // maximum price shading
        double max_log_move = 0.10; // cap per-order price move
        double min_price = 1.0; // reference price floor

        // Spread of the weight distributions: each trader draws its weights from
        // U(0, sigma) once, at birth. These set the character of the population.
        double sigma_fundamental = 1.0; // spread of g1
        double sigma_chartist = 1.5; // spread of g2
        double sigma_noise = 1.0; // spread of n

        std::size_t min_memory = 5; // smallest L_i
        std::size_t max_memory = 50; // largest L_i

        double min_horizon = 5.0; // smallest tau_i
        double max_horizon = 50.0; // largest tau_i

        std::uint64_t order_lifetime = 20; // steps before an unfilled order expires
        std::uint64_t n_steps = 50'000; // length of the simulation
        std::uint64_t seed = 42;

        std::size_t initial_depth = 20; // resting orders per side at startup
        double initial_band = 0.02; // how far they spread from p_f
    };
}
