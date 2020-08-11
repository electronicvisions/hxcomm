
#include "hxcomm/vx/connection_from_env.h"

#include "hxcomm/common/fpga_ip_list.h"
#include "hxcomm/vx/arqconnection.h"
#include "hxcomm/vx/connection_variant.h"
#include "hxcomm/vx/simconnection.h"

#include <cstdlib>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace hxcomm::vx {

namespace detail {

inline std::vector<ConnectionVariant> get_simconnection_list_from_env()
{
	char const* env_sim_port = std::getenv("FLANGE_SIMULATION_RCF_PORT");
	char const* env_sim_host = std::getenv("FLANGE_SIMULATION_RCF_HOST");

	static std::string default_host = "127.0.0.1";
	if (env_sim_host == nullptr) {
		env_sim_host = default_host.c_str();
	}

	std::vector<ConnectionVariant> connection_list;
	if (env_sim_port != nullptr) {
		connection_list.emplace_back(ConnectionVariant{std::in_place_type<SimConnection>,
		                                               env_sim_host,
		                                               static_cast<uint16_t>(atoi(env_sim_port))});
	}
	return connection_list;
}

inline std::vector<ConnectionVariant> get_arqconnection_list_from_env(
    std::optional<size_t> limit = std::nullopt)
{
	auto fpga_ip_list = hxcomm::get_fpga_ip_list();

	if (fpga_ip_list.empty()) {
		return {};
	}

	if (limit && (fpga_ip_list.size() != *limit)) {
		throw std::runtime_error(
		    "Found FPGA IP amount different to specified limit (" +
		    std::to_string(fpga_ip_list.size()) + ") in environment to connect to.");
	}

	std::vector<ConnectionVariant> connection_list;
	for (auto const& fpga_ip : fpga_ip_list) {
		connection_list.emplace_back(ConnectionVariant{std::in_place_type<ARQConnection>, fpga_ip});
	}
	return connection_list;
}

} // namespace detail

std::vector<ConnectionVariant> get_connection_list_from_env(std::optional<size_t> limit)
{
	if (auto arq = detail::get_arqconnection_list_from_env(limit); !arq.empty()) {
		return arq;
	} else if (auto sim = detail::get_simconnection_list_from_env(); !sim.empty()) {
		return sim;
	} else {
		throw std::runtime_error("No executor backend found to connect to.");
	}
}

ConnectionVariant get_connection_from_env()
{
	return std::move(get_connection_list_from_env(1).at(0));
}

} // namespace hxcomm::vx
