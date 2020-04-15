#include "hxcomm/vx/quiggeldy_utility.h"

namespace hxcomm::vx {

std::tuple<pid_t, hxcomm::port_t> launch_quiggely_locally_from_env()
{
	pid_t pid;
	auto port = get_unused_port();

	if (port <= 0) {
		throw std::runtime_error("No unused ports found!");
	}
	auto base_args = std::make_tuple(
	    "quiggeldy", port, "--loglevel", "3", "--no-allocate-gres", "--timeout", "10",
	    hxcomm::is_munge_available() ? "" : "--no-munge");

	if (std::getenv(vision_slurm_fpga_ips_env_name) != nullptr) {
		// run with FPGA IP from env
		auto conn_args = std::tuple_cat(
		    base_args,
		    std::make_tuple("--connect-ip", hxcomm::get_fpga_ip().c_str(), "--arq-connection"));
		pid = std::apply(
		    [](auto... args) -> pid_t { return hxcomm::setup_quiggeldy(args...); }, conn_args);
	} else {
		// run with cosim attached
		auto conn_args = std::tuple_cat(base_args, std::make_tuple("--sim-connection"));
		pid = std::apply(
		    [](auto... args) -> pid_t { return hxcom::setup_quiggeldy(args...); }, conn_args);
	}
	if (pid <= 0) {
		throw std::runtime_error("Could not start quiggeldy");
	}

	setenv(vision_quiggeldy_enabled_env_name, "yes", 1);
	setenv(vision_quiggeldy_ip_env_name, "127.0.0.1", 1);
	{
		std::stringstream port_ss;
		port_ss << port;

		setenv(vision_quiggeldy_port_env_name, port_ss.str().c_str(), 1);
	}
	return std::make_tuple(pid, port);
}

} // namespace hxcomm::vx
