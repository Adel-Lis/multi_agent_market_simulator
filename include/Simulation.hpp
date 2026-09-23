//
// Adel Lis created Simulation on 22/09/2026.
//

#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "Agent.hpp"
#include "Config.hpp"
#include "OrderBook.hpp"
#include "Random.hpp"
#include "RingBuffer.hpp"

namespace cda
{
    struct RunStats
    {
        std::uint64_t steps = 0;
        std::uint64_t orders = 0;
        std::uint64_t trades = 0;
        std::uint64_t volume = 0;
        std::uint64_t expiries = 0;
        std::uint64_t steps_no_quote = 0;
        double final_price = 0.0;
        double min_price = 0.0;
        double max_price = 0.0;
        std::uint64_t clamped = 0;
        std::uint64_t passive = 0;
    };

    class Simulation
    {
    public:
        explicit Simulation(Config cfg);

        /// Run the full simulation, writing CSV as it goes.
        /// @param trades_path one row per trade
        /// @param quotes_path one row per step
        RunStats run(const std::string& trades_path, const std::string& quotes_path);

    private:
        void step(std::uint64_t t, std::ostream& trades_csv, std::ostream& quotes_csv);
        void expire_orders(std::uint64_t t);
        void seed_book();
        double reference_price() const;

        Config cfg_;
        Rng rng_;
        OrderBook book_;
        std::vector<Agent> agents_;
        RingBuffer<double> log_prices_;

        /// Orders that wait for expiration
        std::deque<std::pair<std::uint64_t, std::uint64_t>> pending_expiry_;

        double price_ = 0.0;
        std::uint64_t next_order_id_ = 1;
        RunStats stats_;
    };
}
