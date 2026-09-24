//
// Created by Adel Lis on 16/09/2026.
//

#include "Config.hpp"
#include "Simulation.hpp"

#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace cda;

namespace
{
    void print_usage()
    {
        std::cerr <<
            "usage: sim [options]\n"
            "  --steps N            simulation length\n"
            "  --agents N           population size\n"
            "  --seed N             random seed\n"
            "  --sigma-eps X        std dev of the noise shock\n"
            "  --sigma-chartist X   upper bound of the chartist weight\n"
            "  --sigma-fund X       upper bound of the fundamentalist weight\n"
            "  --kmax X             maximum price shading\n"
            "  --tau-f X            fundamental mean-reversion time\n"
            "  --lifetime N         steps before an unfilled order expires\n"
            "  --max-move X         cap on the per-order log price move\n"
            "  --out TAG            write out/trades_TAG.csv and out/quotes_TAG.csv\n";
    }

    std::uint64_t parse_u64(const std::string& s)
    {
        if (!s.empty() && s[0] == '-')
        {
            throw std::invalid_argument("negative value");
        }
        return std::stoull(s);
    }
} // anonymous namespace

int main(int argc, char* argv[])
{
    Config cfg;
    std::string trades_path = "out/trades.csv";
    std::string quotes_path = "out/quotes.csv";

    // Each option takes exactly one value, so walk the arguments in pairs.
    for (int i = 1; i < argc; i += 2)
    {
        const std::string key = argv[i];

        if (i + 1 >= argc)
        {
            std::cerr << "error: " << key << " needs a value\n\n";
            print_usage();
            return 1;
        }
        const std::string value = argv[i + 1];

        try
        {
            if (key == "--out")
            {
                trades_path = "out/trades_" + value + ".csv";
                quotes_path = "out/quotes_" + value + ".csv";
            }
            else if (key == "--steps") cfg.n_steps = parse_u64(value);
            else if (key == "--agents") cfg.n_agents = parse_u64(value);
            else if (key == "--seed") cfg.seed = parse_u64(value);
            else if (key == "--lifetime") cfg.order_lifetime = parse_u64(value);
            else if (key == "--sigma-eps") cfg.sigma_eps = std::stod(value);
            else if (key == "--sigma-chartist") cfg.sigma_chartist = std::stod(value);
            else if (key == "--sigma-fund") cfg.sigma_fundamental = std::stod(value);
            else if (key == "--kmax") cfg.k_max = std::stod(value);
            else if (key == "--tau-f") cfg.tau_f = std::stod(value);
            else if (key == "--max-move") cfg.max_log_move = std::stod(value);
            else if (key == "--tau-max") cfg.max_horizon = static_cast<std::uint32_t>(parse_u64(value));
            else
            {
                std::cerr << "error: unknown option " << key << "\n\n";
                print_usage();
                return 1;
            }
        }
        catch (const std::invalid_argument&)
        {
            std::cerr << "error: " << key << " expects a number, got '" << value << "'\n";
            return 1;
        }
        catch (const std::out_of_range&)
        {
            std::cerr << "error: value out of range for " << key << ": " << value << '\n';
            return 1;
        }
    }

    std::cout << "running " << cfg.n_steps << " steps with " << cfg.n_agents
        << " agents, seed " << cfg.seed
        << "\n  sigma_eps " << cfg.sigma_eps
        << "  sigma_chartist " << cfg.sigma_chartist
        << "  k_max " << cfg.k_max
        << "  lifetime " << cfg.order_lifetime << '\n';

    try
    {
        Simulation sim{cfg};
        const RunStats s = sim.run(trades_path, quotes_path);

        const double orders = s.orders ? static_cast<double>(s.orders) : 1.0;

        std::cout << std::fixed << std::setprecision(2)
            << "\norders submitted  " << s.orders
            << "\ntrades executed   " << s.trades
            << " (" << (100.0 * s.trades / orders) << "% of orders)"
            << "\nvolume traded     " << s.volume
            << "\norders rested     " << s.passive
            << "\norders expired    " << s.expiries
            << "\nclamp bound on    " << s.clamped
            << " (" << (100.0 * s.clamped / orders) << "%)"
            << "\nsteps w/o 2 sides " << s.steps_no_quote
            << " (" << (100.0 * s.steps_no_quote / s.steps) << "%)"
            << "\nprice range       " << s.min_price << " .. " << s.max_price
            << "\nfinal price       " << s.final_price
            << "\nfundamental       " << cfg.fundamental_price
            << "\n\nwrote " << trades_path << " and " << quotes_path << '\n';
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
