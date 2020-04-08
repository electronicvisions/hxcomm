
#include "hxcomm/vx/connection_from_env.h"

#include "hxcomm/common/fpga_ip_list.h"
#include "hxcomm/vx/arqconnection.h"
#include "hxcomm/vx/connection_variant.h"
#include "hxcomm/vx/simconnection.h"

#include <cstdlib>
#include <utility>
#include <variant>

namespace hxcomm::vx {

ConnectionVariantType get_connection_from_env()
{
	auto fpga_ip_list = hxcomm::get_fpga_ip_list();
	char const* env_sim_port = std::getenv("FLANGE_SIMULATION_RCF_PORT");
	char const* env_sim_host = std::getenv("FLANGE_SIMULATION_RCF_HOST");

	static std::string default_host = "127.0.0.1";
	if (env_sim_host == nullptr) {
		env_sim_host = default_host.c_str();
	}

	if (fpga_ip_list.size() == 1) {
		return ConnectionVariantType{std::in_place_type<ARQConnection>, fpga_ip_list.front()};
	} else if (fpga_ip_list.size() > 1) {
		throw std::runtime_error("Found more than one FPGA IP in environment to connect to.");
	} else if (env_sim_port != nullptr) {
		return ConnectionVariantType{std::in_place_type<SimConnection>, env_sim_host,
		                             static_cast<uint16_t>(atoi(env_sim_port))};
	} else {
		throw std::runtime_error("No executor backend found to connect to.");
	}
}

} // namespace hxcomm::vx
