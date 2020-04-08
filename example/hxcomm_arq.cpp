#include <boost/program_options.hpp>
#include "hxcomm/common/stream.h"
#include "hxcomm/vx/arqconnection.h"
#include "reset_and_id_readout.h"

namespace bpo = boost::program_options;

using namespace hxcomm::vx;

/**
 * Example script to reset chip and read JTAG-ID.
 */
int main(int argc, char* argv[])
{
	// parse arguments
	std::string str_ip;
	// clang-format off
	bpo::options_description desc("Options");
	desc.add_options()("help", "produce help message")
	("ip", bpo::value<std::string>(&str_ip));
	// clang-format on

	bpo::variables_map vm;
	bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
	bpo::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return EXIT_SUCCESS;
	}

	ARQConnection connection(str_ip);
	auto stream = hxcomm::Stream(connection);
	reset_and_id_readout(stream);
}
