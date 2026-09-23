//
// Adel Lis created Agent on 22/09/2026.
//

#pragma once

#include <cstdint>
#include <optional>
#include <cstddef>

#include "Config.hpp"
#include "MarketState.hpp"
#include "Order.hpp"
#include "Random.hpp"

namespace cda
{
    class Agent
    {
    public:
        Agent(std::uint32_t id, const Config& cfg, Rng& rng);

        std::optional<Order> decide(const MarketState& market, const Config& cfg, Rng& rng,
                                    std::uint64_t order_id);

        double expected_return(const MarketState& market, const Config& cfg, double epsilon) const;

        std::uint32_t id() const { return id_; }
        double weight_fundamental() const { return g1_; }
        double weight_chartist() const { return g2_; }
        std::size_t memory() const { return memory_; }
        std::uint32_t horizon() const { return horizon_; }
        double shading() const { return k_; }
        std::uint64_t clamp_hits() const { return clamp_hits_; }

    private:
        std::uint32_t id_ = 0;

        double g1_ = 0.0;
        double g2_ = 0.0;
        double k_ = 0.0;

        std::size_t memory_ = 1;
        std::uint32_t horizon_ = 1;

        std::uint64_t clamp_hits_ = 0;
    };
}
