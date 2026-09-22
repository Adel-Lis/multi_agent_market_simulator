//
// Created by Adel Lis on 19/09/2026.
//

#pragma once

#include <cstdint>
#include "Price.hpp"

namespace cda {

enum class Side : std::uint8_t { Buy, Sell };

constexpr Side opposite(Side s) {
    return s == Side::Buy ? Side::Sell : Side::Buy;
}

struct Order {
    std::uint64_t id = 0;
    std::uint32_t trader_id = 0;
    Side side = Side::Buy;
    Tick price = 0;
    std::uint32_t quantity = 0;
    std::uint64_t timestamp = 0;
};

struct Trade {
    std::uint64_t id = 0;
    Tick price = 0;
    std::uint32_t quantity = 0;
    std::uint64_t timestamp = 0;
    std::uint32_t buyer_id = 0;
    std::uint32_t seller_id = 0;
};

}