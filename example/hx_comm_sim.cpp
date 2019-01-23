#include <iostream>

#include <boost/program_options.hpp>

#include "flange/simulator_control_if.h"
#include "hxcomm/vx/simconnection.h"

namespace bpo = boost::program_options;

using namespace hxcomm::vx;

using namespace hxcomm::vx::instruction;
using data = instruction::to_fpga_jtag::data;
using ins = instruction::to_fpga_jtag::ins;
using halt = instruction::system::halt;

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

	// Reset sequence
	connection.add(ut_message_to_fpga<system::reset>(system::reset::payload_type(true)));
	connection.add(ut_message_to_fpga<timing::setup>());
	connection.add(ut_message_to_fpga<timing::wait_until>(timing::wait_until::payload_type(10)));
	connection.add(ut_message_to_fpga<system::reset>(system::reset::payload_type(false)));
	connection.add(ut_message_to_fpga<timing::wait_until>(timing::wait_until::payload_type(100)));
	connection.add(ut_message_to_fpga<to_fpga_jtag::init>());

	// Set PLL
	connection.add(ut_message_to_fpga<ins>(ins::PLL_TARGET_REG));
	connection.add(
	    ut_message_to_fpga<data>(data::payload_type(true, data::payload_type::NumBits(3), 1)));
	connection.add(ut_message_to_fpga<ins>(ins::SHIFT_PLL));
	connection.add(ut_message_to_fpga<data>(
	    data::payload_type(true, data::payload_type::NumBits(32), 0xC0C3F200)));
	connection.add(ut_message_to_fpga<ins>(ins::PLL_TARGET_REG));
	connection.add(
	    ut_message_to_fpga<data>(data::payload_type(true, data::payload_type::NumBits(3), 3)));
	connection.add(ut_message_to_fpga<ins>(ins::SHIFT_PLL));
	connection.add(ut_message_to_fpga<data>(
	    data::payload_type(true, data::payload_type::NumBits(32), 0xC0C3F200)));

	// Read ID
	connection.add(ut_message_to_fpga<ins>(ins::IDCODE));
	connection.add(
	    ut_message_to_fpga<data>(data::payload_type(true, data::payload_type::NumBits(32), 0)));

	// Halt execution
	connection.add(ut_message_to_fpga<timing::wait_until>(timing::wait_until::payload_type(10000)));
	connection.add(ut_message_to_fpga<halt>());

	connection.commit();
	connection.run_until_halt();

	while (!connection.receive_empty()) {
		auto message = connection.receive();
		boost::apply_visitor([&message](auto m) { std::cout << m << std::endl; }, message);
	}
}
