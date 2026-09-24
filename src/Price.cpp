//
// Created by Adel Lis on 16/09/2026.
//

#include "Price.hpp"
#include <cmath>
#include <limits>

namespace cda
{
    double to_price(const Tick ticks)
    {
        return static_cast<double>(ticks) * kTickSize;
    }

    namespace
    {
        /// Largest price whose tick count fits in Tick (int64) with headroom.
        /// Tick is int64, so the limit is ~9.22e18 ticks; at kTickSize = 0.01
        /// that is a price of ~9.2e16. We stop an order of magnitude short.
        constexpr double kMaxRepresentablePrice = 9.0e15;

        bool representable(double price)
        {
            return std::isfinite(price) && price > 0.0 && price < kMaxRepresentablePrice;
        }
    }

    Tick to_ticks(double price)
    {
        if (!representable(price)) return 0;
        return std::llround(price / kTickSize);
    }

    Tick bid_to_ticks(double price)
    {
        if (!representable(price)) return 0;
        return static_cast<Tick>(std::floor(price / kTickSize));
    }

    Tick ask_to_ticks(double price)
    {
        if (!representable(price)) return 0;
        return static_cast<Tick>(std::ceil(price / kTickSize));
    }
}
