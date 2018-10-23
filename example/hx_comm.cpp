#include <iomanip>
#include <iostream>
#include <memory>
#include <queue>
#include <stdexcept>

#include <boost/program_options.hpp>

#include "hxcomm/arqconnection.h"
#include "hxcomm/jtag.h"

namespace bpo = boost::program_options;

using namespace hxcomm;


int main(int argc, char* argv[])
{
	// parse arguments
	std::string str_ip;
	bpo::options_description desc("Options");
	desc.add_options()("help", "produce help message")("ip", bpo::value<std::string>(&str_ip));

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
	messages.push(UTMessageFactory::set_dr(1, 3, 1));
	messages.push(UTMessageFactory::set_ir(JTAG::SHIFT_PLL));
	messages.push(UTMessageFactory::set_dr(0xC0C3F200, 32, 1));
	messages.push(UTMessageFactory::set_ir(JTAG::PLL_TARGET_REG));
	messages.push(UTMessageFactory::set_dr(3, 3, 1));
	messages.push(UTMessageFactory::set_ir(JTAG::SHIFT_PLL));
	messages.push(UTMessageFactory::set_dr(0xC0C3F200, 32, 1));

	// Read ID
	messages.push(UTMessageFactory::set_ir(JTAG::IDCODE));
	messages.push(UTMessageFactory::set_dr(0, 32, 1));

	if (str_ip != "") {
		// connect HostARQ
		hxcomm::ARQConnection::ip_address_type fpga_ip;
		fpga_ip.from_string(str_ip);
		hxcomm::ARQConnection connection = ARQConnection(fpga_ip);
		while (!messages.empty()) {
			connection.send_one(messages.front());
			messages.pop();
		}
		while (true) {
			// receive until timeout for printing all responses
			connection.receive_one();
		}
	} else {
		// print messages
		while (!messages.empty()) {
			std::cout << std::setfill('0') << std::setw(16) << std::hex << messages.front() << "\n";
			messages.pop();
		}
	}
}
