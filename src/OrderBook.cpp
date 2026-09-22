//
// Adel Lis created OrderBook on 19/09/2026.
//


#include "OrderBook.hpp"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <ostream>
#include <iterator>
#include <stdexcept>

namespace cda {

namespace {
void print_level(std::ostream& os, Tick price, const Level& queue) {
	std::uint64_t total = 0;
	for (const Order& o : queue) total += o.quantity;

	os << std::setw(8) << to_price(price) << "  (" << std::setw(3) << total << ")  ";
	for (const Order& o : queue) {
		os << '[' << o.id << " x" << o.quantity << "] ";
	}
	os << '\n';
}

template <typename Book>
std::uint64_t level_depth(const Book& book, Tick price) {
    const auto it = book.find(price);
    if (it == book.end()) return 0;

    std::uint64_t total = 0;
    for (const Order& o : it->second) total += o.quantity;
    return total;
}

} // anonymus namespace

void OrderBook::print(std::ostream& os) const {
	os << std::fixed << std::setprecision(2);

	os << "--------- ASKS ---------\n";
	for (auto it = asks_.rbegin(); it != asks_.rend(); ++it) {
		print_level(os, it->first, it->second);
	}

	os << "  ~~~~~ spread";
	if (auto sp = spread()) os << ' ' << to_price(*sp);
	os << " ~~~~~\n";

	for (const auto& [price, queue] : bids_) {
		print_level(os, price, queue);
	}
	os << "--------- BIDS ---------\n";
}

template <typename Book>
void OrderBook::match(Order& order, Book& book, std::vector<Trade>& trades) {
    while (order.quantity > 0 && !book.empty()) {
        auto level_it = book.begin();
        const Tick level_price = level_it->first;

        const bool crosses = (order.side == Side::Buy) ? (order.price >= level_price) : (order.price <= level_price);
        if (!crosses) break;

        Level& queue = level_it->second;

        while (order.quantity > 0 && !queue.empty()) {
            Order& resting = queue.front();
            const std::uint32_t fill = std::min(order.quantity, resting.quantity);

            const bool incoming_is_buyer = (order.side == Side::Buy);
            trades.push_back(Trade{
                .id = next_trade_id_++,
                .price = level_price,
                .quantity = fill,
                .timestamp = order.timestamp,
                .buyer_id = incoming_is_buyer ? order.trader_id : resting.trader_id,
                .seller_id = incoming_is_buyer ? resting.trader_id : order.trader_id,
            });

            order.quantity -= fill;
            resting.quantity -= fill;
            last_trade_price_ = level_price;

            if (resting.quantity == 0) {
                index_.erase(resting.id);
                queue.pop_front();
            }
        }

        if (queue.empty()) {
            book.erase(level_it);
        }
    }
}

std::vector<Trade> OrderBook::insert(Order order) {
    std::vector<Trade> trades;
    if (order.quantity == 0) return trades;

    if (index_.contains(order.id)) {
        throw std::invalid_argument("OrderBook::insert: duplicate order id");
    }

    if (order.side == Side::Buy) {
        match(order, asks_, trades);
    } else {
        match(order, bids_, trades);
    }

    if (order.quantity > 0) {
        if (order.side == Side::Buy) rest(order, bids_);
        else rest(order, asks_);
    }
    return trades;
}

std::optional<Tick> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<Tick> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::optional<Tick> OrderBook::spread() const {
    const auto bb = best_bid();
    const auto ba = best_ask();
    if (!bb || !ba) return std::nullopt;
    return *ba - *bb;
}

template <typename Book>
void OrderBook::rest(const Order& order, Book& book) {
    Level& level = book[order.price];
    level.push_back(order);
    index_[order.id] = Location{order.side, order.price, std::prev(level.end())};
}

template <typename Book>
void OrderBook::remove(Book& book, const Location& loc) {
    auto level_it = book.find(loc.price);
    assert(level_it != book.end() && "an indexed order's level must exist");

    Level& level = level_it->second;
    level.erase(loc.it);
    if (level.empty()) {
        book.erase(level_it);
    }
}

bool OrderBook::cancel(std::uint64_t order_id) {
    auto found = index_.find(order_id);
    if (found == index_.end()) return false;

    const Location loc = found->second;
    index_.erase(found);

    if (loc.side == Side::Buy) remove(bids_, loc);
    else remove(asks_, loc);

    return true;
}

std::uint64_t OrderBook::depth_at(Side side, Tick price) const {
    return side == Side::Buy ? level_depth(bids_, price) : level_depth(asks_, price);
}

bool OrderBook::check_invariants() const {
    std::size_t seen = 0;

    // A generic lambda: `auto` parameters make it a template,
    // so the same body checks both the bid map and the ask map.
    auto side_ok = [&](const auto& book, Side side) {
        for (const auto& [price, level] : book) {
            if (level.empty()) return false;

            for (auto it = level.begin(); it != level.end(); ++it) {
                if (it->side != side || it->price != price || it->quantity == 0) {
                    return false;
                }
                const auto found = index_.find(it->id);
                if (found == index_.end()) return false;

                const Location& loc = found->second;
                if (loc.side != side || loc.price != price || loc.it != it) {
                    return false;
                }
                ++seen;
            }
        }
        return true;
    };

    if (!side_ok(bids_, Side::Buy))  return false;
    if (!side_ok(asks_, Side::Sell)) return false;
    if (seen != index_.size())       return false;

    const auto bb = best_bid();
    const auto ba = best_ask();
    if (bb && ba && *bb >= *ba) return false;

    return true;
}

} // namespace cda