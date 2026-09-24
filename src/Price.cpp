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
        constexpr double kMaxPrice = 9.0e16 * kTickSize; // stays inside int64

        bool representable(double price)
        {
            return std::isfinite(price) && price > 0.0 && price < kMaxPrice;
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
