//
// Adel Lis created test_orderbook on 19/09/2026.
//

#include "TestFramework.hpp"

#include "Order.hpp"
#include "OrderBook.hpp"
#include "Price.hpp"

#include <cstdint>
#include <stdexcept>

using namespace cda;

namespace
{
    Order make_order(std::uint64_t id, std::uint32_t trader, Side side,
                     double price, std::uint32_t qty, std::uint64_t ts)
    {
        return Order{
            .id = id, .trader_id = trader, .side = side,
            .price = to_ticks(price), .quantity = qty, .timestamp = ts
        };
    }

    /// The six-order book from the Lesson 2 walkthrough.
    OrderBook walkthrough_book()
    {
        OrderBook book;
        book.insert(make_order(1, 101, Side::Sell, 101.00, 5, 1)); // A1
        book.insert(make_order(2, 102, Side::Sell, 100.50, 3, 2)); // A2
        book.insert(make_order(3, 103, Side::Sell, 100.50, 4, 3)); // A3
        book.insert(make_order(4, 104, Side::Buy, 100.00, 2, 4)); // B1
        book.insert(make_order(5, 105, Side::Buy, 100.00, 6, 5)); // B2
        book.insert(make_order(6, 106, Side::Buy, 99.50, 10, 6)); // B3
        return book; // moved out; copying is deleted
    }

    // ---------------------------------------------------------------------------
    // Tests
    // ---------------------------------------------------------------------------

    void test_non_crossing_orders_rest()
    {
        OrderBook book = walkthrough_book();

        CHECK(book.order_count() == 6);
        CHECK(!book.last_trade_price()); // nothing traded
        CHECK(book.best_bid() == to_ticks(100.00));
        CHECK(book.best_ask() == to_ticks(100.50));
        CHECK(book.spread() == to_ticks(0.50));
        CHECK(book.depth_at(Side::Sell, to_ticks(100.50)) == 7);
        CHECK(book.depth_at(Side::Buy, to_ticks(100.00)) == 8);
        CHECK(book.check_invariants());
    }

    void test_buy_sweeps_two_levels()
    {
        OrderBook book = walkthrough_book();
        const auto trades = book.insert(make_order(7, 107, Side::Buy, 101.00, 10, 7));

        REQUIRE(trades.size() == 3);

        CHECK(trades[0].quantity == 3);
        CHECK(trades[0].price == to_ticks(100.50));
        CHECK(trades[0].seller_id == 102); // A2 first: time priority

        CHECK(trades[1].quantity == 4);
        CHECK(trades[1].price == to_ticks(100.50));
        CHECK(trades[1].seller_id == 103);

        CHECK(trades[2].quantity == 3);
        CHECK(trades[2].price == to_ticks(101.00));
        CHECK(trades[2].seller_id == 101); // partial fill of A1

        for (const Trade& t : trades)
            CHECK(t.buyer_id == 107);
        CHECK(trades[0].id == 1 && trades[1].id == 2 && trades[2].id == 3);

        CHECK(book.depth_at(Side::Sell, to_ticks(100.50)) == 0); // level erased
        CHECK(book.depth_at(Side::Sell, to_ticks(101.00)) == 2); // A1 keeps 2
        CHECK(book.best_ask() == to_ticks(101.00));
        CHECK(book.best_bid() == to_ticks(100.00));
        CHECK(book.last_trade_price() == to_ticks(101.00));
        CHECK(book.order_count() == 4);
        CHECK(book.check_invariants());
    }

    void test_sell_fills_then_rests_remainder()
    {
        // The Lesson 2 exercise: SELL 15 @ 99.75
        OrderBook book = walkthrough_book();
        const auto trades = book.insert(make_order(7, 107, Side::Sell, 99.75, 15, 7));

        REQUIRE(trades.size() == 2);
        CHECK(trades[0].quantity == 2);
        CHECK(trades[0].price == to_ticks(100.00));
        CHECK(trades[0].buyer_id == 104); // B1
        CHECK(trades[1].quantity == 6);
        CHECK(trades[1].price == to_ticks(100.00));
        CHECK(trades[1].buyer_id == 105); // B2
        for (const Trade& t : trades)
            CHECK(t.seller_id == 107);

        // 99.50 < 99.75, so matching stops; 7 units rest as a new best ask.
        CHECK(book.depth_at(Side::Buy, to_ticks(99.50)) == 10);
        CHECK(book.depth_at(Side::Sell, to_ticks(99.75)) == 7);
        CHECK(book.best_ask() == to_ticks(99.75));
        CHECK(book.best_bid() == to_ticks(99.50));
        CHECK(book.spread() == to_ticks(0.25));
        CHECK(book.order_count() == 5);
        CHECK(book.check_invariants());
    }

