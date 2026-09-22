//
// Adel Lis created MarketState on 22/09/2026.
//

#pragma once

#include <cstdint>

#include "RingBuffer.hpp"

namespace cda
{
    // What a trader agent is allowed to see when deciding
    struct MarketState
    {
        double price; // p_t, the current reference price
        const RingBuffer<double>& log_prices; // ln p, one entry per step, ago(0) is now
        std::uint64_t timestamp; // the current steps
    };
}
