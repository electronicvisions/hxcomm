#include <iomanip>
#include <iostream>
#include <memory>
#include <queue>
#include <stdexcept>

#include <boost/program_options.hpp>

#include "flange/simulator_control_if.h"
#include "hxcomm/jtag.h"
#include "hxcomm/simconnection.h"
#include "hxcomm/utmessage.h"

namespace bpo = boost::program_options;

using namespace hxcomm;


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

	std::queue<ut_message_t> messages;

	// Reset sequence
	messages.push(UTMessageFactory::init_msg(true));
	messages.push(UTMessageFactory::reset_timestamp());
	messages.push(UTMessageFactory::wait_until(10));
	messages.push(UTMessageFactory::init_msg(false));
	messages.push(UTMessageFactory::wait_until(100));
	messages.push(UTMessageFactory::reset_msg());

	// Set PLL
	messages.push(UTMessageFactory::set_ir(JTAG::PLL_TARGET_REG));
	messages.push(UTMessageFactory::set_dr(1, 3, true));
	messages.push(UTMessageFactory::set_ir(JTAG::SHIFT_PLL));
	messages.push(UTMessageFactory::set_dr(0xC0C3F200, 32, true));
	messages.push(UTMessageFactory::set_ir(JTAG::PLL_TARGET_REG));
	messages.push(UTMessageFactory::set_dr(3, 3, true));
	messages.push(UTMessageFactory::set_ir(JTAG::SHIFT_PLL));
	messages.push(UTMessageFactory::set_dr(0xC0C3F200, 32, true));

	// Read ID
	messages.push(UTMessageFactory::set_ir(JTAG::IDCODE));
	messages.push(UTMessageFactory::set_dr(0, 32, true));

	if (only_print) {
		while (!messages.empty()) {
			std::cout << std::setfill('0') << std::setw(16) << std::hex << messages.front() << "\n";
			messages.pop();
		}
	} else {
		// Instantiate a Simulation client.
		hxcomm::SimConnection connection(str_ip, port);
		connection.set_runnable(true);
		while (!messages.empty()) {
			connection.send(messages.front());
			messages.pop();
		}
		while (true) {
			// receive until timeout for printing all responses
			try {
				std::cout << std::setfill('0') << std::setw(16) << std::hex << connection.receive()
				          << "\n";
			} catch (std::runtime_error& ignored) {
				break;
			}
		}
	}
}
