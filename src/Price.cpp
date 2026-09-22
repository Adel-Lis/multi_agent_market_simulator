//
// Created by Adel Lis on 16/09/2026.
//

#include "Price.hpp"
#include <cmath>

namespace cda {
    Tick to_ticks(double price) {
        return std::llround(price / kTickSize);
    }

    double to_price(Tick ticks) {
       return static_cast<double>(ticks) * kTickSize;
    }
}
