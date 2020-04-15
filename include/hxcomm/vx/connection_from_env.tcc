#include "hxcomm/common/fpga_ip_list.h"
#include "hxcomm/vx/arqconnection.h"
#include "hxcomm/vx/connection_from_env.h"
#include "hxcomm/vx/connection_variant.h"
#include "hxcomm/vx/quiggeldy_connection.h"
#include "hxcomm/vx/simconnection.h"
#include "hxcomm/vx/zeromockconnection.h"

#include "slurm/vision_defines.h"

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

inline std::vector<ConnectionVariant> get_zeromockconnection_list_from_env()
{
	char const* env_zero_mock = std::getenv("HXCOMM_ENABLE_ZERO_MOCK");

	std::vector<ConnectionVariant> connection_list;
	if (env_zero_mock != nullptr && atoi(env_zero_mock)) {
		connection_list.emplace_back(ConnectionVariant{std::in_place_type<ZeroMockConnection>});
	}
	return connection_list;
}

inline std::vector<ConnectionVariant> get_quiggeldyclient_list_from_env(std::optional<size_t>)
{
	std::vector<ConnectionVariant> list;
	char const* env_quiggeldy_enabled = std::getenv(vision_quiggeldy_enabled_env_name);
	if (env_quiggeldy_enabled != nullptr && atoi(env_quiggeldy_enabled)) {
		list.emplace_back(ConnectionVariant{std::in_place_type<QuiggeldyConnection>});
	}
	return list;
}
} // namespace detail

std::vector<ConnectionVariant> get_connection_list_from_env(std::optional<size_t> limit)
{
	if (auto qgc = detail::get_quiggeldyclient_list_from_env(limit); !qgc.empty()) {
		return qgc;
	} else if (auto arq = detail::get_arqconnection_list_from_env(limit); !arq.empty()) {
		return arq;
	} else if (auto sim = detail::get_simconnection_list_from_env(); !sim.empty()) {
		return sim;
	} else if (auto zeromock = detail::get_zeromockconnection_list_from_env(); !zeromock.empty()) {
		return zeromock;
	} else {
		throw std::runtime_error("No executor backend found to connect to.");
	}
}


ConnectionVariant get_connection_from_env()
{
	return std::move(get_connection_list_from_env(1).at(0));
}

std::optional<hxcomm::vx::ConnectionFullStreamInterfaceVariant>
get_connection_full_stream_interface_from_env()
{
	ConnectionVariant variant_from_env(get_connection_from_env());

	return std::visit(
	    [](auto&& variant) -> std::optional<hxcomm::vx::ConnectionFullStreamInterfaceVariant> {
		    if constexpr (hxcomm::supports_full_stream_interface<decltype(variant)>::value) {
			    return std::make_optional<hxcomm::vx::ConnectionFullStreamInterfaceVariant>(
			        std::in_place_type<std::decay_t<decltype(variant)>>, std::move(variant));
		    } else {
			    return std::nullopt;
		    }
	    },
	    std::move(variant_from_env));
}

} // namespace hxcomm::vx
