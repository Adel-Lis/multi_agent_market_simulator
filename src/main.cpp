//
// Created by Adel Lis on 16/09/2026.
//

#include "Config.hpp"
#include "Simulation.hpp"

#include <exception>
#include <iomanip>
#include <iostream>

using namespace cda;

int main()
{
    Config cfg;

    std::cout << "running " << cfg.n_steps << " steps with " << cfg.n_agents << " agents, seed " << cfg.seed <<
        std::endl;

    try
    {
        Simulation sim{cfg};
        const RunStats s = sim.run("out/trades.csv", "out/quotes.csv");

        std::cout << std::fixed << std::setprecision(2)
            << "\norders submitted  " << s.orders
            << "\ntrades executed   " << s.trades
            << "\nvolume traded     " << s.volume
            << "\norders expired    " << s.expiries
            << "\nsteps w/o 2 sides " << s.steps_no_quote
            << "\nprice range       " << s.min_price << " .. " << s.max_price
            << "\nfinal price       " << s.final_price
            << "\nfundamental       " << cfg.fundamental_price
            << "\n\nwrote out/trades.csv and out/quotes.csv\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }
}
