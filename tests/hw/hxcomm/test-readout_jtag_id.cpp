#include <memory>
#include <queue>
#include <stdexcept>

#include <boost/program_options.hpp>
#include <gtest/gtest.h>

#include "hxcomm/simconnection.h"
#include "hxcomm/utmessage.h"

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

	using namespace hxcomm;

	std::queue<ut_message_t> messages;

	// Reset sequence
	messages.push(UTMessageFactory::init_msg(true));
	messages.push(UTMessageFactory::reset_timestamp());
	messages.push(UTMessageFactory::wait_until(10));
	messages.push(UTMessageFactory::init_msg(false));
	messages.push(UTMessageFactory::wait_until(100));

	// JTAG init
	messages.push(UTMessageFactory::set_clockscaler(3));
	messages.push(UTMessageFactory::reset_msg());

	// Read ID (JTAG instruction register is by specification IDCODE after init)
	messages.push(UTMessageFactory::set_dr(0, 32, 1));

	// Instantiate a Simulation client.
	hxcomm::SimConnection connection(str_ip, port);

	connection.set_runnable(true);
	while (!messages.empty()) {
		connection.send(messages.front());
		messages.pop();
	}
	std::vector<ut_message_t> responses;
	while (true) {
		// receive until timeout for printing all responses
		try {
			responses.push_back(connection.receive());
		} catch (std::runtime_error& ignored) {
			break;
		}
	}
	EXPECT_EQ(responses.size(), 1);
	EXPECT_EQ(static_cast<uint32_t>(responses.back()), 0x48580AF);
}
