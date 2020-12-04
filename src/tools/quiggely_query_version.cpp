// Simple mock client connector to debug possible errors.

#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include "hxcomm/vx/quiggeldy_connection.h"
#include <boost/program_options.hpp>


#include "logger.h"
#include "logging_ctrl.h"

#include <iostream>

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
	logger_default_config(Logger::log4cxx_level_v2(HXCOMM_LOG_THRESHOLD));
	auto log = log4cxx::Logger::getLogger("hxcomm.quiggeldy_query_version");

	po::options_description desc{
	    "Query remote version information of running quiggeldy instance.\n\n"
	    "Allowed options"};

	hxcomm::port_t port;

	desc.add_options()("help,h", "produce help message")(
	    "ip,i", po::value<std::string>()->default_value(""), "IP to connect to")(
	    "port,p", po::value<hxcomm::port_t>(&port)->required(), "Port to connect to");

	// populate vm variable
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if (vm.count("help")) {
		std::cerr << desc << std::endl;
		return EXIT_SUCCESS;
	}
	po::notify(vm);

	std::string ip = vm["ip"].as<std::string>();
	auto connection = hxcomm::vx::QuiggeldyConnection(ip, port);

	std::cout << "quiggeldy running at " << ip << ":" << port << " "
	          << connection.get_version_string() << std::endl;
	return 0;
}
