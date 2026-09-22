//
// Created by Adel Lis on 16/09/2026.
//

#include "Order.hpp"
#include "OrderBook.hpp"
#include "Price.hpp"

#include <iomanip>
#include <iostream>
#include <string_view>

using namespace cda;

namespace {

// Small helper so the test script below stays readable.
Order make_order(std::uint64_t id, std::uint32_t trader, Side side,
                 double price, std::uint32_t qty, std::uint64_t ts) {
    return Order{
        .id        = id,
        .trader_id = trader,
        .side      = side,
        .price     = to_ticks(price),
        .quantity  = qty,
        .timestamp = ts
    };
}

void print_quotes(const OrderBook& book, std::string_view label) {
    std::cout << label << "  ";

    if (auto bb = book.best_bid()) std::cout << "bid " << to_price(*bb);
    else                           std::cout << "bid   --  ";

    if (auto ba = book.best_ask()) std::cout << "   ask " << to_price(*ba);
    else                           std::cout << "   ask   --  ";

    if (auto sp = book.spread())   std::cout << "   spread " << to_price(*sp);

    if (auto lp = book.last_trade_price()) std::cout << "   last " << to_price(*lp);

    std::cout << '\n';
}

} // anonymous namespace

int main() {
    std::cout << std::fixed << std::setprecision(2);

    OrderBook book;

    // The resting book from the walkthrough. Timestamps are the arrival order,
    // which is what gives A2 priority over A3 at the same price.
    book.insert(make_order(1, 101, Side::Sell, 101.00,  5, 1));   // A1
    book.insert(make_order(2, 102, Side::Sell, 100.50,  3, 2));   // A2
    book.insert(make_order(3, 103, Side::Sell, 100.50,  4, 3));   // A3
    book.insert(make_order(4, 104, Side::Buy,  100.00,  2, 4));   // B1
    book.insert(make_order(5, 105, Side::Buy,  100.00,  6, 5));   // B2
    book.insert(make_order(6, 106, Side::Buy,   99.50, 10, 6));   // B3

    print_quotes(book, "before:");

    // The taker: BUY 10 @ 101.00
    const auto trades = book.insert(make_order(7, 107, Side::Buy, 101.00, 10, 7));

    std::cout << "\ntrades (" << trades.size() << "):\n";
    for (const Trade& t : trades) {
        std::cout << "  #" << t.id
                  << "  " << t.quantity << " @ " << to_price(t.price)
                  << "   buyer " << t.buyer_id
                  << "  seller " << t.seller_id << '\n';
    }

    std::cout << '\n';
    print_quotes(book, "after: ");

    std::cout << '\n';
    book.print(std::cout);
    std::cout << "resting orders: " << book.order_count() << '\n';

    // A1 (order 1) partially filled earlier and still rests with 2 units.
    std::cout << "\ncancel order 1 -> " << std::boolalpha << book.cancel(1) << '\n';
    std::cout << "cancel order 1 again -> " << book.cancel(1) << '\n';
    std::cout << "cancel order 2 (filled) -> " << book.cancel(2) << "\n\n";

    book.print(std::cout);
    std::cout << "resting orders: " << book.order_count() << '\n';
    print_quotes(book, "final: ");
}