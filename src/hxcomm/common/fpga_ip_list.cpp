#include "hxcomm/common/fpga_ip_list.h"
#include "halco/common/misc_types.h"
#include "hwdb4cpp/hwdb4cpp.h"

#include "slurm/vision_defines.h"
#include <sstream>

namespace hxcomm {

std::vector<std::string> get_fpga_ip_list()
{
	char const* env_ip_list = std::getenv(vision_slurm_fpga_ips_env_name);
	std::vector<std::string> ip_list;
	if (env_ip_list != nullptr) {
		std::istringstream env_ip_list_stream(env_ip_list);
		std::string ip;
		while (getline(env_ip_list_stream, ip, ',')) {
			ip_list.push_back(ip);
		}
	}
	return ip_list;
}

std::string get_fpga_ip()
{
	auto const ip_list = get_fpga_ip_list();
	if (ip_list.empty()) {
		throw std::runtime_error("No FPGA IP found in environment (SLURM_FPGA_IPS).");
	} else if (ip_list.size() != 1) {
		throw std::runtime_error("More than one FPGA IP found in environment (SLURM_FPGA_IPS).");
	}
	return ip_list.at(0);
}

#ifdef WITH_HXCOMM_EXTOLL
std::optional<RMA2_Nodeid> convert_ip_to_extollid(std::string ip)
{
	std::optional<RMA2_Nodeid> id = std::nullopt;

	hwdb4cpp::database hwdb;
	hwdb.load(hwdb4cpp::database::get_default_path());
	auto const hxcube_ids = hwdb.get_hxcube_ids();
	for (auto const cube_id : hxcube_ids) {
		auto const& entry = hwdb.get_hxcube_setup_entry(cube_id);
		for (auto const& [f, e] : entry.fpgas) {
			if (e.ip.to_string() == ip) {
				id = e.extoll_node_id;
				break;
			}
		}
	}

	return id;
}

std::vector<RMA2_Nodeid> convert_ips_to_extollids(std::vector<std::string> ips)
{
	std::vector<RMA2_Nodeid> ids;

	for (auto ip : ips) {
		auto id = convert_ip_to_extollid(ip);
		if (id) {
			ids.push_back(convert_ip_to_extollid(ip).value());
		}
	}
	if (ids.size() != ips.size()) {
		throw std::runtime_error("Not all FPGA IPs have an associated Extoll-Node-ID");
	}
	return ids;
}
#endif // WITH_HXCOMM_EXTOLL

} // namespace hxcomm
