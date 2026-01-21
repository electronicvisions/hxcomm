#include "hxcomm/vx/connection_from_env.h"

#include "hxcomm/common/fpga_ip_list.h"
#ifdef WITH_HXCOMM_HOSTARQ
#include "hxcomm/vx/arqconnection.h"
#endif
#include "hxcomm/vx/connection_variant.h"
#include "hxcomm/vx/quiggeldy_connection.h"
#include "hxcomm/vx/simconnection.h"
#include "hxcomm/vx/zeromockconnection.h"

#include "slurm/vision_defines.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace hxcomm::vx {

namespace detail {


inline std::vector<SimConnection> get_simconnection_list_from_env()
{
	char const* env_sim_port = std::getenv("FLANGE_SIMULATION_RCF_PORT");
	char const* env_sim_host = std::getenv("FLANGE_SIMULATION_RCF_HOST");

	static std::string default_host = "127.0.0.1";
	if (env_sim_host == nullptr) {
		env_sim_host = default_host.c_str();
	}

	std::vector<SimConnection> connection_list;
	if (env_sim_port != nullptr) {
		connection_list.emplace_back(
		    SimConnection(env_sim_host, static_cast<uint16_t>(atoi(env_sim_port))));
	}
	return connection_list;
}

#ifdef WITH_HXCOMM_HOSTARQ
inline std::vector<ARQConnection> get_arqconnection_list_from_env(
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

	std::vector<ARQConnection> connection_list;
	for (auto const& fpga_ip : fpga_ip_list) {
		connection_list.emplace_back(ARQConnection(fpga_ip));
	}
	return connection_list;
}
#endif // WITH_HOSTARQ

inline std::vector<ZeroMockConnection> get_zeromockconnection_list_from_env()
{
	char const* env_zero_mock = std::getenv("HXCOMM_ENABLE_ZERO_MOCK");

	std::vector<ZeroMockConnection> connection_list;
	if (env_zero_mock != nullptr && atoi(env_zero_mock)) {
		connection_list.emplace_back(ZeroMockConnection());
	}
	return connection_list;
}

inline std::vector<QuiggeldyConnection> get_quiggeldyclient_list_from_env(std::optional<size_t>)
{
	std::vector<QuiggeldyConnection> connection_list;
	char const* env_quiggeldy_enabled = std::getenv(vision_quiggeldy_enabled_env_name);
	if (env_quiggeldy_enabled != nullptr && atoi(env_quiggeldy_enabled)) {
		QuiggeldyConnection conn{};

		// Check if there is a custom user name to be used by quiggeldy -> disable munge
		if (auto const custom_user_env = std::getenv(vision_quiggeldy_user_no_munge_env_name);
		    custom_user_env != nullptr) {
			conn.set_custom_username(std::string{custom_user_env});
			conn.set_use_munge(false);
		}

		connection_list.emplace_back(std::move(conn));
	}
	return connection_list;
}

} // namespace detail


ConnectionVariant get_connection_from_env(size_t connection_size)
{
	return std::move(get_connection_list_from_env(connection_size).at(0));
}


