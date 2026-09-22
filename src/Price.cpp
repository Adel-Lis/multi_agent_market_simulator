//
// Created by Adel Lis on 16/09/2026.
//

#include "Price.hpp"
#include <cmath>

namespace cda
{
    Tick to_ticks(const double price)
    {
        return std::llround(price / kTickSize);
    }

    double to_price(const Tick ticks)
    {
        return static_cast<double>(ticks) * kTickSize;
    }

    Tick bid_to_ticks(const double price)
    {
        return static_cast<Tick>(std::floor(price / kTickSize));
    }

    Tick ask_to_ticks(const double price)
    {
        return static_cast<Tick>(std::ceil(price / kTickSize));
    }
}
