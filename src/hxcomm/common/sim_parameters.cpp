#include "hxcomm/vx/connection_from_env.h"

#include <cstdlib>

namespace hxcomm {

std::tuple<std::string, uint16_t> get_sim_parameters()
{
	char const* env_sim_port = std::getenv("FLANGE_SIMULATION_RCF_PORT");
	char const* env_sim_host = std::getenv("FLANGE_SIMULATION_RCF_HOST");

	static std::string default_host = "127.0.0.1";
	if (env_sim_host == nullptr) {
		env_sim_host = default_host.c_str();
	}

	if (env_sim_port == nullptr) {
		throw std::runtime_error("No simulator parameters found in environment.");
	}

	return std::tuple{env_sim_host, static_cast<uint16_t>(atoi(env_sim_port))};
}

} // namespace hxcomm
