//
// Created by Adel Lis on 16/09/2026.
//

#pragma once
#include <cstdint>

namespace cda {

using Tick = std::int64_t;
inline constexpr double kTickSize = 0.01;

Tick to_ticks(double price);
double to_price(Tick ticks);

}
