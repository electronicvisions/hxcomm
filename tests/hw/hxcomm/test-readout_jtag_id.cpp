#include <memory>
#include <queue>
#include <stdexcept>

#include <boost/program_options.hpp>
#include <gtest/gtest.h>

#include "hxcomm/vx/simconnection.h"
#include "hxcomm/vx/utmessage.h"

extern int waf_gtest_argc;
extern char** waf_gtest_argv;

TEST(SimConnection, ReadoutJtagID)
{
	namespace bpo = boost::program_options;

	// parse arguments
	std::string str_ip;
	int port;
	bpo::options_description desc("Options");
	// clang-format off
	desc.add_options()("readout_jtag_id_ip", bpo::value<std::string>(&str_ip)->default_value("127.0.0.1"))
	    ("readout_jtag_id_port", bpo::value<int>(&port)->default_value(50001));
	// clang-format on

	bpo::variables_map vm;
	bpo::store(bpo::parse_command_line(waf_gtest_argc, waf_gtest_argv, desc), vm);
	bpo::notify(vm);

	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;
	using data = instruction::to_fpga_jtag::data;

	SimConnection connection(str_ip, port);

	// Reset sequence
	connection.add(ut_message_to_fpga<system::reset>(system::reset::payload_type(true)));
	connection.add(ut_message_to_fpga<timing::setup>());
	connection.add(ut_message_to_fpga<timing::wait_until>(timing::wait_until::payload_type(10)));
	connection.add(ut_message_to_fpga<system::reset>(system::reset::payload_type(false)));
	connection.add(ut_message_to_fpga<timing::wait_until>(timing::wait_until::payload_type(100)));

	// JTAG init
	connection.add(ut_message_to_fpga<to_fpga_jtag::scaler>(3));
	connection.add(ut_message_to_fpga<to_fpga_jtag::init>());

	// Read ID (JTAG instruction register is by specification IDCODE after init)
	connection.add(
	    ut_message_to_fpga<data>(data::payload_type(true, data::payload_type::NumBits(32), 0)));

	// Halt execution
	connection.add(ut_message_to_fpga<timing::wait_until>(timing::wait_until::payload_type(10000)));
	connection.add(ut_message_to_fpga<system::halt>());

	connection.commit();

	connection.run_until_halt();

	std::vector<ut_message_from_fpga_variant> responses;
	while (true) {
		try {
			responses.push_back(connection.receive());
		} catch (std::runtime_error& ignored) {
			break;
		}
	}
	EXPECT_EQ(responses.size(), 2);
	EXPECT_EQ(
	    static_cast<uint32_t>(
	        boost::get<ut_message_from_fpga<jtag_from_hicann::data>>(responses.front()).decode()),
	    0x48580AF);
}
