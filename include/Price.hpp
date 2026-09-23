//
// Created by Adel Lis on 16/09/2026.
//

#pragma once
#include <cstdint>

namespace cda
{
    using Tick = std::int64_t;
    inline constexpr double kTickSize = 0.01;

    Tick to_ticks(double price);
    double to_price(Tick ticks);

    /// Round a bid price DOWN to the tick grid: never bid more than intended.
    Tick bid_to_ticks(double price);

    /// Round an ask price UP to the tick grid: never ask less than intended.
    Tick ask_to_ticks(double price);
}
