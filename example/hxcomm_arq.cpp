#include <iostream>

#include <boost/program_options.hpp>

#include "hxcomm/vx/arqconnection.h"

namespace bpo = boost::program_options;

using namespace hxcomm::vx;

using namespace hxcomm::vx::instruction;
using Data = instruction::to_fpga_jtag::Data;
using Ins = instruction::to_fpga_jtag::Ins;

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

	// Reset sequence
	connection.add(UTMessageToFPGA<system::Reset>(system::Reset::Payload(true)));
	connection.add(UTMessageToFPGA<timing::Setup>());
	connection.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(10)));
	connection.add(UTMessageToFPGA<system::Reset>(system::Reset::Payload(false)));
	connection.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(100)));
	connection.add(UTMessageToFPGA<to_fpga_jtag::Init>());

	// Set PLL
	connection.add(UTMessageToFPGA<Ins>(Ins::PLL_TARGET_REG));
	connection.add(UTMessageToFPGA<Data>(Data::Payload(true, Data::Payload::NumBits(3), 1)));
	connection.add(UTMessageToFPGA<Ins>(Ins::SHIFT_PLL));
	connection.add(
	    UTMessageToFPGA<Data>(Data::Payload(true, Data::Payload::NumBits(32), 0xC0C3F200)));
	connection.add(UTMessageToFPGA<Ins>(Ins::PLL_TARGET_REG));
	connection.add(UTMessageToFPGA<Data>(Data::Payload(true, Data::Payload::NumBits(3), 3)));
	connection.add(UTMessageToFPGA<Ins>(Ins::SHIFT_PLL));
	connection.add(
	    UTMessageToFPGA<Data>(Data::Payload(true, Data::Payload::NumBits(32), 0xC0C3F200)));

	// Read ID
	connection.add(UTMessageToFPGA<Ins>(Ins::IDCODE));
	connection.add(UTMessageToFPGA<Data>(Data::Payload(true, Data::Payload::NumBits(32), 0)));

	// Halt execution
	connection.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(10000)));
	connection.add(UTMessageToFPGA<system::Halt>());

	connection.commit();

	connection.run_until_halt();

	while (!connection.receive_empty()) {
		auto message = connection.receive();
		boost::apply_visitor([&message](auto m) { std::cout << m << std::endl; }, message);
	}
}
