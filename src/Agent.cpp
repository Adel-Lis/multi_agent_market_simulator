#include "Agent.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace cda
{
    Agent::Agent(std::uint32_t id, const Config& cfg, Rng& rng) : id_(id)
    {
        // g1 ~ |N(0, sigma_1)|: half-normal, so fundamentalist weight is never negative.
        std::normal_distribution<double> g1_dist{0.0, cfg.sigma_fundamental};
        g1_ = std::fabs(g1_dist(rng));

        // g2 ~ N(0, sigma_2): negative draws are contrarians.
        std::normal_distribution<double> g2_dist{0.0, cfg.sigma_chartist};
        g2_ = g2_dist(rng);

        std::uniform_real_distribution<double> k_dist{0.0, cfg.k_max};
        k_ = k_dist(rng);

        std::uniform_int_distribution<std::size_t> mem_dist{cfg.min_memory, cfg.max_memory};
        std::uniform_int_distribution<std::uint32_t> hor_dist{cfg.min_horizon, cfg.max_horizon};

        memory_ = mem_dist(rng);
        horizon_ = hor_dist(rng);
    }

    double Agent::expected_return(const MarketState& market,
                                  const Config& cfg,
                                  double epsilon) const
    {
        const double fundamental =
            std::log(cfg.fundamental_price / market.price) / cfg.tau_f;

        // Mean log return over L steps; the sum telescopes to the endpoints.
        double chartist = 0.0;
        if (market.log_prices.size() > memory_)
        {
            const double now = market.log_prices.ago(0);
            const double then = market.log_prices.ago(memory_);
            chartist = (now - then) / static_cast<double>(memory_);
        }

        // No normalization: the noise weight is 1 (Chiarella & Iori 2002, eq. 1).
        return g1_ * fundamental + g2_ * chartist + epsilon;
    }

    std::optional<Order> Agent::decide(const MarketState& market,
                                       const Config& cfg,
                                       Rng& rng,
                                       std::uint64_t order_id)
    {
        std::normal_distribution<double> eps_dist{0.0, cfg.sigma_eps};

        const double r_hat = expected_return(market, cfg, eps_dist(rng));

        double total = r_hat * static_cast<double>(horizon_);
        if (std::fabs(total) > cfg.max_log_move)
        {
            ++clamp_hits_;
            total = std::clamp(total, -cfg.max_log_move, cfg.max_log_move);
        }

        Order order{};
        order.id = order_id;
        order.trader_id = id_;
        order.quantity = 1;
        order.timestamp = market.timestamp;

        // Shading is exponential: b = p_t exp(r*tau - k), a = p_t exp(r*tau + k).
        if (r_hat > 0.0)
        {
            order.side = Side::Buy;
            order.price = bid_to_ticks(market.price * std::exp(total - k_));
        }
        else if (r_hat < 0.0)
        {
            order.side = Side::Sell;
            order.price = ask_to_ticks(market.price * std::exp(total + k_));
        }
        else
        {
            return std::nullopt;
        }

        if (order.price <= 0) return std::nullopt;

        return order;
    }
} // namespace cda
