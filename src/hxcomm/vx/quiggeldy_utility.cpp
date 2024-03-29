#include "hxcomm/vx/quiggeldy_utility.h"

namespace hxcomm::vx {

std::tuple<pid_t, hxcomm::port_t> launch_quiggeldy_locally_from_env()
{
	auto const port = get_unused_port();

	auto env_timeout = std::getenv("QUIGGELDY_TIMEOUT");

	auto base_args = std::make_tuple(
	    "quiggeldy", port, "--no-allocate-license", "--timeout",
	    env_timeout == nullptr ? "10" : env_timeout, "--release", "31536000",
	    hxcomm::is_munge_available() ? "" : "--no-munge");

	pid_t pid = 0;
	if (std::getenv(vision_slurm_fpga_ips_env_name) != nullptr) {
		// run with FPGA IP from env
		auto const fpga_ip = hxcomm::get_fpga_ip();
		auto conn_args = std::tuple_cat(
		    base_args, std::make_tuple("--connect-ip", fpga_ip.c_str(), "--connection-arq"));
		pid = std::apply(
		    [](auto... args) -> pid_t { return hxcomm::setup_quiggeldy(args...); }, conn_args);
	} else {
		// run with cosim attached
		auto conn_args = std::tuple_cat(base_args, std::make_tuple("--connection-sim"));
		pid = std::apply(
		    [](auto... args) -> pid_t { return hxcomm::setup_quiggeldy(args...); }, conn_args);
	}
	if (pid <= 0) {
		throw std::runtime_error("Could not start quiggeldy");
	}

	setenv(vision_quiggeldy_enabled_env_name, "1", 1);
	setenv(vision_quiggeldy_ip_env_name, "127.0.0.1", 1);
	{
		std::stringstream port_ss;
		port_ss << port;

		setenv(vision_quiggeldy_port_env_name, port_ss.str().c_str(), 1);
	}
	return std::make_tuple(pid, port);
}


void unset_quiggeldy_env()
{
	unsetenv(vision_quiggeldy_enabled_env_name);
	unsetenv(vision_quiggeldy_ip_env_name);
	unsetenv(vision_quiggeldy_port_env_name);
}

} // namespace hxcomm::vx