std::vector<ConnectionVariant> get_connection_list_from_env(size_t number_connections_per_multi)
{
	std::vector<ConnectionVariant> result;
	if (auto env_connections = detail::get_zeromockconnection_list_from_env();
	    !env_connections.empty()) {
		// If not further specified all connections are packed in a single MultiConnection.
		if (number_connections_per_multi == 0) {
			number_connections_per_multi = env_connections.size();
		}
		// If not on a JBOA setup the resulting MultiConnections only wrapp a single connection.
		if (!std::all_of(env_connections.begin(), env_connections.end(), [](auto const& conn) {
			    return std::holds_alternative<hwdb4cpp::JboaSetupEntry>(conn.get_hwdb_entry());
		    })) {
			number_connections_per_multi = 1;
		}

		std::vector<ZeroMockConnection> selection;

		for (size_t i = 0; i < env_connections.size(); i += number_connections_per_multi) {
			selection.insert(
			    selection.end(), std::make_move_iterator(env_connections.begin() + i),
			    std::make_move_iterator(
			        env_connections.begin() + i +
			        std::min(number_connections_per_multi, env_connections.size())));
			result.emplace_back(std::move(MultiZeroMockConnection(std::move(selection))));
			selection.clear();
		}
		return result;
	} else if (auto qgc = detail::get_quiggeldyclient_list_from_env(1); !qgc.empty()) {
		result.emplace_back(std::move(qgc.at(0)));
		return result;
#ifdef WITH_HXCOMM_HOSTARQ
	} else if (auto env_connections = detail::get_arqconnection_list_from_env();
	           !env_connections.empty()) {
		// If not further specified all connections are packed in a single MultiConnection.
		if (number_connections_per_multi == 0) {
			number_connections_per_multi = env_connections.size();
		}
		// If not on a JBOA setup the resulting MultiConnections only wrapp a single connection.
		if (!std::all_of(env_connections.begin(), env_connections.end(), [](auto const& conn) {
			    return std::holds_alternative<hwdb4cpp::JboaSetupEntry>(conn.get_hwdb_entry());
		    })) {
			number_connections_per_multi = 1;
		}

		std::vector<ARQConnection> selection;

		for (size_t i = 0; i < env_connections.size(); i += number_connections_per_multi) {
			selection.insert(
			    selection.end(), std::make_move_iterator(env_connections.begin() + i),
			    std::make_move_iterator(
			        env_connections.begin() + i +
			        std::min(number_connections_per_multi, env_connections.size())));
			result.emplace_back(std::move(MultiARQConnection(std::move(selection))));
			selection.clear();
		}
		return result;
#endif
	} else if (auto env_connections = detail::get_simconnection_list_from_env();
	           !env_connections.empty()) {
		// If not further specified all connections are packed in a single MultiConnection.
		if (number_connections_per_multi == 0) {
			number_connections_per_multi = env_connections.size();
		}
		// If not on a JBOA setup the resulting MultiConnections only wrapp a single connection.
		if (!std::all_of(env_connections.begin(), env_connections.end(), [](auto const& conn) {
			    return std::holds_alternative<hwdb4cpp::JboaSetupEntry>(conn.get_hwdb_entry());
		    })) {
			number_connections_per_multi = 1;
		}

		std::vector<SimConnection> selection;

		for (size_t i = 0; i < env_connections.size(); i += number_connections_per_multi) {
			selection.insert(
			    selection.end(), std::make_move_iterator(env_connections.begin() + i),
			    std::make_move_iterator(
			        env_connections.begin() + i +
			        std::min(number_connections_per_multi, env_connections.size())));
			result.emplace_back(std::move(MultiSimConnection(std::move(selection))));
			selection.clear();
		}
		return result;
	} else {
		throw std::runtime_error("No executor backend found to connect to.");
	}
}


std::optional<hxcomm::vx::SingleConnectionFullStreamInterfaceVariant>
get_connection_full_stream_interface_from_env()
{
	auto connection_variant = get_connection_from_env();
	std::optional<SingleConnectionVariant> single_variant_from_env;

	if (std::holds_alternative<MultiZeroMockConnection>(connection_variant)) {
		single_variant_from_env.emplace(
		    std::move(std::get<MultiZeroMockConnection>(connection_variant)[0]));
	}
#ifdef WITH_HXCOMM_HOSTARQ
	else if (std::holds_alternative<MultiARQConnection>(connection_variant)) {
		single_variant_from_env.emplace(
		    std::move(std::get<MultiARQConnection>(connection_variant)[0]));
	}
#endif
	else if (std::holds_alternative<MultiSimConnection>(connection_variant)) {
		single_variant_from_env.emplace(
		    std::move(std::get<MultiSimConnection>(connection_variant)[0]));
	} else if (std::holds_alternative<QuiggeldyConnection>(connection_variant)) {
		return std::nullopt; // No full stream interface for QuiggeldyConnetion.
	}

	if (!single_variant_from_env) {
		return std::nullopt;
	}

	return std::visit(
	    [](auto&& variant)
	        -> std::optional<hxcomm::vx::SingleConnectionFullStreamInterfaceVariant> {
		    if constexpr (hxcomm::supports_full_stream_interface<decltype(variant)>::value) {
			    return std::make_optional<hxcomm::vx::SingleConnectionFullStreamInterfaceVariant>(
			        std::in_place_type<std::decay_t<decltype(variant)>>, std::move(variant));
		    } else {
			    return std::nullopt;
		    }
	    },
	    std::move(*single_variant_from_env));
}

} // namespace hxcomm::vx
