//
// Adel Lis created Config on 24/09/2026.
//

#include "Config.hpp"
#include <stdexcept>

namespace cda
{
    void Config::validate() const
    {
        auto require = [](const bool ok, const char* msg)
        {
            if (!ok) throw std::invalid_argument(std::string("Config: ") + msg);
        };

        require(n_agents > 0, "n_agents must be positive");
        require(fundamental_price > 0.0, "fundamental_price must be positive");
        require(tau_f > 0.0, "tau_f must be positive");
        require(sigma_eps > 0.0, "sigma_eps must be positive");
        require(sigma_fundamental > 0.0, "sigma_fundamental must be positive");
        require(sigma_chartist > 0.0, "sigma_chartist must be positive");
        require(k_max >= 0.0, "k_max must be non-negative");
        require(min_memory >= 1, "min_memory must be at least 1");
        require(min_memory <= max_memory, "min_memory exceeds max_memory");
        require(min_horizon >= 1, "min_horizon must be at least 1");
        require(min_horizon <= max_horizon, "min_horizon exceeds max_horizon");
        require(order_lifetime > 0, "order_lifetime must be positive");
        require(n_steps > 0, "n_steps must be positive");
        require(min_price > 0.0, "min_price must be positive");
        require(max_log_move > 0.0, "max_log_move must be positive");
    }
}
