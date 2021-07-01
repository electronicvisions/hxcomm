#include "hxcomm/common/fpga_ip_list.h"

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

} // namespace hxcomm
