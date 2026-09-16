//
// Created by Adel Lis on 16/09/2026.
//

#include "Price.hpp"
#include <iostream>

int main() {
    cda::Tick t = cda::to_ticks(100.37);
    std::cout << t << " ticks = " << cda::to_price(t) << '\n';
}