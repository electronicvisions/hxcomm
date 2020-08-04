#include "flange/simulator_control_if.h"
#include "hxcomm/common/stream.h"
#include "hxcomm/vx/simconnection.h"
#include "reset_and_id_readout.h"
#include <boost/program_options.hpp>

namespace bpo = boost::program_options;

using namespace hxcomm::vx;

/**
 * Example script to reset chip and read JTAG-ID.
 */
int main(int argc, char* argv[])
{
	// parse arguments
	std::string str_ip;
	int port;
	bool only_print;
	bpo::options_description desc("Options");
	// clang-format off
	desc.add_options()("help", "produce help message")
	    ("ip", bpo::value<std::string>(&str_ip)->default_value("127.0.0.1"))
	    ("port", bpo::value<int>(&port)->default_value(flange::default_rcf_port))
	    ("only_print", bpo::bool_switch(&only_print)->default_value(false));
	// clang-format on

	bpo::variables_map vm;
	bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
	bpo::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	SimConnection connection(str_ip, port);
	auto stream = hxcomm::Stream(connection);
	reset_and_id_readout(stream);
}