    void test_order_inside_spread_rests()
    {
        // The Lesson 2 bonus: BUY 4 @ 100.25 doesn't reach 100.50.
        OrderBook book = walkthrough_book();
        const auto trades = book.insert(make_order(7, 107, Side::Buy, 100.25, 4, 7));

        CHECK(trades.empty());
        CHECK(book.best_bid() == to_ticks(100.25)); // new best bid
        CHECK(book.spread() == to_ticks(0.25)); // spread narrowed
        CHECK(book.order_count() == 7);
        CHECK(book.check_invariants());
    }

    void test_equal_price_crosses()
    {
        OrderBook book = walkthrough_book();
        const auto trades = book.insert(make_order(7, 107, Side::Buy, 100.50, 3, 7));

        REQUIRE(trades.size() == 1);
        CHECK(trades[0].quantity == 3);
        CHECK(trades[0].seller_id == 102);
        CHECK(book.best_ask() == to_ticks(100.50)); // A3 still there
        CHECK(book.depth_at(Side::Sell, to_ticks(100.50)) == 4);
        CHECK(book.order_count() == 5);
        CHECK(book.check_invariants());
    }

    void test_cancel_removes_order_and_empty_level()
    {
        OrderBook book = walkthrough_book();

        CHECK(book.cancel(6)); // B3, alone at 99.50
        CHECK(book.depth_at(Side::Buy, to_ticks(99.50)) == 0);
        CHECK(book.order_count() == 5);
        CHECK(book.check_invariants());

        CHECK(book.cancel(4)); // B1, front of 100.00
        CHECK(book.best_bid() == to_ticks(100.00)); // B2 still there
        CHECK(book.depth_at(Side::Buy, to_ticks(100.00)) == 6);
        CHECK(book.check_invariants());

        CHECK(book.cancel(5)); // last bid: side is now empty
        CHECK(!book.best_bid());
        CHECK(!book.spread());
        CHECK(book.check_invariants());
    }

    void test_cancel_unknown_or_filled_returns_false()
    {
        OrderBook book = walkthrough_book();
        CHECK(!book.cancel(99)); // never existed

        book.insert(make_order(7, 107, Side::Buy, 101.00, 10, 7));
        CHECK(!book.cancel(2)); // fully filled
        CHECK(!book.cancel(3)); // fully filled
        CHECK(book.cancel(1)); // partially filled, still resting
        CHECK(!book.cancel(1)); // already cancelled
        CHECK(book.order_count() == 3);
        CHECK(book.check_invariants());
    }

    void test_cancelled_order_is_never_matched()
    {
        OrderBook book = walkthrough_book();
        CHECK(book.cancel(2)); // remove A2 from the front

        const auto trades = book.insert(make_order(7, 107, Side::Buy, 100.50, 3, 7));
        REQUIRE(trades.size() == 1);
        CHECK(trades[0].seller_id == 103); // A3 now first in line
        CHECK(book.depth_at(Side::Sell, to_ticks(100.50)) == 1);
        CHECK(book.check_invariants());
    }

    void test_zero_quantity_is_ignored()
    {
        OrderBook book = walkthrough_book();
        const auto trades = book.insert(make_order(7, 107, Side::Buy, 101.00, 0, 7));

        CHECK(trades.empty());
        CHECK(book.order_count() == 6);
        CHECK(book.best_ask() == to_ticks(100.50));
        CHECK(book.check_invariants());
    }

    void test_duplicate_id_is_rejected()
    {
        OrderBook book = walkthrough_book();

        bool threw = false;
        try
        {
            book.insert(make_order(1, 999, Side::Buy, 90.00, 1, 8)); // id 1 is resting
        }
        catch (const std::invalid_argument&)
        {
            threw = true;
        }
        CHECK(threw);
        CHECK(book.order_count() == 6); // book unchanged
        CHECK(book.depth_at(Side::Buy, to_ticks(90.00)) == 0);
        CHECK(book.check_invariants());
    }
} // anonymous namespace

int main()
{
    const test::Case cases[] = {
        {"non-crossing orders rest", test_non_crossing_orders_rest},
        {"buy sweeps two levels", test_buy_sweeps_two_levels},
        {"sell fills then rests remainder", test_sell_fills_then_rests_remainder},
        {"order inside spread rests", test_order_inside_spread_rests},
        {"equal price crosses", test_equal_price_crosses},
        {"cancel removes order and level", test_cancel_removes_order_and_empty_level},
        {"cancel unknown or filled is false", test_cancel_unknown_or_filled_returns_false},
        {"cancelled order is never matched", test_cancelled_order_is_never_matched},
        {"zero quantity is ignored", test_zero_quantity_is_ignored},
        {"duplicate id is rejected", test_duplicate_id_is_rejected},
    };
    return test::run(cases);
}
