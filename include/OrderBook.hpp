//
// Adel Lis created OrderBook on 19/09/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <vector>
#include <ostream>
#include <unordered_map>

#include "Order.hpp"
#include "Price.hpp"

namespace cda {

    /// All resting orders at a single price, oldest first.
    using Level = std::list<Order>;

    /// A continuous double auction book with price-time priority.
    class OrderBook {
    public:
        OrderBook() = default;

        // A copy would duplicate the lists but leave index_ pointing into the
        // original's nodes. Moving transfers the nodes, so iterators stay valid.
        OrderBook(const OrderBook&) = delete;
        OrderBook& operator=(const OrderBook&) = delete;
        OrderBook(OrderBook&&) = default;
        OrderBook& operator=(OrderBook&&) = default;

        /// Insert an order, matching it against the opposite side first.
        /// Any unfilled remainder rests at its limit price.
        /// @return the trades generated, oldest first
        std::vector<Trade> insert(Order order);

        /// Remove a resting order. Returns false if it isn't in the book
        /// (already filled, already cancelled, or never rested).
        bool cancel(std::uint64_t order_id);

        /// Highest price any buyer is willing to pay, if the bid side is non-empty.
        std::optional<Tick> best_bid() const;

        /// Lowest price any seller will accept, if the ask side is non-empty.
        std::optional<Tick> best_ask() const;

        /// Best ask minus best bid, if both sides are non-empty.
        std::optional<Tick> spread() const;

        /// Price of the most recent trade, if any trade has occurred.
        std::optional<Tick> last_trade_price() const { return last_trade_price_; }

        /// Total quantity resting at one price on one side (0 if no such level)
        std::uint64_t depth_at(Side side, Tick price) const;

        /// Number of orders currently resting on both sides
        std::size_t order_count() const { return index_.size(); }

        /// Verify every structural rule of the book. O(n): for tests and debugging
        bool check_invariants() const;

        void print(std::ostream& os) const;

    private:
        using BidBook = std::map<Tick, Level, std::greater<Tick>>;
        using AskBook = std::map<Tick, Level, std::less<Tick>>;

        /// Where a resting order lives, so it can be removed without searching
        struct Location {
            Side side;
            Tick price;
            Level::iterator it;
        };

        /// Match `order` against `book` while prices cross, appending to `trades`.
        /// Reduces order.quantity by the amount filled.
        template <typename Book>
        void match(Order& order, Book& book, std::vector<Trade>& trades);

        template <typename Book>
        void rest(const Order& order, Book& book);

        template <typename Book>
        void remove(Book& book, const Location& loc);

        BidBook bids_;
        AskBook asks_;
        std::unordered_map<std::uint64_t, Location> index_;

        std::uint64_t next_trade_id_ = 1;
        std::optional<Tick> last_trade_price_;
    };
} // namespace cda