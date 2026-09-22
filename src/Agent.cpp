//
// Adel Lis created Agent on 22/09/2026.
//

#include "Agent.hpp"

#include <cmath>
#include <random>

namespace cda
{
    Agent::Agent(std::uint32_t id, const Config& cfg, Rng& rng) : id_(id)
    {
        std::uniform_real_distribution<double> g1_dist{0.0, cfg.sigma_fundamental};
        std::uniform_real_distribution<double> g2_dist{0.0, cfg.sigma_chartist};
        std::uniform_real_distribution<double> n_dist{0.0, cfg.sigma_noise};

        std::uniform_int_distribution<std::size_t> mem_dist{cfg.min_memory, cfg.max_memory};

        std::uniform_real_distribution<double> hor_dist{cfg.min_horizon, cfg.max_horizon};

        g1_ = g1_dist(rng);
        g2_ = g2_dist(rng);
        n_ = n_dist(rng);

        const double sum = g1_ + g2_ + n_;
        inv_sum_ = (sum > 0.0) ? 1.0 / sum : 0.0;

        memory_ = mem_dist(rng);
        horizon_ = hor_dist(rng);
    }

    double Agent::expected_return(const MarketState& market, const Config& cfg, double epsilon) const
    {
        const double fundamental = std::log(cfg.fundamental_price / market.price) / cfg.tau_f;

        double chartist = 0.0;
        if (market.log_prices.size() > memory_)
        {
            const double now = market.log_prices.ago(0);
            const double then = market.log_prices.ago(memory_);
            chartist = (now - then) / static_cast<double>(memory_);
        }

        return inv_sum_ * (g1_ * fundamental + g2_ * chartist + n_ * epsilon);
    }

    std::optional<Order> Agent::decide(const MarketState& market, const Config& cfg, Rng& rng, std::uint64_t order_id)
    {
        std::normal_distribution<double> eps_dist{0.0, cfg.sigma_eps};
        std::uniform_real_distribution<double> k_dist{0.0, cfg.k_max};

        const double r_hat = expected_return(market, cfg, eps_dist(rng));

        const double expected_price = market.price * std::exp(r_hat * horizon_);
        const double k = k_dist(rng);

        Order order{};
        order.id = order_id;
        order.trader_id = id_;
        order.quantity = 1;
        order.timestamp = market.timestamp;

        if (expected_price > market.price)
        {
            order.side = Side::Buy;
            order.price = bid_to_ticks(expected_price * (1.0 - k));
        }
        else if (expected_price < market.price)
        {
            order.side = Side::Sell;
            order.price = ask_to_ticks(expected_price * (1.0 + k));
        }
        else
        {
            return std::nullopt;
        }

        if (order.price <= 0) return std::nullopt;

        return order;
    }
}
