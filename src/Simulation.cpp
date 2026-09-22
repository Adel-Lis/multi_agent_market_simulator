//
// Adel Lis created Simulation on 22/09/2026.
//

#include "Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <stdexcept>

namespace cda
{
    namespace
    {
        /// Write an optional tick price, or an empty field if there is no value.
        void write_optional_price(std::ostream& os, const std::optional<Tick>& v)
        {
            if (v) os << to_price(*v);
        }
    }

    Simulation::Simulation(Config cfg) : cfg_(cfg), rng_(cfg_.seed), log_prices_(cfg.max_memory + 1),
                                         price_(cfg.fundamental_price)
    {
        if (cfg.n_agents == 0)
        {
            throw std::invalid_argument("Simulation::Simulation(): n_agents is 0. It must be positive");
        }
        if (cfg.min_memory > cfg.max_memory)
        {
            throw std::invalid_argument("Simulation::Simulation(): min_memory exceeds max_memory");
        }

        agents_.reserve(cfg.n_agents);
        for (std::uint32_t i = 0; i < cfg_.n_agents; ++i)
        {
            agents_.emplace_back(i, cfg_, rng_);
        }

        const double log_p0 = std::log(cfg_.fundamental_price);
        for (std::size_t i = 0; i < log_prices_.capacity(); ++i)
        {
            log_prices_.push(log_p0);
        }

        stats_.min_price = price_;
        stats_.max_price = price_;

        seed_book();
    }

    void Simulation::seed_book()
    {
        std::uniform_real_distribution<double> offset{0.0, cfg_.initial_band};

        for (std::size_t i = 0; i < cfg_.initial_depth; ++i)
        {
            const double d = offset(rng_);

            Order bid{};
            bid.id = next_order_id_++;
            bid.trader_id = 0;
            bid.side = Side::Buy;
            bid.price = bid_to_ticks(cfg_.fundamental_price * (1.0 - d));
            bid.quantity = 1;
            bid.timestamp = 0;
            book_.insert(bid);

            Order ask{};
            ask.id = next_order_id_++;
            ask.trader_id = 0;
            ask.side = Side::Sell;
            ask.price = ask_to_ticks(cfg_.fundamental_price * (1.0 + d));
            ask.quantity = 1;
            ask.timestamp = 0;
            book_.insert(ask);
        }
    }

    void Simulation::expire_orders(std::uint64_t t)
    {
        if (t <= cfg_.order_lifetime) return;
        const std::uint64_t cutoff = t - cfg_.order_lifetime;

        while (!pending_expiry_.empty() && pending_expiry_.front().first <= cutoff)
        {
            const std::uint64_t order_id = pending_expiry_.front().second;
            pending_expiry_.pop_front();

            if (book_.cancel(order_id)) ++stats_.expiries;
        }
    }

    void Simulation::step(std::uint64_t t, std::ostream& trades_csv, std::ostream& quotes_csv)
    {
        expire_orders(t);

        std::uniform_int_distribution<std::size_t> pick{0, agents_.size() - 1};
        Agent& agent = agents_[pick(rng_)];

        const MarketState market{price_, log_prices_, t};

        std::size_t n_trades_this_step = 0;

        if (auto order = agent.decide(market, cfg_, rng_, next_order_id_))
        {
            // --- TEMPORARY DIAGNOSTIC ---
            if (t <= 200)
            {
                const auto bb = book_.best_bid();
                const auto ba = book_.best_ask();
                std::cerr << t
                    << (order->side == Side::Buy ? " BUY  " : " SELL ")
                    << to_price(order->price)
                    << "  p=" << price_
                    << "  bid=" << (bb ? to_price(*bb) : 0.0)
                    << "  ask=" << (ba ? to_price(*ba) : 0.0)
                    << "  n=" << book_.order_count()
                    << '\n';
            }
            // --- END DIAGNOSTIC ---

            ++next_order_id_;
            ++stats_.orders;

            const auto trades = book_.insert(*order);
            pending_expiry_.emplace_back(t, order->id);

            for (const auto& [id, price, quantity, timestamp, buyer_id, seller_id] : trades)
            {
                trades_csv << id << "," << timestamp << "," << to_price(price) << "," << quantity << "," << buyer_id
                    <<
                    "," << seller_id << '\n';
            }

            if (!trades.empty())
            {
                price_ = std::max(to_price(trades.back().price), cfg_.min_price);
                for (const Trade& tr : trades) stats_.volume += tr.quantity;
            }

            n_trades_this_step = trades.size();
            stats_.trades += trades.size();
        }

        log_prices_.push(std::log(price_));
        stats_.min_price = std::min(stats_.min_price, price_);
        stats_.max_price = std::max(stats_.max_price, price_);

        const auto bb = book_.best_bid();
        const auto ba = book_.best_ask();
        if (!bb || !ba) ++stats_.steps_no_quote;

        quotes_csv << t << "," << price_ << ",";
        write_optional_price(quotes_csv, bb);
        quotes_csv << ",";
        write_optional_price(quotes_csv, ba);
        quotes_csv << ",";
        write_optional_price(quotes_csv, book_.spread());
        quotes_csv << "," << (bb ? book_.depth_at(Side::Buy, *bb) : 0) << "," << (
                ba ? book_.depth_at(Side::Sell, *ba) : 0) << "," << book_.order_count() << "," << n_trades_this_step
            <<
            '\n';
    }

    RunStats Simulation::run(const std::string& trades_path, const std::string& quotes_path)
    {
        std::ofstream trades_csv{trades_path};
        if (!trades_csv)
        {
            throw std::runtime_error("Simulation::Simulation(): failed to open trades_csv at " + trades_path);
        }
        std::ofstream quotes_csv{quotes_path};
        if (!quotes_csv)
        {
            throw std::runtime_error("Simulation::Simulation(): failed to open quotes_csv at " + quotes_path);
        }

        trades_csv << std::fixed << std::setprecision(2);
        quotes_csv << std::fixed << std::setprecision(2);

        trades_csv << "trade_id,timestamp,price,quantity,buyer_id,seller_id\n";
        quotes_csv << "timestamp,price,best_bid,best_ask,spread,"
            "bid_depth,ask_depth,n_orders,n_trades\n";

        for (std::uint64_t t = 1; t <= cfg_.n_steps; ++t)
        {
            step(t, trades_csv, quotes_csv);
        }

        stats_.steps = cfg_.n_steps;
        stats_.final_price = price_;
        return stats_;
    }
}
